///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#include <Reporting\SpanGirderReportSpecification.h>

#include "TxDOTOptionalDesignSummaryChapterBuilder.h"

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\PierData2.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\DebondUtil.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>

#include "TxDOTOptionalDesignData.h"
#include "TxDOTOptionalDesignUtilities.h"
#include "TexasIBNSParagraphBuilder.h"

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
static void non_standard_table(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits, const std::_tstring& tableName, 
                               const CTxDOTOptionalDesignGirderData::StrandRowContainer& strandRows );
static void original_results_summary(rptChapter* pChapter,IBroker* pBroker,const CTxDOTOptionalDesignData* pProjectData,IEAFDisplayUnits* pDisplayUnits);
static void optional_results_summary(rptChapter* pChapter,IBroker* pBroker,const CTxDOTOptionalDesignData* pProjectData,IEAFDisplayUnits* pDisplayUnits);
static void camber_summary(rptChapter* pChapter,IBroker* pBroker,const CTxDOTOptionalDesignData* pProjectData,IEAFDisplayUnits* pDisplayUnits);
static void shear_summary(rptChapter* pChapter,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits);

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
   CBrokerReportSpecification* pBrokerRptSpec = dynamic_cast<CBrokerReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pBrokerRptSpec->GetBroker(&pBroker);
   
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IGetTogaData,pGetTogaData);
   const CTxDOTOptionalDesignData* pProjectData = pGetTogaData->GetTogaData();

   design_information( pChapter, pBroker, pProjectData, pDisplayUnits );
   design_data       ( pChapter, pBroker, pProjectData, pDisplayUnits );

   // Original girder
   girder_design(pChapter, pBroker, pProjectData->GetOriginalDesignGirderData(),  TOGA_ORIG_GDR, pDisplayUnits);

   // Throw in a page break
   rptParagraph* p = new rptParagraph;
   *pChapter << p;
   *p << rptNewPage;

   // Fab opt girder
   girder_design(pChapter, pBroker, pProjectData->GetPrecasterDesignGirderData(), TOGA_FABR_GDR, pDisplayUnits);

   original_results_summary( pChapter, pBroker, pProjectData, pDisplayUnits );
   optional_results_summary( pChapter, pBroker, pProjectData, pDisplayUnits );

   // Throw in a page break
   p = new rptParagraph;
   *pChapter << p;
   *p << rptNewPage;

   camber_summary(pChapter, pBroker, pProjectData, pDisplayUnits);
   shear_summary(pChapter, pBroker, pDisplayUnits);

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
   CSegmentKey fabrSegmentKey(TOGA_SPAN,TOGA_FABR_GDR,0);

   // interfaces
   GET_IFACE2(pBroker, IMaterials,         pMaterial);
   GET_IFACE2(pBroker, IBridge,            pBridge);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   GET_IFACE2(pBroker, IEnvironment,       pEnvironment);
   GET_IFACE2(pBroker, ILiveLoads,         pLiveLoads);
   GET_IFACE2(pBroker, IIntervals,         pIntervals);

   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptLengthUnitValue,   length,    pDisplayUnits->GetSpanLengthUnit(),   true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,   component, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptPressureUnitValue, stress,    pDisplayUnits->GetStressUnit(),       true );
   INIT_UV_PROTOTYPE( rptStressUnitValue,   modE,      pDisplayUnits->GetModEUnit(),         true );
   INIT_UV_PROTOTYPE( rptDensityUnitValue, density, pDisplayUnits->GetDensityUnit(),         true );

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table = rptStyleManager::CreateTableNoHeading(2,_T("Design Information"));
   *p << p_table << rptNewLine;


   RowIndexType row = 0;
   (*p_table)(row,0) << _T("Span No.");
   (*p_table)(row++,1) << pProjectData->GetSpanNo();

   (*p_table)(row,0) << _T("Beam No.");
   (*p_table)(row++ ,1) << pProjectData->GetBeamNo();

   (*p_table)(row,0) << _T("Beam Type");
   (*p_table)(row++,1) << pProjectData->GetBeamType();

   (*p_table)(row,0) << _T("Span Length (CL Bearings)") ;
   (*p_table)(row++,1) << length.SetValue(pBridge->GetSegmentSpanLength(fabrSegmentKey));

   ASSERT(pgsTypes::sbsUniform==pBridgeDesc->GetGirderSpacingType() || pgsTypes::sbsUniformAdjacent==pBridgeDesc->GetGirderSpacingType());
   Float64 spacing = pBridgeDesc->GetGirderSpacing();

   if (pgsTypes::sbsUniformAdjacent==pBridgeDesc->GetGirderSpacingType())
   {
      // For adjacent girders, the spacing returned above is the joint spacing. Add the girder width to 
      // this to get the CL-CL girder spacing
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(fabrSegmentKey.groupIndex);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(fabrSegmentKey.girderIndex);

      const GirderLibraryEntry* pGdr = pGirder->GetGirderLibraryEntry();

      Float64 wid = pGdr->GetBeamWidth(pgsTypes::metStart);
      spacing += wid;
   }

   (*p_table)(row,0) << _T("Beam Spacing") ;
   (*p_table)(row++,1) << length.SetValue(spacing);

   pgsPointOfInterest zero_poi(fabrSegmentKey,0.0);

   (*p_table)(row,0) << _T("Slab Thickness");
   (*p_table)(row++,1) << component.SetValue(pBridge->GetStructuralSlabDepth( zero_poi ));

   (*p_table)(row,0) << _T("Relative Humidity");
   (*p_table)(row++,1) <<  pEnvironment->GetRelHumidity()<<_T("%");

   (*p_table)(row,0) << _T("LLDF (Moment)");
   (*p_table)(row++,1) << pProjectData->GetLldfMoment();

   (*p_table)(row,0) << _T("LLDF (Shear)");
   (*p_table)(row++,1) << pProjectData->GetLldfShear();

   (*p_table)(row,0) << RPT_EC <<_T(" Deck");
   (*p_table)(row++,1) << modE.SetValue( pMaterial->GetDeckEc(0,liveLoadIntervalIdx) );

   (*p_table)(row,0) << RPT_EC <<_T(" Beam");
   (*p_table)(row++,1) << modE.SetValue( pMaterial->GetSegmentEc(fabrSegmentKey,liveLoadIntervalIdx) );

   (*p_table)(row,0) << RPT_FC <<_T(" Deck");
   (*p_table)(row++,1) << stress.SetValue( pMaterial->GetDeckDesignFc(liveLoadIntervalIdx) );

   (*p_table)(row,0) <<_T("Unit Weight, Beam");
   (*p_table)(row++,1) << density.SetValue( pMaterial->GetSegmentWeightDensity(fabrSegmentKey,liveLoadIntervalIdx) );

   (*p_table)(row,0) << _T("Project Criteria");
   (*p_table)(row++,1) << pProjectData->GetSelectedProjectCriteriaLibrary();

   (*p_table)(row,0) << _T("Live Load");
   std::vector<std::_tstring> designLiveLoads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltDesign);
   std::vector<std::_tstring>::iterator iter;
   bool first = true;
   for (iter = designLiveLoads.begin(); iter != designLiveLoads.end(); iter++)
   {
      if(!first)
         (*p_table)(row,1) << _T(", ");

      std::_tstring& load_name = *iter;
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

   rptRcTable* p_table = rptStyleManager::CreateTableNoHeading(2,_T("Design Data"));
   *p << p_table << rptNewLine;

   RowIndexType row = 0;
   (*p_table)(row,0) << RPT_FTOP <<_T(", Design Load Compressive Stress, Top CL");
   (*p_table)(row++,1) << stress.SetValue( pProjectData->GetFt() );

   (*p_table)(row,0) <<RPT_FBOT<< _T(", Design Load Tensile Stress, Bottom CL");
   (*p_table)(row++,1) << stress.SetValue( pProjectData->GetFb() );

   (*p_table)(row,0) <<_T("M")<<Sub(_T("u"))<< _T(", Required Ultimate Moment Capacity");
   (*p_table)(row++,1) << moment.SetValue( pProjectData->GetMu() );

   (*p_table)(row,0) << _T("W")<<Sub(_T("non-comp DC"));
   (*p_table)(row++,1) << fpl.SetValue(pProjectData->GetWNonCompDc());

   (*p_table)(row,0) << _T("W")<<Sub(_T("comp DC"));
   (*p_table)(row++,1) << fpl.SetValue(pProjectData->GetWCompDc());

   (*p_table)(row,0) << _T("W")<<Sub(_T("Overlay"));
   (*p_table)(row++,1) << fpl.SetValue(pProjectData->GetWCompDw());

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   Float64 allow_stf = pSpecEntry->GetAtReleaseCompressionStressFactor();
   (*p_table)(row,0) << _T("Allowable Compressive Stress Factor at Release");
   (*p_table)(row++,1) << allow_stf << RPT_FCI;

   *p <<Bold(_T("Note:"))<<_T(" Values in the above table reflect the following sign convention: Compressive stress is positive. Tensile stress is negative.");

}

