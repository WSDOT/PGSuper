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

// TxDOTAgentImp.cpp : Implementation of CTxDOTAgentImp

#include "stdafx.h"
#include "TxDOTAgentImp.h"

#include <IFace\StatusCenter.h>
#include <IReportManager.h>
#include <IFace\Project.h>
#include <IFace\Artifact.h>
#include <IFace\Test1250.h>
#include <IFace\GirderHandling.h>

#include <PgsExt\BridgeDescription.h>

#include <EAF\EAFAutoProgress.h>
#include <EAF\EAFApp.h>
#include <EAF\EAFUtilities.h>

#include <Reporting\PGSuperTitlePageBuilder.h>
#include <Reporting\SpanGirderReportSpecificationBuilder.h>
#include <Reporting\SpecCheckSummaryChapterBuilder.h>
#include <Reporting\LoadingDetailsChapterBuilder.h>
#include <Reporting\LiveLoadDetailsChapterBuilder.h>
#include <Reporting\UserDefinedLoadsChapterBuilder.h>

#include "TexasIBNSChapterBuilder.h"
#include "TexasGirderSummaryChapterBuilder.h"
#include "TexasPrestressSummaryChapterBuilder.h"
#include "TexasCamberAndDeflectionChapterBuilder.h"
#include "TexasStressChecksChapterBuilder.h"
#include "TexasMomentCapacityChapterBuilder.h"
#include "TexasShearChapterBuilder.h"

#include "TxDOTOptionalDesignSummaryChapterBuilder.h"
#include "TogaStressChecksChapterBuilder.h"
#include "TogaSpecCheckSummaryChapterBuilder.h"


#include "TxDOTCadWriter.h"


DECLARE_LOGFILE;

// CTxDOTAgentImp

/////////////////////////////////////////////////////////////////////////////
// IAgentEx
//
STDMETHODIMP CTxDOTAgentImp::SetBroker(IBroker* pBroker)
{
   AGENT_SET_BROKER(pBroker);
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

   AGENT_INIT;

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

   boost::shared_ptr<CReportSpecificationBuilder> pSpanGirderRptSpecBuilder( new CSpanGirderReportSpecificationBuilder(m_pBroker) );


   // Texas Girder Schedule - use compacted title page
   CReportBuilder* pRptBuilder = new CReportBuilder("TxDOT Girder Schedule Report");
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(new CPGSuperTitlePageBuilder(m_pBroker,pRptBuilder->GetName(),false)) );
   pRptBuilder->SetReportSpecificationBuilder( pSpanGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTexasIBNSChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTexasCamberAndDeflectionChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   // Texas Summary report
   pRptBuilder = new CReportBuilder("TxDOT Summary Report");
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(new CPGSuperTitlePageBuilder(m_pBroker,pRptBuilder->GetName())) );
   pRptBuilder->SetReportSpecificationBuilder( pSpanGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CSpecCheckSummaryChapterBuilder(true)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTexasGirderSummaryChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLoadingDetailsChapterBuilder(true,true,false,true)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLiveLoadDetailsChapterBuilder(true,false)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CUserDefinedLoadsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTexasPrestressSummaryChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTexasCamberAndDeflectionChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTexasStressChecksChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTexasMomentCapacityChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTexasShearChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   // TOGA Long Form
   pRptBuilder = new CReportBuilder("TxDOT Optional Girder Analysis (TOGA) - Long Form",true);
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(new CPGSuperTitlePageBuilder(m_pBroker,pRptBuilder->GetName(),false)) );
   pRptBuilder->SetReportSpecificationBuilder( pSpanGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTogaSpecCheckSummaryChapterBuilder(true)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTxDOTOptionalDesignSummaryChapterBuilder()) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CLoadingDetailsChapterBuilder(true,true,false,true)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CUserDefinedLoadsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTexasPrestressSummaryChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTexasCamberAndDeflectionChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTogaStressChecksChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTexasMomentCapacityChapterBuilder) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTexasShearChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder );

   // TOGA Short Form
   pRptBuilder = new CReportBuilder("TxDOT Optional Girder Analysis (TOGA) - Short Form",true);
   pRptBuilder->AddTitlePageBuilder( boost::shared_ptr<CTitlePageBuilder>(new CPGSuperTitlePageBuilder(m_pBroker,pRptBuilder->GetName(),false)) );
   pRptBuilder->SetReportSpecificationBuilder( pSpanGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTogaSpecCheckSummaryChapterBuilder(true)) );
   pRptBuilder->AddChapterBuilder( boost::shared_ptr<CChapterBuilder>(new CTxDOTOptionalDesignSummaryChapterBuilder()) );
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
   CLOSE_LOGFILE;
   AGENT_CLEAR_INTERFACE_CACHE;
   return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// ITxDOTCadExport
