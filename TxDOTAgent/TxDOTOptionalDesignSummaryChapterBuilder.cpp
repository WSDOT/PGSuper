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

#include "StdAfx.h"
#include <Reporting\ReportStyleHolder.h>
#include <Reporting\SpanGirderReportSpecification.h>

#include "TxDOTOptionalDesignSummaryChapterBuilder.h"

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderData.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\PierData.h>
#include <PgsExt\BridgeDescription.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include "TxDOTOptionalDesignData.h"
#include "TxDOTOptionalDesignUtilities.h"

#include <psgLib\ConnectionLibraryEntry.h>

#include <WBFLCogo.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static void design_information(rptChapter* pChapter,IBroker* pBroker,const CTxDOTOptionalDesignData* pProjectData,IEAFDisplayUnits* pDisplayUnits);
static void design_data(rptChapter* pChapter,IBroker* pBroker,const CTxDOTOptionalDesignData* pProjectData,IEAFDisplayUnits* pDisplayUnits);
static void girder_design(rptChapter* pChapter,IBroker* pBroker,const CTxDOTOptionalDesignGirderData* pGirderData,
                          GirderIndexType gdr, IEAFDisplayUnits* pDisplayUnits);
static void non_standard_table(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits, const std::string& tableName, 
                               const CTxDOTOptionalDesignGirderData::StrandRowContainer& strandRows );
static void original_results_summary(rptChapter* pChapter,IBroker* pBroker,const CTxDOTOptionalDesignData* pProjectData,IEAFDisplayUnits* pDisplayUnits);
static void optional_results_summary(rptChapter* pChapter,IBroker* pBroker,const CTxDOTOptionalDesignData* pProjectData,IEAFDisplayUnits* pDisplayUnits);
static void camber_summary(rptChapter* pChapter,IBroker* pBroker,const CTxDOTOptionalDesignData* pProjectData,IEAFDisplayUnits* pDisplayUnits);