void girder_design(rptChapter* pChapter,IBroker* pBroker,const CTxDOTOptionalDesignGirderData* pGirderData,
                          GirderIndexType gdrIdx, IEAFDisplayUnits* pDisplayUnits)
{
   CSegmentKey segmentKey(TOGA_SPAN,gdrIdx,0);

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptPressureUnitValue, stress,      pDisplayUnits->GetStressUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, component, pDisplayUnits->GetComponentDimUnit(), true );

   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry );
   GET_IFACE2(pBroker, IMaterials, pMaterial);

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   std::_tstring title = (gdrIdx==TOGA_ORIG_GDR) ? _T("Original Girder Design") : _T("Fabricator Optional Girder Design");

   rptRcTable* p_table = rptStyleManager::CreateTableNoHeading(2,title.c_str());
   *p << p_table << rptNewLine;

   RowIndexType row = 0;
   (*p_table)(row,0) << RPT_FCI;
   (*p_table)(row++,1) << stress.SetValue( pGirderData->GetFci() );

   (*p_table)(row,0) << RPT_FC;
   (*p_table)(row++,1) << stress.SetValue( pGirderData->GetFc() );

   const matPsStrand* pstrand = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Permanent);

   (*p_table)(row,0) << _T("Prestressing Strands");
   (*p_table)(row++,1) << get_strand_size(pstrand->GetSize()) <<_T(", ")
                       <<(pstrand->GetGrade() == matPsStrand::Gr1725 ? _T("Grade 250, ") : _T("Grade 270, "))
                       <<(pstrand->GetType() == matPsStrand::LowRelaxation ? _T("Low Relaxation") : _T("Stress Relieved"));

   (*p_table)(row,0) << _T("No. Strands");

   CTxDOTOptionalDesignGirderData::StrandFillType fill_type = pGirderData->GetStrandFillType();

   std::_tstring note;
   if (fill_type == CTxDOTOptionalDesignGirderData::sfStandard)
   {
      note = _T(" (standard fill used)");
   }
   else if (fill_type == CTxDOTOptionalDesignGirderData::sfHarpedRows)
   {
      note = _T(" (non-standard fill, with depressed strands)");
   }
   else if (fill_type == CTxDOTOptionalDesignGirderData::sfDirectFill)
   {
      note = _T(" (non-standard direct straight strand fill)");
   }

   (*p_table)(row++,1) << pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Permanent) << note;

   if( pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Permanent) > 0) // no use reporting strand data if there are none
   {
      StrandIndexType nh = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Harped);
      if (fill_type == CTxDOTOptionalDesignGirderData::sfStandard && nh>0)
      {
         (*p_table)(row,0) << _T("Girder Bottom to Topmost Strand (To)");
         (*p_table)(row++,1) << component.SetValue( pGirderData->GetStrandTo() );
      }

      StrandIndexType ndb = pStrandGeometry->GetNumDebondedStrands(segmentKey, pgsTypes::Straight,pgsTypes::dbetEither);
      if (ndb>0)
      {
         (*p_table)(row,0) << _T("No. Debonded");
         (*p_table)(row++,1) << ndb;
      }

      GET_IFACE2(pBroker, IBridge, pBridge);
      Float64 span2 = pBridge->GetSegmentSpanLength(segmentKey)/2.0;
      pgsPointOfInterest midpoi(segmentKey,span2);

      (*p_table)(row,0) << _T("e")<<Sub(_T("CL"));
      Float64 neff;
      (*p_table)(row++,1) << component.SetValue( pStrandGeometry->GetEccentricity(releaseIntervalIdx, midpoi,pgsTypes::Permanent, &neff) );

      pgsPointOfInterest zeropoi(segmentKey,0.0);
      (*p_table)(row,0) << _T("e")<<Sub(_T("girder ends"));
      (*p_table)(row++,1) << component.SetValue( pStrandGeometry->GetEccentricity(releaseIntervalIdx, zeropoi,pgsTypes::Permanent,&neff) );

      // non standard fill row tables
      if (fill_type == CTxDOTOptionalDesignGirderData::sfHarpedRows)
      {
         non_standard_table(pChapter, pDisplayUnits, _T("Non-Standard Strand Pattern at Girder Centerline"),
                            pGirderData->GetStrandsAtCL());

         non_standard_table(pChapter, pDisplayUnits, _T("Non-Standard Strand Pattern at Girder Ends"),
                            pGirderData->GetStrandsAtEnds());
      }

      if (fill_type == CTxDOTOptionalDesignGirderData::sfDirectFill)
      {
         // Write debond tables if direct fill
         Float64 girder_length = pBridge->GetSegmentLength(segmentKey);

         TxDOTIBNSDebondWriter tx_writer(segmentKey, girder_length, pStrandGeometry);
         tx_writer.WriteDebondData(p, pBroker, pDisplayUnits, title);
      }
   }
}

