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

#include <Reporting\MomentCapacityDetailsChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\BridgeDescription.h>
#include <PgsExt\PointOfInterest.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\MomentCapacity.h>
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CMomentCapacityDetailsChapterBuilder
****************************************************************************/

void write_moment_data_table(IBroker* pBroker,
                             IEAFDisplayUnits* pDisplayUnits,
                             SpanIndexType span,
                             GirderIndexType gdr,
                             const std::vector<pgsPointOfInterest>& pois,
                             rptChapter* pChapter,
                             pgsTypes::Stage stage,
                             const std::string& strStageName,
                                 bool bPositiveMoment);

void write_crack_moment_data_table(IBroker* pBroker,
                                   IEAFDisplayUnits* pDisplayUnits,
                                   SpanIndexType span,
                                   GirderIndexType gdr,
                                   const std::vector<pgsPointOfInterest>& pois,
                                   rptChapter* pChapter,
                                   pgsTypes::Stage stage,
                                   const std::string& strStageName,
                                 bool bPositiveMoment);

void write_min_moment_data_table(IBroker* pBroker,
                                 IEAFDisplayUnits* pDisplayUnits,
                                 SpanIndexType span,
                                 GirderIndexType gdr,
                                 const std::vector<pgsPointOfInterest>& pois,
                                 rptChapter* pChapter,
                                 pgsTypes::Stage stage,
                                 const std::string& strStageName,
                                 bool bPositiveMoment);

void write_over_reinforced_moment_data_table(IBroker* pBroker,
                                 IEAFDisplayUnits* pDisplayUnits,
                                 SpanIndexType span,
                                 GirderIndexType gdr,
                                 const std::vector<pgsPointOfInterest>& pois,
                                 rptChapter* pChapter,
                                 pgsTypes::Stage stage,
                                 const std::string& strStageName,
                                 bool bPositiveMoment);

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CMomentCapacityDetailsChapterBuilder::CMomentCapacityDetailsChapterBuilder(bool bReportCapacityOnly,bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
   m_bCapacityOnly = bReportCapacityOnly;
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CMomentCapacityDetailsChapterBuilder::GetName() const
{
   return TEXT("Moment Capacity Details");
}

rptChapter* CMomentCapacityDetailsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CGirderReportSpecification* pGdrRptSpec    = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   SpanIndexType span;
   GirderIndexType gdr;

   if ( pSGRptSpec )
   {
      pSGRptSpec->GetBroker(&pBroker);
      span = pSGRptSpec->GetSpan();
      gdr = pSGRptSpec->GetGirder();
   }
   else if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      span = ALL_SPANS;
      gdr = pGdrRptSpec->GetGirder();
   }
   else
   {
      span = ALL_SPANS;
      gdr  = ALL_GIRDERS;
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   std::vector<pgsPointOfInterest> vPoi;

// NOTE
// No longer designing/checking for ultimate moment in temporary construction state
// per e-mail from Bijan Khaleghi, dated 4/28/1999.  See project log.
//   vPoi = pIPOI->GetPointsOfInterest(pgsTypes::BridgeSite1, span,girder, POI_FLEXURECAPACITY | POI_TABULAR);
//   write_moment_data_table(pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, pgsTypes::BridgeSite1, "Bridge Site Stage 1");
//   write_crack_moment_data_table(pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, pgsTypes::BridgeSite1, "Bridge Site Stage 1");
//   write_min_moment_data_table(pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, pgsTypes::BridgeSite1, "Bridge Site Stage 1");

   SpanIndexType nSpans = pBridge->GetSpanCount();
   SpanIndexType firstSpanIdx = (span == ALL_SPANS ? 0 : span);
   SpanIndexType lastSpanIdx  = (span == ALL_SPANS ? nSpans : firstSpanIdx+1);
   for ( SpanIndexType spanIdx = firstSpanIdx; spanIdx < lastSpanIdx; spanIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType firstGirderIdx = min(nGirders-1,(gdr == ALL_GIRDERS ? 0 : gdr));
      GirderIndexType lastGirderIdx  = min(nGirders,  (gdr == ALL_GIRDERS ? nGirders : firstGirderIdx + 1));
      for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx < lastGirderIdx; gdrIdx++ )
      {
         rptParagraph* pPara;

         if ( span == ALL_SPANS || gdr == ALL_GIRDERS )
         {
            pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
            *pChapter << pPara;
            std::ostringstream os;
            os << "Span " << LABEL_SPAN(spanIdx) << " Girder " << LABEL_GIRDER(gdrIdx);
            pPara->SetName( os.str().c_str() );
            (*pPara) << pPara->GetName() << rptNewLine;
         }

         pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         *pChapter << pPara;
         *pPara << "Positive Moment Capacity Details" << rptNewLine;
         vPoi = pIPOI->GetPointsOfInterest(spanIdx, gdrIdx, pgsTypes::BridgeSite3, POI_FLEXURECAPACITY | POI_SHEAR, POIFIND_OR );

         write_moment_data_table(pBroker,pDisplayUnits,spanIdx,gdrIdx, vPoi, pChapter, pgsTypes::BridgeSite3, "Final with Live Load (Bridge Site 3)",true);
         if ( !m_bCapacityOnly )
         {
            write_crack_moment_data_table(pBroker,pDisplayUnits,spanIdx, gdrIdx, vPoi, pChapter, pgsTypes::BridgeSite3, "Final with Live Load (Bridge Site 3)",true);
            write_min_moment_data_table(pBroker,pDisplayUnits,spanIdx, gdrIdx, vPoi, pChapter, pgsTypes::BridgeSite3, "Final with Live Load (Bridge Site 3)",true);
            write_over_reinforced_moment_data_table(pBroker,pDisplayUnits,spanIdx, gdrIdx, vPoi, pChapter, pgsTypes::BridgeSite3, "Final with Live Load (Bridge Site 3)",true);
         }

         if ( pBridge->ProcessNegativeMoments(span) )
         {
            pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
            *pChapter << pPara;
            *pPara << "Negative Moment Capacity Details" << rptNewLine;

            write_moment_data_table(pBroker,pDisplayUnits,spanIdx, gdrIdx, vPoi, pChapter, pgsTypes::BridgeSite3, "Final with Live Load (Bridge Site 3)",false);
            if ( !m_bCapacityOnly )
            {
               write_crack_moment_data_table(pBroker,pDisplayUnits,spanIdx, gdrIdx, vPoi, pChapter, pgsTypes::BridgeSite3, "Final with Live Load (Bridge Site 3)",false);
               write_min_moment_data_table(pBroker,pDisplayUnits,spanIdx, gdrIdx, vPoi, pChapter, pgsTypes::BridgeSite3, "Final with Live Load (Bridge Site 3)",false);
               write_over_reinforced_moment_data_table(pBroker,pDisplayUnits,spanIdx, gdrIdx, vPoi, pChapter, pgsTypes::BridgeSite3, "Final with Live Load (Bridge Site 3)",false);
            }
         }
      }
   }

   return pChapter;
}