int CTxDOTAgentImp::WriteCADDataToFile(FILE *fp, IBroker* pBroker, SpanIndexType span, GirderIndexType gdr, TxDOTCadExportFormatType format, bool designSucceeded)
{
   return TxDOT_WriteCADDataToFile(fp,pBroker,span, gdr, format, designSucceeded);
}

int CTxDOTAgentImp::WriteDistributionFactorsToFile(FILE *fp, IBroker* pBroker, SpanIndexType span, GirderIndexType gdr)
{
   return TxDOT_WriteDistributionFactorsToFile(fp,pBroker,span,gdr);
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

   return (txCmdInfo.m_bError ? TRUE : FALSE);
}


void CTxDOTAgentImp::ProcessTxDotCad(const CTxDOTCommandLineInfo& rCmdInfo)
{
   ASSERT(rCmdInfo.m_DoTxCadReport);


   if (rCmdInfo.m_TxGirder != TXALLGIRDERS && 
       rCmdInfo.m_TxGirder != TXEIGIRDERS && 
       (rCmdInfo.m_TxGirder < 0 || 27 < rCmdInfo.m_TxGirder))
   {
      ::AfxMessageBox("Invalid girder specified on command line for TxDOT CAD report");
      return;
   }

   if (rCmdInfo.m_TxSpan != ALL_SPANS && rCmdInfo.m_TxSpan < 0)
   {
      ::AfxMessageBox("Invalid span specified on command line for TxDOT CAD report");
      return;
   }

   CString errfile;
   if (CreateTxDOTFileNames(rCmdInfo.m_TxOutputFile, &errfile))
   {
      try
      {
         if ( !DoTxDotCadReport(rCmdInfo.m_TxOutputFile, errfile, rCmdInfo) )
         {
            CString msg = CString("Error - Running test on file")+rCmdInfo.m_strFileName;
            ::AfxMessageBox(msg);
         }
      }
      catch(const sysXBase& e)
      {
         std::string msg;
         e.GetErrorMessage(&msg);
         std::ofstream os;
         os.open(errfile);
         os <<"Error running TxDOT CAD report for input file: "<<rCmdInfo.m_strFileName<<std::endl<< msg;
      }
      catch(CException* pex)
      {
         TCHAR   szCause[255];
         CString strFormatted;
         pex->GetErrorMessage(szCause, 255);
         std::ofstream os;
         os.open(errfile);
         os <<"Error running TxDOT CAD report for input file: "<<rCmdInfo.m_strFileName<<std::endl<< szCause;
         delete pex;
      }
      catch(CException& ex)
      {
         TCHAR   szCause[255];
         CString strFormatted;
         ex.GetErrorMessage(szCause, 255);
         std::ofstream os;
         os.open(errfile);
         os <<"Error running TxDOT CAD report for input file: "<<rCmdInfo.m_strFileName<<std::endl<< szCause;
      }
      catch(const std::exception* pex)
      {
         std::string strMsg(pex->what());
         std::ofstream os;
         os.open(errfile);
         os <<"Error running TxDOT CAD report for input file: "<<rCmdInfo.m_strFileName<<std::endl<<strMsg<< std::endl;
         delete pex;
      }
      catch(const std::exception& ex)
      {
          std::string strMsg(ex.what());
         std::ofstream os;
         os.open(errfile);
         os <<"Error running TxDOT CAD report for input file: "<<rCmdInfo.m_strFileName<<std::endl<<strMsg<< std::endl;
      }
      catch(...)
      {
         std::ofstream os;
         os.open(errfile);
         os <<"Unknown Error running TxDOT CAD report for input file: "<<rCmdInfo.m_strFileName;
      }
   }
}