void non_standard_table(rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits, const std::_tstring& tableName, 
                   const CTxDOTOptionalDesignGirderData::StrandRowContainer& strandRows )
{
   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptLengthUnitValue, component, pDisplayUnits->GetComponentDimUnit(), false );

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   RowIndexType nrows = strandRows.size() + 1;

   rptRcTable* p_table = rptStyleManager::CreateTableNoHeading(nrows,tableName.c_str());
   *p << p_table << rptNewLine;

   std::_tstring tit = _T("Row (") + component.GetUnitTag() + _T(")");

   (*p_table)(0,0) << Bold( tit.c_str() );
   (*p_table)(1,0) << Bold(_T("No. Strands"));

   ColumnIndexType col = 1;
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

   GET_IFACE2(pBroker,IGetTogaResults,pGetTogaResults);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(5,_T("Input Design Data Vs. Calculated Design Data"));
   *p << p_table;

   int row = 0;
   (*p_table)(row,0) << _T("Value");
   (*p_table)(row,1) << _T("Input")<<rptNewLine<<_T("Design Data");
   (*p_table)(row,2) << _T("Calculated")<<rptNewLine<<_T("Design Data");
   (*p_table)(row,3) << _T("Input/Calculated")<<rptNewLine<<_T("Ratio");
   (*p_table)(row,4) << _T("Status");

   Float64 stress_val, stress_fac, stress_loc;
   pGetTogaResults->GetControllingCompressiveStress(&stress_val, &stress_fac, &stress_loc);

   row++;
   (*p_table)(row,0) << _T("Design Load Compressive Stress, Top CL (")<<stress.GetUnitTag()<<_T(")");
   (*p_table)(row,1) << stress.SetValue( pProjectData->GetFt() );
   (*p_table)(row,2) << stress.SetValue( -1.0 * stress_val );
   (*p_table)(row,3) << stress_fac;

   if(stress_fac >= 1.0)
      (*p_table)(row,4) << color(Green) << _T("Ok") << color(Black);
   else
      (*p_table)(row,4) << color(Red) << _T("Design Deficiency") << color(Black);

   pGetTogaResults->GetControllingTensileStress(&stress_val, &stress_fac, &stress_loc);

   row++;
   (*p_table)(row,0) << _T("Design Load Tensile Stress, Bottom CL (")<<stress.GetUnitTag()<<_T(")");
   (*p_table)(row,1) << stress.SetValue( pProjectData->GetFb() );
   (*p_table)(row,2) << stress.SetValue( -1.0 * stress_val );
   (*p_table)(row,3) << stress_fac;

   if(stress_fac >= 1.0)
      (*p_table)(row,4) << color(Green) << _T("Ok") << color(Black);
   else
      (*p_table)(row,4) << color(Red) << _T("Design Deficiency") << color(Black);

   row++;
   (*p_table)(row,0) << _T("Required Ultimate Moment  (")<<moment.GetUnitTag()<<_T(")");
   (*p_table)(row,1) << moment.SetValue( pProjectData->GetMu() );
   (*p_table)(row,2) << moment.SetValue( pGetTogaResults->GetRequiredUltimateMoment() );
   (*p_table)(row,3) << pProjectData->GetMu() / pGetTogaResults->GetRequiredUltimateMoment();

   if(pProjectData->GetMu() >= pGetTogaResults->GetRequiredUltimateMoment())
      (*p_table)(row,4) << color(Green) << _T("Ok") << color(Black);
   else
      (*p_table)(row,4) << color(Red) << _T("Design Deficiency") << color(Black);

   *p<<Bold(_T("Note:"))<<_T(" Values in the above table reflect the following sign convention: Compressive stress is positive. Tensile stress is negative.");
}