/****************************************************************************
CLASS
   CTxDOTOptionalDesignSummaryChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTxDOTOptionalDesignSummaryChapterBuilder::CTxDOTOptionalDesignSummaryChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CTxDOTOptionalDesignSummaryChapterBuilder::GetName() const
{
   return TEXT("Optional Design Summary");
}

rptChapter* CTxDOTOptionalDesignSummaryChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IGetTogaData,pGetTogaData);
   const CTxDOTOptionalDesignData* pProjectData = pGetTogaData->GetTogaData();


#if defined IGNORE_2007_CHANGES
   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
   {

      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << color(Red) << bold(ON) << "Changes to LRFD 4th Edition, 2007, Article 5.4.2.3.2 have been ignored." << bold(OFF) << color(Black) << rptNewLine;
   }
#endif

   design_information( pChapter, pBroker, pProjectData, pDisplayUnits );
   design_data       ( pChapter, pBroker, pProjectData, pDisplayUnits );

   girder_design(pChapter, pBroker, pProjectData->GetOriginalDesignGirderData(),  TOGA_ORIG_GDR, pDisplayUnits);
   girder_design(pChapter, pBroker, pProjectData->GetPrecasterDesignGirderData(), TOGA_FABR_GDR, pDisplayUnits);

   original_results_summary( pChapter, pBroker, pProjectData, pDisplayUnits );
   optional_results_summary( pChapter, pBroker, pProjectData, pDisplayUnits );
   camber_summary(pChapter, pBroker, pProjectData, pDisplayUnits);

   return pChapter;
}

CChapterBuilder* CTxDOTOptionalDesignSummaryChapterBuilder::Clone() const
{
   return new CTxDOTOptionalDesignSummaryChapterBuilder;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////
void design_information(rptChapter* pChapter,IBroker* pBroker,const CTxDOTOptionalDesignData* pProjectData, IEAFDisplayUnits* pDisplayUnits)
{
   // interfaces
   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry );
   GET_IFACE2(pBroker, IBridgeMaterial, pMaterial);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   GET_IFACE2(pBroker,IEnvironment,pEnvironment);
   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, component, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptPressureUnitValue, stress,      pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  modE,    pDisplayUnits->GetModEUnit(),         false );

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table = pgsReportStyleHolder::CreateTableNoHeading(2,"Design Information");
   *p << p_table << rptNewLine;


   RowIndexType row = 0;
   (*p_table)(row,0) << "Span";
   (*p_table)(row++,1) << LABEL_SPAN(TOGA_SPAN);

   (*p_table)(row,0) << "Girder";
   (*p_table)(row++ ,1) << LABEL_GIRDER(TOGA_FABR_GDR);

   (*p_table)(row,0) << "Girder Type";
   (*p_table)(row++,1) << pProjectData->GetBeamType();

   (*p_table)(row,0) << "Span Length, CL Bearing to CL Bearing" ;
   (*p_table)(row++,1) << length.SetValue(pBridge->GetSpanLength(TOGA_SPAN,TOGA_FABR_GDR));

   ASSERT(pgsTypes::sbsUniform == pBridgeDesc->GetGirderSpacingType());
   (*p_table)(row,0) << "Beam Spacing" ;
   (*p_table)(row++,1) << length.SetValue(pBridgeDesc->GetGirderSpacing());

   pgsPointOfInterest zero_poi(TOGA_SPAN,TOGA_FABR_GDR,0.0);

   (*p_table)(row,0) << "Slab Thickness";
   (*p_table)(row++,1) << component.SetValue(pBridge->GetStructuralSlabDepth( zero_poi ));

   (*p_table)(row,0) << "Relative Humidity";
   (*p_table)(row++,1) <<  pEnvironment->GetRelHumidity()<<"%";

   (*p_table)(row,0) << RPT_EC <<" Slab";
   (*p_table)(row++,1) << modE.SetValue( pMaterial->GetEcSlab() );

   (*p_table)(row,0) << RPT_EC <<" Beam";
   (*p_table)(row++,1) << modE.SetValue( pMaterial->GetEcGdr(TOGA_SPAN,TOGA_FABR_GDR) );

   (*p_table)(row,0) << "Slab Compressive Strength";
   (*p_table)(row++,1) << stress.SetValue( pMaterial->GetFcSlab() );

   const matPsStrand* pstrand = pMaterial->GetStrand(TOGA_SPAN,TOGA_FABR_GDR);

   (*p_table)(row,0) << "Prestressing Strands";
   (*p_table)(row++,1) << get_strand_size(pstrand->GetSize()) <<", "
                       <<(pstrand->GetGrade() == matPsStrand::Gr1725 ? "Grade 250, " : "Grade 270, ")
                       <<(pstrand->GetType() == matPsStrand::LowRelaxation ? "Low Relaxation" : "Stress Relieved");

   (*p_table)(row,0) << "Live Load";
   std::vector<std::string> designLiveLoads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltDesign);
   std::vector<std::string>::iterator iter;
   bool first = true;
   for (iter = designLiveLoads.begin(); iter != designLiveLoads.end(); iter++)
   {
      if(!first)
         (*p_table)(row,1) << ", ";

      std::string& load_name = *iter;
      (*p_table)(row,1) << load_name;

      first = false;
   }
}

static void design_data(rptChapter* pChapter,IBroker* pBroker,const CTxDOTOptionalDesignData* pProjectData,IEAFDisplayUnits* pDisplayUnits)
{
   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptPressureUnitValue, stress,      pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,   moment,      pDisplayUnits->GetMomentUnit(), true );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, fpl,   pDisplayUnits->GetForcePerLengthUnit(), true );

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table = pgsReportStyleHolder::CreateTableNoHeading(2,"Design Data");
   *p << p_table << rptNewLine;

   RowIndexType row = 0;
   (*p_table)(row,0) << "Design Load Compressive Stress (Top C.L.), "<<RPT_FTOP;
   (*p_table)(row++,1) << stress.SetValue( pProjectData->GetFt() );

   (*p_table)(row,0) << "Design Load Tensile Stress (Bottom C.L.), "<<RPT_FBOT;
   (*p_table)(row++,1) << stress.SetValue( pProjectData->GetFb() );

   (*p_table)(row,0) << "Required Ultimate Moment Capacity, M"<<Sub("u");
   (*p_table)(row++,1) << moment.SetValue( pProjectData->GetMu() );

   (*p_table)(row,0) << "LLDF (Moment)";
   (*p_table)(row++,1) << pProjectData->GetLldfMoment();

   (*p_table)(row,0) << "LLDF (Shear)";
   (*p_table)(row++,1) << pProjectData->GetLldfShear();

   (*p_table)(row,0) << "w"<<Sub("DC")<<" non-comp";
   (*p_table)(row++,1) << fpl.SetValue(pProjectData->GetWNonCompDc());

   (*p_table)(row,0) << "w"<<Sub("DC")<<" comp";
   (*p_table)(row++,1) << fpl.SetValue(pProjectData->GetWCompDc());

   (*p_table)(row,0) << "w"<<Sub("DW")<<" non-comp";
   (*p_table)(row++,1) << fpl.SetValue(pProjectData->GetWCompDw());
}

void girder_design(rptChapter* pChapter,IBroker* pBroker,const CTxDOTOptionalDesignGirderData* pGirderData,
                          GirderIndexType gdr, IEAFDisplayUnits* pDisplayUnits)
{
   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptPressureUnitValue, stress,      pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, component, pDisplayUnits->GetComponentDimUnit(), true );

   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry );
   GET_IFACE2(pBroker,IBridge,pBridge);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   std::string title = (gdr==TOGA_ORIG_GDR) ? "Original Girder Design" : "Fabricator Optional Girder Design";

   rptRcTable* p_table = pgsReportStyleHolder::CreateTableNoHeading(2,title);
   *p << p_table << rptNewLine;

   RowIndexType row = 0;
   (*p_table)(row,0) << RPT_FCI;
   (*p_table)(row++,1) << stress.SetValue( pGirderData->GetFci() );

   (*p_table)(row,0) << RPT_FC;
   (*p_table)(row++,1) << stress.SetValue( pGirderData->GetFc() );

   (*p_table)(row,0) << "No. Strands";

   std::string note;
   if (pGirderData->GetStandardStrandFill())
   {
      note = " (standard fill used)";
   }
   else
   {
      note = (pGirderData->GetUseDepressedStrands()) ? " (non-standard fill, with depressed strands)" : " (non-standard fill, with all straight strands)";
   }

   (*p_table)(row++,1) << pStrandGeometry->GetNumStrands(TOGA_SPAN,gdr,pgsTypes::Permanent) << note;

   if (pGirderData->GetStandardStrandFill())
   {
      (*p_table)(row,0) << "Yb of Topmost Depressed Strand(s) @ End";
      (*p_table)(row++,1) << component.SetValue( pGirderData->GetStrandTo() );
   }

   Float64 span2 = pBridge->GetSpanLength(TOGA_SPAN,gdr)/2.0;
   pgsPointOfInterest midpoi(TOGA_SPAN,gdr,span2);

   (*p_table)(row,0) << "e"<<Sub("CL");
   Float64 neff;
   (*p_table)(row++,1) << component.SetValue( pStrandGeometry->GetEccentricity(midpoi,false,&neff) );

   pgsPointOfInterest zeropoi(TOGA_SPAN,gdr,0.0);
   (*p_table)(row,0) << "e"<<Sub("girder ends");
   (*p_table)(row++,1) << component.SetValue( pStrandGeometry->GetEccentricity(zeropoi,false,&neff) );

   // non standard fill row tables
   if (!pGirderData->GetStandardStrandFill())
   {
      non_standard_table(pChapter, pDisplayUnits, "Non-Standard Strand Pattern at Girder Centerline",
                         pGirderData->GetStrandsAtCL());

      if (pGirderData->GetUseDepressedStrands())
      {
         non_standard_table(pChapter, pDisplayUnits, "Non-Standard Strand Pattern at Girder Ends",
                            pGirderData->GetStrandsAtEnds());
      }
   }
}

void non_standard_table(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits, const std::string& tableName, 
                   const CTxDOTOptionalDesignGirderData::StrandRowContainer& strandRows )
{
   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptLengthUnitValue, component, pDisplayUnits->GetComponentDimUnit(), false );

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   int nrows = strandRows.size() + 1;

   rptRcTable* p_table = pgsReportStyleHolder::CreateTableNoHeading(nrows,tableName);
   *p << p_table << rptNewLine;

   std::string tit = "Row (" + component.GetUnitTag() + ")";

   (*p_table)(0,0) << Bold( tit.c_str() );
   (*p_table)(1,0) << Bold("No. Strands");

   int col = 1;
   for (CTxDOTOptionalDesignGirderData::StrandRowConstIterator it=strandRows.begin(); it!=strandRows.end(); it++)
   {
      (*p_table)(0,col)   << component.SetValue( it->RowElev );
      (*p_table)(1,col++) << it->StrandsInRow;
   }
}

static void original_results_summary(rptChapter* pChapter,IBroker* pBroker,const CTxDOTOptionalDesignData* pProjectData,IEAFDisplayUnits* pDisplayUnits)
{
   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptPressureUnitValue, stress,      pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,   moment,      pDisplayUnits->GetMomentUnit(), false );

   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2(pBroker,IGetTogaResults,pGetTogaResults);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(5,"Input Design Data Vs. Analysis of Original Design");
   *p << p_table;

   int row = 0;
   (*p_table)(row,0) << "Value";
   (*p_table)(row,1) << "Input"<<rptNewLine<<"Design Data";
   (*p_table)(row,2) << "Analysis of"<<rptNewLine<<"Original Design";
   (*p_table)(row,3) << "Input/Analysis"<<rptNewLine<<"Ratio";
   (*p_table)(row,4) << "Status";

   row++;
   (*p_table)(row,0) << "Required Ultimate Moment  ("<<moment.GetUnitTag()<<")";
   (*p_table)(row,1) << moment.SetValue( pProjectData->GetMu() );
   (*p_table)(row,2) << moment.SetValue( pGetTogaResults->GetRequiredUltimateMoment() );
   (*p_table)(row,3) << pProjectData->GetMu() / pGetTogaResults->GetRequiredUltimateMoment();

   if(pProjectData->GetMu() >= pGetTogaResults->GetRequiredUltimateMoment())
      (*p_table)(row,4) << color(Green) << "Ok" << color(Black);
   else
      (*p_table)(row,4) << color(Red) << "Design Deficiency" << color(Black);

   Float64 stress_val, stress_fac, stress_loc;
   pGetTogaResults->GetControllingCompressiveStress(&stress_val, &stress_fac, &stress_loc);

   row++;
   (*p_table)(row,0) << "Design Load Compressive Stress, Top CL ("<<stress.GetUnitTag()<<")";
   (*p_table)(row,1) << stress.SetValue( pProjectData->GetFt() );
   (*p_table)(row,2) << stress.SetValue( -1.0 * stress_val );
   (*p_table)(row,3) << stress_fac;

   if(stress_fac >= 1.0)
      (*p_table)(row,4) << color(Green) << "Ok" << color(Black);
   else
      (*p_table)(row,4) << color(Red) << "Design Deficiency" << color(Black);

   pGetTogaResults->GetControllingTensileStress(&stress_val, &stress_fac, &stress_loc);

   row++;
   (*p_table)(row,0) << "Design Load Tensile Stress, Bottom CL ("<<stress.GetUnitTag()<<")";
   (*p_table)(row,1) << stress.SetValue( pProjectData->GetFb() );
   (*p_table)(row,2) << stress.SetValue( -1.0 * stress_val );
   (*p_table)(row,3) << stress_fac;

   if(stress_fac >= 1.0)
      (*p_table)(row,4) << color(Green) << "Ok" << color(Black);
   else
      (*p_table)(row,4) << color(Red) << "Design Deficiency" << color(Black);

   *p<<"NOTES: Values in the above table reflect the following sign convention:"<<rptNewLine;
   *p<<"Compressive stress is positive. Tensile stress is negative.";

}

static void optional_results_summary(rptChapter* pChapter,IBroker* pBroker,const CTxDOTOptionalDesignData* pProjectData,IEAFDisplayUnits* pDisplayUnits)
{
   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptPressureUnitValue, stress,      pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,   moment,      pDisplayUnits->GetMomentUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,   length, pDisplayUnits->GetSpanLengthUnit(), false );

   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2(pBroker,IGetTogaResults,pGetTogaResults);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(5,"Analysis of Fabricator Optional Design");
   *p << p_table;

   int row = 0;
   (*p_table)(row,0) << "Value";
   (*p_table)(row,1) << "Input"<<rptNewLine<<"Design Data";
   (*p_table)(row,2) << "Analysis of"<<rptNewLine<<"Fabricator"<<rptNewLine<<"Optional Design";
   (*p_table)(row,3) << "Input/Analysis"<<rptNewLine<<"Ratio";
   (*p_table)(row,4) << "Status";

   Float64 input_fc = pProjectData->GetPrecasterDesignGirderData()->GetFc();
   Float64 reqd_fc = pGetTogaResults->GetRequiredFc();

   row++;
   (*p_table)(row,0) << "Required Concrete Strength  ("<<stress.GetUnitTag()<<")";
   (*p_table)(row,1) << stress.SetValue( input_fc );
   (*p_table)(row,2) << stress.SetValue(  reqd_fc );
   (*p_table)(row,3) << input_fc / reqd_fc;

   if(input_fc >= reqd_fc)
      (*p_table)(row,4) << color(Green) << "Ok" << color(Black);
   else
      (*p_table)(row,4) << color(Red) << bold(ON) << "Beam Does not Satisfy Design Requirements" << bold(OFF) << color(Black);

   row++;
   (*p_table)(row,0) << "Ultimate Moment Capacity  ("<<moment.GetUnitTag()<<")";
   (*p_table)(row,1) << moment.SetValue( pProjectData->GetMu() );
   (*p_table)(row,2) << moment.SetValue( pGetTogaResults->GetUltimateMomentCapacity() );
   (*p_table)(row,3) << pProjectData->GetMu() / pGetTogaResults->GetUltimateMomentCapacity();

   if(pProjectData->GetMu() <= pGetTogaResults->GetUltimateMomentCapacity())
      (*p_table)(row,4) << color(Green) << "Ok" << color(Black);
   else
      (*p_table)(row,4) << color(Red) << bold(ON) << "Beam Does not Satisfy Design Requirements" << bold(OFF) << color(Black);
}



static void camber_summary(rptChapter* pChapter,IBroker* pBroker,const CTxDOTOptionalDesignData* pProjectData,IEAFDisplayUnits* pDisplayUnits)
{
   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptLengthUnitValue,   length, pDisplayUnits->GetSpanLengthUnit(), false );

   GET_IFACE2(pBroker,IGetTogaResults,pGetTogaResults);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(5,"Camber Analysis");
   *p << p_table;

   int row = 0;
   (*p_table)(row,0) << "Value";
   (*p_table)(row,1) << "Analysis of"<<rptNewLine<<"Original Design";
   (*p_table)(row,2) << "Analysis of"<<rptNewLine<<"Fabricator"<<rptNewLine<<"Optional Design";
   (*p_table)(row,3) << "Original/"<<rptNewLine<<"Fabricator"<<rptNewLine<<"Ratio";
   (*p_table)(row,4) << "Status";

   row++;
   (*p_table)(row,0) << "Maximum Camber  ("<<length.GetUnitTag()<<")";
   (*p_table)(row,1) << length.SetValue( pGetTogaResults->GetMaximumCamber() );
   (*p_table)(row,2) << length.SetValue( pGetTogaResults->GetFabricatorMaximumCamber() );
   (*p_table)(row,3) << pGetTogaResults->GetMaximumCamber()/pGetTogaResults->GetFabricatorMaximumCamber();

   if(pGetTogaResults->GetMaximumCamber() >= pGetTogaResults->GetFabricatorMaximumCamber())
      (*p_table)(row,4) << color(Green) << "Ok" << color(Black);
   else
      (*p_table)(row,4) << color(Red) << "Design Deficiency" << color(Black);

   *p<<"NOTE: Upward Camber is positive";
}