///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include "TexasLoadRatingSummaryChapterBuilder.h"
#include "TOGATitlePageBuilder.h"

#include "TxDOTOptionalDesignSummaryChapterBuilder.h"
#include "TogaStressChecksChapterBuilder.h"
#include "TogaSpecCheckSummaryChapterBuilder.h"
#include "TogaCamberAndDeflectionChapterBuilder.h"
#include "TogaLongSectionChapterBuilder.h"
#include "TexasHaunchChapterBuilder.h"

#include "TxDOTCadWriter.h"
#include "TOGATestFileWriter.h"

static bool CreateTxDOTFileNames(const CString& output, CString* pErrFileName)
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
   clsidMap->AddCLSID(CComBSTR("{360F7694-BE5B-4E97-864F-EF3575689C6E}"),CComBSTR("{3700B253-8489-457C-8A6D-D174F95C457C}"));

   return S_OK;
}

STDMETHODIMP CTxDOTAgentImp::RegInterfaces()
{
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

   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pGirderRptSpecBuilder( std::make_shared<CGirderReportSpecificationBuilder>(m_pBroker,CGirderKey(0,0)) );
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pMultiGirderRptSpecBuilder( std::make_shared<CMultiGirderReportSpecificationBuilder>(m_pBroker) );
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pMultiViewRptSpecBuilder( std::make_shared<CMultiViewSpanGirderReportSpecificationBuilder>(m_pBroker) );


   // Texas Girder Schedule - use compacted title page
   std::shared_ptr<WBFL::Reporting::ReportBuilder> pRptBuilder(std::make_shared<WBFL::Reporting::ReportBuilder>(_T("TxDOT Girder Schedule Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(std::make_shared<CPGSuperTitlePageBuilder>(m_pBroker,pRptBuilder->GetName(),false,false)) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTexasIBNSChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTexasCamberAndDeflectionChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTexasHaunchChapterBuilder>()) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   // Texas Summary report - short
   pRptBuilder = std::make_shared<WBFL::Reporting::ReportBuilder>(_T("TxDOT Summary Report (Short Form)"));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(std::make_shared<CPGSuperTitlePageBuilder>(m_pBroker,pRptBuilder->GetName(), false,false)) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiViewRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CSpecCheckSummaryChapterBuilder>(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTexasGirderSummaryChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLoadingDetailsChapterBuilder>(true,true,false,true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLiveLoadDetailsChapterBuilder>(true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CUserDefinedLoadsChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTexasCamberAndDeflectionChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTexasHaunchChapterBuilder>()) );
//   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CBearingDeductChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTexasLoadRatingSummaryChapterBuilder>(false)) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   // Texas Summary report - long form
   pRptBuilder = std::make_shared<WBFL::Reporting::ReportBuilder>(_T("TxDOT Summary Report (Long Form)"));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(std::make_shared<CPGSuperTitlePageBuilder>(m_pBroker,pRptBuilder->GetName(),true,false)) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiViewRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CSpecCheckSummaryChapterBuilder>(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTexasGirderSummaryChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLoadingDetailsChapterBuilder>(true,true,false,true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLiveLoadDetailsChapterBuilder>(true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CUserDefinedLoadsChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTexasCamberAndDeflectionChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTexasHaunchChapterBuilder>()) );
//   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CBearingDeductChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTexasPrestressSummaryChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTexasStressChecksChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTexasMomentCapacityChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTexasShearChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTexasLoadRatingSummaryChapterBuilder>(false)) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   // TOGA Long Form
   pRptBuilder = std::make_shared<WBFL::Reporting::ReportBuilder>(_T("TxDOT Optional Girder Analysis (TOGA) - Long Report"),true);
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(std::make_shared<CTOGATitlePageBuilder>(m_pBroker,pRptBuilder->GetName(),false)) );
   pRptBuilder->SetReportSpecificationBuilder( pGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTogaSpecCheckSummaryChapterBuilder>(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTxDOTOptionalDesignSummaryChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTogaLongSectionChapterBuilder>()) );
//   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CSectPropChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CSectPropChapterBuilder>(true,true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CLoadingDetailsChapterBuilder>(true,true,false,true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CUserDefinedLoadsChapterBuilder>(true,true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTexasPrestressSummaryChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTogaCamberAndDeflectionChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTexasHaunchChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTogaStressChecksChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTexasMomentCapacityChapterBuilder>()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTexasShearChapterBuilder>()) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   // TOGA Short Form
   pRptBuilder = std::make_shared<WBFL::Reporting::ReportBuilder>(_T("TxDOT Optional Girder Analysis (TOGA) - Short Report"),true);
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<WBFL::Reporting::TitlePageBuilder>(std::make_shared<CTOGATitlePageBuilder>(m_pBroker,pRptBuilder->GetName(),false)) );
   pRptBuilder->SetReportSpecificationBuilder( pGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTogaSpecCheckSummaryChapterBuilder>(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<WBFL::Reporting::ChapterBuilder>(std::make_shared<CTxDOTOptionalDesignSummaryChapterBuilder>()) );
   pRptMgr->AddReportBuilder( pRptBuilder );

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

    if (txCmdInfo.m_DoTogaTest && !txCmdInfo.m_bError)
   {
      ProcessTOGAReport(txCmdInfo);
      return TRUE;
   }

   return (txCmdInfo.m_bError ? TRUE : FALSE);
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
   catch(const WBFL::System::XBase& e)
   {
      std::_tstring msg = e.GetErrorMessage();
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

   // Close the open text file 
   fclose (fp);

   return true;
}