CChapterBuilder* CMomentCapacityDetailsChapterBuilder::Clone() const
{
   return new CMomentCapacityDetailsChapterBuilder(m_bCapacityOnly);
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

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

void write_moment_data_table(IBroker* pBroker,
                             IEAFDisplayUnits* pDisplayUnits,
                             SpanIndexType span,
                             GirderIndexType gdr,
                             const std::vector<pgsPointOfInterest>& pois,
                             rptChapter* pChapter,
                             pgsTypes::Stage stage,
                             const std::string& strStageName,
                                 bool bPositiveMoment)
{
   rptParagraph* pPara = new rptParagraph();
   *pChapter << pPara;

   GET_IFACE2(pBroker, IBridge,            pBridge);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);

   const GirderLibraryEntry* pGdrEntry = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdr);

   CComPtr<IBeamFactory> pFactory;
   pGdrEntry->GetBeamFactory(&pFactory);

   pgsTypes::SupportedDeckType deckType = pBridgeDesc->GetDeckDescription()->DeckType;

   std::string strPicture;
   if ( bPositiveMoment )
      strPicture = pFactory->GetPositiveMomentCapacitySchematicImage(deckType);
   else
      strPicture = pFactory->GetNegativeMomentCapacitySchematicImage(deckType);

   *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + strPicture) << rptNewLine;

   *pPara << rptNewLine;

   // Setup the table
   pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;

   std::ostringstream os;
   os << "Moment Capacity [5.7.3.2.4] - " << strStageName << std::endl;
   ColumnIndexType nColumns = (bPositiveMoment ? 14 : 10);
   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(nColumns,os.str());

   *pPara << table << rptNewLine;


   if ( span == ALL_SPANS )
   {
      table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   ColumnIndexType col = 0;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,col++)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,col++) << COLHDR("c", rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR("d" << Sub("c"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR("d" << Sub("e"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR("d" << Sub("t"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   if ( bPositiveMoment )
   {
      (*table)(0,col++) << COLHDR(RPT_STRESS("pe"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,col++) << Sub2(symbol(epsilon),"psi") << rptNewLine << "x 1000";
      (*table)(0,col++) << COLHDR(RPT_STRESS("ps,avg"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,col++) << "PPR";
   }
   (*table)(0,col++) << symbol(phi);
   (*table)(0,col++) << COLHDR("Moment" << rptNewLine << "Arm", rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR("C", rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*table)(0,col++) << COLHDR("T", rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*table)(0,col++) << COLHDR("M" << Sub("n"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,    pDisplayUnits->GetGeneralForceUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment,   pDisplayUnits->GetMomentUnit(),       false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),       false );

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   rptRcScalar scalar;
   scalar.SetFormat( pDisplayUnits->GetScalarFormat().Format );
   scalar.SetWidth( pDisplayUnits->GetScalarFormat().Width );
   scalar.SetPrecision( pDisplayUnits->GetScalarFormat().Precision );

   rptRcScalar strain;
   strain.SetFormat( sysNumericFormatTool::Fixed );
   strain.SetWidth(6);
   strain.SetPrecision(3);

   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   Int16 count = 0;
   RowIndexType row = table->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IMomentCapacity,pMomentCap);
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      MOMENTCAPACITYDETAILS mcd;
      pMomentCap->GetMomentCapacityDetails(stage,poi,bPositiveMoment,&mcd);

      col = 0;

      (*table)(row,col++) << location.SetValue( pgsTypes::BridgeSite3, poi, end_size );
      (*table)(row,col++) << dim.SetValue( mcd.c );
      (*table)(row,col++) << dim.SetValue( mcd.dc );
      (*table)(row,col++) << dim.SetValue( mcd.de );
      (*table)(row,col++) << dim.SetValue( mcd.dt );
      if ( bPositiveMoment )
      {
         (*table)(row,col++) << stress.SetValue( mcd.fpe );
         (*table)(row,col++) << strain.SetValue(mcd.e_initial * 1000);
         (*table)(row,col++) << stress.SetValue( mcd.fps );
         (*table)(row,col++) << scalar.SetValue( mcd.PPR );
      }
      (*table)(row,col++) << scalar.SetValue( mcd.Phi );
      (*table)(row,col++) << dim.SetValue( mcd.MomentArm );
      (*table)(row,col++) << force.SetValue( -mcd.C );
      (*table)(row,col++) << force.SetValue( mcd.T );
      (*table)(row,col++) << moment.SetValue( mcd.Mn );


      row++;
      count++;
   }

   GET_IFACE2(pBroker,ISpecification,pSpec);
   GET_IFACE2(pBroker,ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter2005 = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::ThirdEditionWith2006Interims ? true : false );
   if ( pSpec->GetMomentCapacityMethod() == WSDOT_METHOD || bAfter2005 )
      *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + "GeneralizedFlexureResistanceFactor.png") << rptNewLine;

}

void write_crack_moment_data_table(IBroker* pBroker,
                                   IEAFDisplayUnits* pDisplayUnits,
                                   SpanIndexType span,
                                   GirderIndexType gdr,
                                   const std::vector<pgsPointOfInterest>& pois,
                                   rptChapter* pChapter,
                                   pgsTypes::Stage stage,
                                   const std::string& strStageName,
                                 bool bPositiveMoment)
{
   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter2002 = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::SecondEditionWith2003Interims ? true : false );

   // Setup the table
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   ColumnIndexType nColumns = bAfter2002 ? 8 : 7;
   std::ostringstream os;
   os << (bPositiveMoment ? "Positive" : "Negative") << " Cracking Moment Details [5.7.3.3.2] - " << strStageName << std::endl;


   *pParagraph << rptNewLine;
   if ( bAfter2002 )
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Mcr_2005.png") << rptNewLine;
   else
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Mcr.png") << rptNewLine;
   
   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(nColumns,os.str());

   if ( span == ALL_SPANS )
   {
      table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   *pParagraph << table << rptNewLine;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1)  << COLHDR( RPT_STRESS("r"), rptPressureUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,2)  << COLHDR( RPT_STRESS("cpe"), rptPressureUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,3)  << COLHDR( Sub2("S","nc"), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
   (*table)(0,4)  << COLHDR( Sub2("S","c"), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
   (*table)(0,5)  << COLHDR( Sub2("M","dnc"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,6)  << COLHDR( Sub2("M","cr"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   if ( bAfter2002 )
      (*table)(0,7)  << COLHDR(Sub2("S","c") << RPT_STRESS("r"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptLength3UnitValue, sect_mod, pDisplayUnits->GetSectModulusUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), false );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, fr_coefficient, pDisplayUnits->GetTensionCoefficientUnit(), false );

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   Int16 count = 0;
   RowIndexType row = table->GetNumberOfHeaderRows();


   GET_IFACE2(pBroker,IMomentCapacity,pMomentCapacity);

   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      const pgsPointOfInterest &poi = *i;
      CRACKINGMOMENTDETAILS cmd;
      pMomentCapacity->GetCrackingMomentDetails(stage,poi,bPositiveMoment,&cmd);

      (*table)(row,0) << location.SetValue( pgsTypes::BridgeSite3, poi, end_size );
      (*table)(row,1) << stress.SetValue( cmd.fr );
      (*table)(row,2) << stress.SetValue( cmd.fcpe);
      (*table)(row,3) << sect_mod.SetValue( cmd.Sb );
      (*table)(row,4) << sect_mod.SetValue( cmd.Sbc );
      (*table)(row,5) << moment.SetValue( cmd.Mdnc);
      (*table)(row,6) << moment.SetValue( cmd.Mcr );

      if ( bAfter2002 )
         (*table)(row,7) << moment.SetValue( cmd.McrLimit );

      row++;
      count++;
   }

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << pParagraph;

   GET_IFACE2(pBroker,IBridgeMaterialEx,pMaterial);
   *pParagraph << RPT_STRESS("r") << " = " << fr_coefficient.SetValue(pMaterial->GetFlexureFrCoefficient(span,gdr)) << symbol(ROOT) << RPT_FC << rptNewLine;

   *pParagraph << RPT_STRESS("cpe") << " = compressive stress in concrete due to effective prestress force only (after allowance for all prestress losses) at extreme fiber of section where tensile stress is caused by externally applied loads." << rptNewLine;
   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "fcpe.png") << rptNewLine;
   *pParagraph << Sub2("S","nc") << " = section modulus for the extreme fiber of the monolithic or noncomposite section where tensile stress is caused by externally applied loads" << rptNewLine;
   *pParagraph << Sub2("S","c") << " = section modulus for the extreme fiber of the composite section where tensile stress is caused by externally applied loads" << rptNewLine;
   *pParagraph << Sub2("M","dnc") << " = total unfactored dead load moment acting on the monolithic or noncomposite section" << rptNewLine;
   *pParagraph << rptNewLine;
}

void write_min_moment_data_table(IBroker* pBroker,
                                 IEAFDisplayUnits* pDisplayUnits,
                                 SpanIndexType span,
                                 GirderIndexType gdr,
                                 const std::vector<pgsPointOfInterest>& pois,
                                 rptChapter* pChapter,
                                 pgsTypes::Stage stage,
                                 const std::string& strStageName,
                                 bool bPositiveMoment)
{
   // Setup the table
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   std::ostringstream os;
   os << "Minimum Reinforcement [5.7.3.3.2] - " << strStageName << std::endl;
   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(7,os.str());

   if ( span == ALL_SPANS )
   {
      table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   *pParagraph << table << rptNewLine;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1)  << COLHDR( "M" << Sub("cr"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,2)  << COLHDR( "1.2M" << Sub("cr"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,3)  << "Loading";
   (*table)(0,4)  << COLHDR( "M" << Sub("u"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,5)  << COLHDR( "1.33M" << Sub("u"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,6)  << COLHDR( symbol(phi) << "M" << Sub("n") << " Min", rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), false );

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   Int16 count = 0;
   RowIndexType row = table->GetNumberOfHeaderRows();


   GET_IFACE2(pBroker,IMomentCapacity,pMomentCapacity);

   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      MINMOMENTCAPDETAILS mmcd;
      pMomentCapacity->GetMinMomentCapacityDetails(stage,poi,bPositiveMoment,&mmcd);

      (*table)(row,0) << location.SetValue( pgsTypes::BridgeSite3, poi, end_size );
      (*table)(row,1) << moment.SetValue( mmcd.Mcr );
      (*table)(row,2) << moment.SetValue( mmcd.MrMin1 );
      (*table)(row,3) << mmcd.LimitState;
      (*table)(row,4) << moment.SetValue( mmcd.Mu );
      (*table)(row,5) << moment.SetValue( mmcd.MrMin2 );
      (*table)(row,6) << moment.SetValue( mmcd.MrMin );

      row++;
      count++;
   }

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << pParagraph;
   *pParagraph << symbol(phi) << Sub2("M","n") << " Min = " << "min(" << Sub2("1.2M","cr") << ", " << Sub2("1.33M","u") << ")" << rptNewLine;
}

void write_over_reinforced_moment_data_table(IBroker* pBroker,
                                 IEAFDisplayUnits* pDisplayUnits,
                                 SpanIndexType span,
                                 GirderIndexType gdr,
                                 const std::vector<pgsPointOfInterest>& pois,
                                 rptChapter* pChapter,
                                 pgsTypes::Stage stage,
                                 const std::string& strStageName,
                                 bool bPositiveMoment)
{
   // Determine if this table is even needed...
   // It isn't needed if there aren't any over reinforced sections
   bool bTableNeeded = false;
   GET_IFACE2(pBroker,IMomentCapacity,pMomentCap);
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end() && !bTableNeeded; i++)
   {
      const pgsPointOfInterest& poi = *i;
      MOMENTCAPACITYDETAILS mcd;
      pMomentCap->GetMomentCapacityDetails(stage,poi,bPositiveMoment,&mcd);
      if ( mcd.bOverReinforced )
      {
         bTableNeeded = true;
      }
   }

   if ( !bTableNeeded )
      return;

   // If we get here, the table is needed.

   // Setup the table
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   if ( bPositiveMoment )
      *pParagraph << "Limiting Capacity of Over Reinforced Sections - Positive Moment - " << strStageName << rptNewLine;
   else
      *pParagraph << "Limiting Capacity of Over Reinforced Sections - Negative Moment - " << strStageName << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;
   *pParagraph << "Over reinforced sections may be considered adequate if the flexural demand does not exceed the flexural resistance suggested by LRFD C5.7.3.3.1." << rptNewLine;
   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "LimitingCapacityOfOverReinforcedSections.jpg") << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(10,"Nominal Resistance of Over Reinforced Sections [C5.7.3.3.1]");

   if ( span == ALL_SPANS )
   {
      table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   *pParagraph << table << rptNewLine;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1) << symbol(beta) << Sub("1");
   (*table)(0,2) << COLHDR("f" << Sub("c"), rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*table)(0,3) << COLHDR("b", rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,4) << COLHDR("b" << Sub("w"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,5) << COLHDR("d" << Sub("e"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,6) << COLHDR("h" << Sub("f"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,7) << "Equation";
   (*table)(0,8) << COLHDR("M" << Sub("n"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,9) << COLHDR(symbol(phi) << "M" << Sub("n"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment,   pDisplayUnits->GetMomentUnit(),       false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   rptRcScalar scalar;
   scalar.SetFormat( pDisplayUnits->GetScalarFormat().Format );
   scalar.SetWidth( pDisplayUnits->GetScalarFormat().Width );
   scalar.SetPrecision( pDisplayUnits->GetScalarFormat().Precision );

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage


   RowIndexType row = table->GetNumberOfHeaderRows();

   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      MOMENTCAPACITYDETAILS mcd;
      pMomentCap->GetMomentCapacityDetails(stage,poi,bPositiveMoment,&mcd);

      if ( mcd.bOverReinforced )
      {
         (*table)(row,0) << location.SetValue( pgsTypes::BridgeSite3, poi, end_size );
         (*table)(row,1) << scalar.SetValue( mcd.Beta1Slab );
         (*table)(row,2) << stress.SetValue( mcd.FcSlab );
         (*table)(row,3) << dim.SetValue( mcd.b );
         (*table)(row,4) << dim.SetValue( mcd.bw );
         (*table)(row,5) << dim.SetValue( mcd.de );
         (*table)(row,6) << dim.SetValue( mcd.hf );
         (*table)(row,7) << (mcd.bRectSection ? "C5.7.3.3.1-1" : "C5.7.3.3.1-2");
         (*table)(row,8) << moment.SetValue( mcd.MnMin );
         (*table)(row,9) << moment.SetValue( mcd.Phi * mcd.MnMin );

         row++;
      }
   }
}