static void optional_results_summary(rptChapter* pChapter,IBroker* pBroker,const CTxDOTOptionalDesignData* pProjectData,IEAFDisplayUnits* pDisplayUnits)
{
   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptPressureUnitValue, stress,      pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,   moment,      pDisplayUnits->GetMomentUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,   length, pDisplayUnits->GetSpanLengthUnit(), false );

   GET_IFACE2(pBroker,IGetTogaResults,pGetTogaResults);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(5,_T("Analysis of Fabricator Optional Design"));
   *p << p_table;

   int row = 0;
   (*p_table)(row,0) << _T("Value");
   (*p_table)(row,1) << _T("Input")<<rptNewLine<<_T("Design Data");
   (*p_table)(row,2) << _T("Analysis of")<<rptNewLine<<_T("Fabricator")<<rptNewLine<<_T("Optional Design");
   (*p_table)(row,3) << _T("Input/Analysis")<<rptNewLine<<_T("Ratio");
   (*p_table)(row,4) << _T("Status");


   Float64 input_fci = pProjectData->GetPrecasterDesignGirderData()->GetFci();
   Float64 reqd_fci = pGetTogaResults->GetRequiredFci();

   row++;
   (*p_table)(row,0) << _T("Required ")<<RPT_FCI<<_T(" (")<<stress.GetUnitTag()<<_T(")");
   (*p_table)(row,1) << stress.SetValue( input_fci );
   (*p_table)(row,2) << stress.SetValue(  reqd_fci );
   (*p_table)(row,3) << input_fci / reqd_fci;

   if(input_fci >= reqd_fci)
      (*p_table)(row,4) << color(Green) << _T("Ok") << color(Black);
   else
      (*p_table)(row,4) << color(Red) << bold(ON) << _T("Beam Does not Satisfy Design Requirements") << bold(OFF) << color(Black);

   Float64 input_fc = pProjectData->GetPrecasterDesignGirderData()->GetFc();
   Float64 reqd_fc = pGetTogaResults->GetRequiredFc();

   row++;
   (*p_table)(row,0) << _T("Required ")<<RPT_FC<<_T(" (")<<stress.GetUnitTag()<<_T(")");
   (*p_table)(row,1) << stress.SetValue( input_fc );
   (*p_table)(row,2) << stress.SetValue(  reqd_fc );
   (*p_table)(row,3) << input_fc / reqd_fc;

   if(input_fc >= reqd_fc)
      (*p_table)(row,4) << color(Green) << _T("Ok") << color(Black);
   else
      (*p_table)(row,4) << color(Red) << bold(ON) << _T("Beam Does not Satisfy Design Requirements") << bold(OFF) << color(Black);

   row++;
   (*p_table)(row,0) << _T("Ultimate Moment Capacity  (")<<moment.GetUnitTag()<<_T(")");
   (*p_table)(row,1) << moment.SetValue( pProjectData->GetMu() );
   (*p_table)(row,2) << moment.SetValue( pGetTogaResults->GetUltimateMomentCapacity() );
   (*p_table)(row,3) << pProjectData->GetMu() / pGetTogaResults->GetUltimateMomentCapacity();

   if(pProjectData->GetMu() <= pGetTogaResults->GetUltimateMomentCapacity())
      (*p_table)(row,4) << color(Green) << _T("Ok") << color(Black);
   else
      (*p_table)(row,4) << color(Red) << bold(ON) << _T("Beam Does not Satisfy Design Requirements") << bold(OFF) << color(Black);
}