bool CTxDOTAgentImp::CreateTxDOTFileNames(const CString& output, CString* pErrFileName)
{
   CString tmp(output);
   tmp.MakeLower();
   int loc = tmp.Find(".",0);
   if (loc>0)
   {
      CString basename = output.Left(loc);
      *pErrFileName = basename + ".err";
      return true;
   }
   else
   {
      *pErrFileName = output + ".err";

      return false;
   }
}

bool CTxDOTAgentImp::DoTxDotCadReport(const CString& outputFileName, const CString& errorFileName, const CTxDOTCommandLineInfo& txInfo)
{
   // Called from the command line processing in the Application object

   // open the error file
   std::ofstream err_file(errorFileName);
   if ( !err_file )
   {
	   AfxMessageBox ("Could not Create error file");
	   return false;
   }

   // Open/create the specified output file 
   FILE	*fp;
   errno_t result;
   if (txInfo.m_DoAppendToFile)
   {
      result = fopen_s(&fp, LPCTSTR (outputFileName), "a+");
   }
   else
   {
      result = fopen_s(&fp, LPCTSTR (outputFileName), "w+");
   }

   if (result != 0 || fp == NULL)
   {
      err_file<<"Error: Output file could not be Created."<<std::endl;
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
         err_file<<"Span value is out of range for this bridge"<<std::endl;
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
      }

      else if (txInfo.m_TxGirder<nGirders)
      {
         SpanGirderHashType key = HashSpanGirder(is,txInfo.m_TxGirder);
         spn_grd_list.push_back(key);
      }
      else
      {
         err_file<<"Girder value is out of range for this bridge"<<std::endl;
	      return false;
      }
   }

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   // Loop over all span/girder combos and create results
   for(std::vector<SpanGirderHashType>::iterator it=spn_grd_list.begin(); it!=spn_grd_list.end(); it++)
   {
      SpanGirderHashType key = *it;
      SpanIndexType span;
      GirderIndexType girder;
      UnhashSpanGirder(key, &span, &girder);

      CString strMessage;
      strMessage.Format("Creating TxDOT CAD report for Span %d, Girder %s",LABEL_SPAN(span), LABEL_GIRDER(girder));
      pProgress->UpdateMessage(strMessage);

      // See if we need to run a design
      bool designSucceeded=true;
      if (txInfo.m_TxRunType == CTxDOTCommandLineInfo::txrDesign)
      {
         // get design options from library entry. Do flexure only
         GET_IFACE(ISpecification,pSpecification);
         arDesignOptions des_options = pSpecification->GetDesignOptions(span,girder);

         des_options.doDesignForShear = false; // shear design is off

         GET_IFACE(IArtifact,pIArtifact);
         const pgsDesignArtifact* pArtifact;
         try
         {
            // Design the girder
            pArtifact = pIArtifact->CreateDesignArtifact( span,girder, des_options);
         
            if (pArtifact->GetOutcome() != pgsDesignArtifact::Success)
            {
               err_file <<"Design was unsuccessful"<<std::endl;
               designSucceeded=false;
            }

            // and copy the design to the bridge
            SaveFlexureDesign(span,girder, des_options, pArtifact);
         }
         catch(...)
         {
           err_file <<"Design Failed for span"<<span<<" girder "<<girder<<std::endl;
            return false;
         }
      }

      GET_IFACE(ITxDOTCadExport,pTxDOTCadExport);
      if ( !pTxDOTCadExport )
      {
         AfxMessageBox("The TxDOT Cad Exporter is not currently installed");
         return false;
      }

      if (txInfo.m_TxRunType == CTxDOTCommandLineInfo::TxrDistributionFactors)
      {
         // Write distribution factor data to file
         if (CAD_SUCCESS != pTxDOTCadExport->WriteDistributionFactorsToFile (fp, this->m_pBroker, span, girder))
         {
            err_file <<"Warning: An error occured while writing to File"<<std::endl;
	         return false;
         }
      }
      else
      {
         /* Write CAD data to text file */
         if (CAD_SUCCESS != pTxDOTCadExport->WriteCADDataToFile(fp, this->m_pBroker, span, girder, (TxDOTCadExportFormatType)txInfo.m_TxFType, designSucceeded) )
         {
            err_file <<"Warning: An error occured while writing to File"<<std::endl;
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
      if (create_test_file_names(txInfo.m_strFileName,&resultsfile,&poifile,&errfile))
      {
         GET_IFACE(ITest1250, ptst );

         try
         {
            if (!ptst->RunTestEx(RUN_CADTEST, spn_grd_list, std::string(resultsfile), std::string(poifile)))
            {
               CString msg = CString("Error - Running test on file")+txInfo.m_strFileName;
               ::AfxMessageBox(msg);
            }
         }
         catch(...)
         {
            CString msg = CString("Error - running test for input file:")+txInfo.m_strFileName;
            ::AfxMessageBox(msg);
            return false;
         }
      }
      else
      {
         CString msg = CString("Error - Determining 1250 test file names for")+txInfo.m_strFileName;
         ::AfxMessageBox(msg);
         return false;
      }
   }


   return true;
}


void CTxDOTAgentImp::SaveFlexureDesign(SpanIndexType span,GirderIndexType gdr,const arDesignOptions& designOptions,const pgsDesignArtifact* pArtifact)
{
   GET_IFACE(IGirderData,pGirderData);
   GET_IFACE(IStrandGeometry, pStrandGeometry );

   CGirderData girderData = pGirderData->GetGirderData(span, gdr);

   // Convert Harp offset data
   // offsets are absolute measure in the design artifact
   // convert them to the measurement basis that the CGirderData object is using
   girderData.HpOffsetAtEnd = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(span, gdr, 
                                                                                  pArtifact->GetNumHarpedStrands(), 
                                                                                  girderData.HsoEndMeasurement, 
                                                                                  pArtifact->GetHarpStrandOffsetEnd());

   girderData.HpOffsetAtHp = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(span, gdr, 
                                                                                pArtifact->GetNumHarpedStrands(), 
                                                                                girderData.HsoHpMeasurement, 
                                                                                pArtifact->GetHarpStrandOffsetHp());



#pragma Reminder("############ - Update with loop after updating Artifact #############")
   // see if strand design data fits in grid
   bool fills_grid=false;
   StrandIndexType num_permanent = pArtifact->GetNumHarpedStrands() + pArtifact->GetNumStraightStrands();
   if (designOptions.doStrandFillType==ftGridOrder)
   {
      // we asked design to fill using grid, but this may be a non-standard design - let's check
      StrandIndexType ns, nh;
      if (pStrandGeometry->ComputeNumPermanentStrands(num_permanent, span, gdr, &ns, &nh))
      {
         if (ns==pArtifact->GetNumStraightStrands() && nh==pArtifact->GetNumHarpedStrands() )
         {
            fills_grid = true;
         }
      }
   }

   if (fills_grid)
   {
      girderData.NumPermStrandsType = NPS_TOTAL_NUMBER;

      girderData.Nstrands[pgsTypes::Permanent]            = num_permanent;
      girderData.Pjack[pgsTypes::Permanent]               = pArtifact->GetPjackStraightStrands() + pArtifact->GetPjackHarpedStrands();
      girderData.bPjackCalculated[pgsTypes::Permanent]    = pArtifact->GetUsedMaxPjackStraightStrands();
   }
   else
   {
      girderData.NumPermStrandsType = NPS_STRAIGHT_HARPED;
   }

   girderData.Nstrands[pgsTypes::Harped]            = pArtifact->GetNumHarpedStrands();
   girderData.Nstrands[pgsTypes::Straight]          = pArtifact->GetNumStraightStrands();
   girderData.Nstrands[pgsTypes::Temporary]         = pArtifact->GetNumTempStrands();
   girderData.Pjack[pgsTypes::Harped]               = pArtifact->GetPjackHarpedStrands();
   girderData.Pjack[pgsTypes::Straight]             = pArtifact->GetPjackStraightStrands();
   girderData.Pjack[pgsTypes::Temporary]            = pArtifact->GetPjackTempStrands();
   girderData.bPjackCalculated[pgsTypes::Harped]    = pArtifact->GetUsedMaxPjackHarpedStrands();
   girderData.bPjackCalculated[pgsTypes::Straight]  = pArtifact->GetUsedMaxPjackStraightStrands();
   girderData.bPjackCalculated[pgsTypes::Temporary] = pArtifact->GetUsedMaxPjackTempStrands();
   girderData.LastUserPjack[pgsTypes::Harped]       = pArtifact->GetPjackHarpedStrands();
   girderData.LastUserPjack[pgsTypes::Straight]     = pArtifact->GetPjackStraightStrands();
   girderData.LastUserPjack[pgsTypes::Temporary]    = pArtifact->GetPjackTempStrands();

   girderData.TempStrandUsage = pArtifact->GetTemporaryStrandUsage();

   // Get debond information from design artifact
   girderData.ClearDebondData();
   girderData.bSymmetricDebond = true;  // design is always symmetric

   DebondInfoCollection dbcoll = pArtifact->GetStraightStrandDebondInfo();
   // TRICKY: Mapping from DEBONDINFO to CDebondInfo is tricky because
   //         former designates individual strands and latter stores symmetric strands
   //         in pairs.
   // sort this collection by strand idices to ensure we get it right
   std::sort( dbcoll.begin(), dbcoll.end() ); // default < operator is by index

   for (DebondInfoIterator dbit = dbcoll.begin(); dbit!=dbcoll.end(); dbit++)
   {
      const DEBONDINFO& rdbrinfo = *dbit;

      CDebondInfo cdbi;
      cdbi.idxStrand1 = rdbrinfo.strandIdx;

      // if the difference between the current and next number of strands is 2, this is a pair
      StrandIndexType currnum = rdbrinfo.strandIdx;
      StrandIndexType nextnum = pStrandGeometry->GetNextNumStrands(span, gdr, pgsTypes::Straight, currnum);
      if (nextnum-currnum == 2)
      {
         dbit++; // increment counter to account for a pair
         cdbi.idxStrand2 = dbit->strandIdx;

         // some asserts to ensure we got things right
         ATLASSERT(cdbi.idxStrand1+1 == cdbi.idxStrand2);
         ATLASSERT(rdbrinfo.LeftDebondLength == dbit->LeftDebondLength);
         ATLASSERT(rdbrinfo.RightDebondLength == dbit->RightDebondLength);
      }
      else
      {
         // not a pair
         cdbi.idxStrand2 = INVALID_INDEX;
      }
      cdbi.Length1    = rdbrinfo.LeftDebondLength;
      cdbi.Length2    = rdbrinfo.RightDebondLength;

      girderData.Debond[pgsTypes::Straight].push_back(cdbi);
   }
   
   // concrete
   girderData.Material.Fci = pArtifact->GetReleaseStrength();
   girderData.Material.Fc  = pArtifact->GetConcreteStrength();

   pGirderData->SetGirderData( girderData, span, gdr );

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   CBridgeDescription bridgeDesc = *pIBridgeDesc->GetBridgeDescription();
   CSpanData* pSpan = bridgeDesc.GetSpan(span);
   CGirderTypes girderTypes = *(pSpan->GetGirderTypes());
   girderTypes.SetSlabOffset(gdr,pgsTypes::metStart,pArtifact->GetSlabOffset(pgsTypes::metStart));
   girderTypes.SetSlabOffset(gdr,pgsTypes::metEnd,  pArtifact->GetSlabOffset(pgsTypes::metEnd));
   pSpan->SetGirderTypes(girderTypes);
   pIBridgeDesc->SetBridgeDescription(bridgeDesc);

   GET_IFACE(IGirderLifting,pLifting);
   pLifting->SetLiftingLoopLocations(span, gdr,pArtifact->GetLeftLiftingLocation(),pArtifact->GetRightLiftingLocation());

   GET_IFACE(IGirderHauling,pHauling);
   pHauling->SetTruckSupportLocations(span, gdr,pArtifact->GetTrailingOverhang(),pArtifact->GetLeadingOverhang());

}