static void camber_summary(rptChapter* pChapter,IBroker* pBroker,const CTxDOTOptionalDesignData* pProjectData,IEAFDisplayUnits* pDisplayUnits)
{
   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptLengthUnitValue,   length, pDisplayUnits->GetSpanLengthUnit(), false );

   GET_IFACE2(pBroker,IGetTogaResults,pGetTogaResults);

   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(5,_T("Camber Analysis"));
   *p << p_table;

   int row = 0;
   (*p_table)(row,0) << _T("Value");
   (*p_table)(row,1) << _T("Analysis of")<<rptNewLine<<_T("Original Design");
   (*p_table)(row,2) << _T("Analysis of")<<rptNewLine<<_T("Fabricator")<<rptNewLine<<_T("Optional Design");
   (*p_table)(row,3) << _T("Difference");
   (*p_table)(row,4) << _T("Status");

   row++;
   (*p_table)(row,0) << _T("Maximum Camber  (")<<length.GetUnitTag()<<_T(")");
   (*p_table)(row,1) << length.SetValue( pGetTogaResults->GetMaximumCamber() );
   (*p_table)(row,2) << length.SetValue( pGetTogaResults->GetFabricatorMaximumCamber() );

   Float64 camber_diff = pGetTogaResults->GetFabricatorMaximumCamber() - pGetTogaResults->GetMaximumCamber();

   (*p_table)(row,3) << length.SetValue(camber_diff);

   if(IsZero(camber_diff, ::ConvertToSysUnits( 0.5,unitMeasure::Inch)))
      (*p_table)(row,4) << color(Green) << _T("Ok") << color(Black);
   else
      (*p_table)(row,4) << color(Red) << _T("Design Deficiency") << color(Black);

   *p<<Bold(_T("Note:"))<<_T(" Upward Camber is positive");
}

static void shear_summary(rptChapter* pChapter,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits)
{

   GET_IFACE2(pBroker,IGetTogaResults,pGetTogaResults);

   rptParagraph* p = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << p;

   *p << _T("Shear Design Check");

   p = new rptParagraph;
   *pChapter << p;

   *p << _T("Standard shear reinforcing pattern: ");

   bool passed = pGetTogaResults->ShearPassed();

   if(passed)
      *p << color(Green) << bold(ON) << _T("Ok") << bold(OFF) << color(Black);
   else
      *p << color(Red) << bold(ON) << _T("Beam Does not Satisfy Design Requirements") << bold(OFF) << color(Black);
}