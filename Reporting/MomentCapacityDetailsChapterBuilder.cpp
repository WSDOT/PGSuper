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
                             const std::_tstring& strStageName,
                                 bool bPositiveMoment);

void write_crack_moment_data_table(IBroker* pBroker,
                                   IEAFDisplayUnits* pDisplayUnits,
                                   SpanIndexType span,
                                   GirderIndexType gdr,
                                   const std::vector<pgsPointOfInterest>& pois,
                                   rptChapter* pChapter,
                                   pgsTypes::Stage stage,
                                   const std::_tstring& strStageName,
                                 bool bPositiveMoment);

void write_min_moment_data_table(IBroker* pBroker,
                                 IEAFDisplayUnits* pDisplayUnits,
                                 SpanIndexType span,
                                 GirderIndexType gdr,
                                 const std::vector<pgsPointOfInterest>& pois,
                                 rptChapter* pChapter,
                                 pgsTypes::Stage stage,
                                 const std::_tstring& strStageName,
                                 bool bPositiveMoment);

void write_over_reinforced_moment_data_table(IBroker* pBroker,
                                 IEAFDisplayUnits* pDisplayUnits,
                                 SpanIndexType span,
                                 GirderIndexType gdr,
                                 const std::vector<pgsPointOfInterest>& pois,
                                 rptChapter* pChapter,
                                 pgsTypes::Stage stage,
                                 const std::_tstring& strStageName,
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
            std::_tostringstream os;
            os << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx);
            pPara->SetName( os.str().c_str() );
            (*pPara) << pPara->GetName() << rptNewLine;
         }

         pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         *pChapter << pPara;
         *pPara << _T("Positive Moment Capacity Details") << rptNewLine;
         vPoi = pIPOI->GetPointsOfInterest(spanIdx, gdrIdx, pgsTypes::BridgeSite3, POI_FLEXURECAPACITY | POI_SHEAR, POIFIND_OR );

         write_moment_data_table(pBroker,pDisplayUnits,spanIdx,gdrIdx, vPoi, pChapter, pgsTypes::BridgeSite3, _T("Final with Live Load (Bridge Site 3)"),true);
         if ( !m_bCapacityOnly )
         {
            write_crack_moment_data_table(pBroker,pDisplayUnits,spanIdx, gdrIdx, vPoi, pChapter, pgsTypes::BridgeSite3, _T("Final with Live Load (Bridge Site 3)"),true);
            write_min_moment_data_table(pBroker,pDisplayUnits,spanIdx, gdrIdx, vPoi, pChapter, pgsTypes::BridgeSite3, _T("Final with Live Load (Bridge Site 3)"),true);
            write_over_reinforced_moment_data_table(pBroker,pDisplayUnits,spanIdx, gdrIdx, vPoi, pChapter, pgsTypes::BridgeSite3, _T("Final with Live Load (Bridge Site 3)"),true);
         }

         if ( pBridge->ProcessNegativeMoments(span) )
         {
            pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
            *pChapter << pPara;
            *pPara << _T("Negative Moment Capacity Details") << rptNewLine;

            write_moment_data_table(pBroker,pDisplayUnits,spanIdx, gdrIdx, vPoi, pChapter, pgsTypes::BridgeSite3, _T("Final with Live Load (Bridge Site 3)"),false);
            if ( !m_bCapacityOnly )
            {
               write_crack_moment_data_table(pBroker,pDisplayUnits,spanIdx, gdrIdx, vPoi, pChapter, pgsTypes::BridgeSite3, _T("Final with Live Load (Bridge Site 3)"),false);
               write_min_moment_data_table(pBroker,pDisplayUnits,spanIdx, gdrIdx, vPoi, pChapter, pgsTypes::BridgeSite3, _T("Final with Live Load (Bridge Site 3)"),false);
               write_over_reinforced_moment_data_table(pBroker,pDisplayUnits,spanIdx, gdrIdx, vPoi, pChapter, pgsTypes::BridgeSite3, _T("Final with Live Load (Bridge Site 3)"),false);
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
                             const std::_tstring& strStageName,
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

   std::_tstring strPicture;
   if ( bPositiveMoment )
      strPicture = pFactory->GetPositiveMomentCapacitySchematicImage(deckType);
   else
      strPicture = pFactory->GetNegativeMomentCapacitySchematicImage(deckType);

   *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + strPicture) << rptNewLine;

   *pPara << rptNewLine;

   // Setup the table
   pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;

   std::_tostringstream os;
   os << _T("Moment Capacity [5.7.3.2.4] - ") << strStageName << std::endl;
   ColumnIndexType nColumns = (bPositiveMoment ? 15 : 11);

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(nColumns,os.str());

   *pPara << table << rptNewLine;


   if ( bPositiveMoment )
   {
      rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pPara;
      (*pPara) << _T("* Used to compute ") << Sub2(_T("d"),_T("v")) << _T(" for shear. Depth to resultant tension force for strands in tension. See PCI BDM 8.4.1.2") << rptNewLine;
   }

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

   (*table)(0,col++) << COLHDR(_T("c"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(_T("d") << Sub(_T("c")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(_T("d") << Sub(_T("e")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   
   if ( bPositiveMoment )
      (*table)(0,col++) << COLHDR(_T("d") << Sub(_T("e")) << rptNewLine << _T("(Shear *)"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   (*table)(0,col++) << COLHDR(_T("d") << Sub(_T("t")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   if ( lrfdVersionMgr::SixthEdition2012 <= lrfdVersionMgr::GetVersion() )
   {
      (*table)(0,col++) << Sub2(symbol(epsilon),_T("t"));
   }

   if ( bPositiveMoment )
   {
      (*table)(0,col++) << COLHDR(RPT_STRESS(_T("pe")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,col++) << Sub2(symbol(epsilon),_T("psi")) << rptNewLine << _T("x 1000");
      (*table)(0,col++) << COLHDR(RPT_STRESS(_T("ps,avg")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      if ( lrfdVersionMgr::GetVersion() <= lrfdVersionMgr::FifthEdition2010 )
         (*table)(0,col++) << _T("PPR");
   }
   (*table)(0,col++) << symbol(phi);
   (*table)(0,col++) << COLHDR(_T("Moment") << rptNewLine << _T("Arm"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(_T("C"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*table)(0,col++) << COLHDR(_T("T"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*table)(0,col++) << COLHDR(_T("M") << Sub(_T("n")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

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

      if ( bPositiveMoment )
         (*table)(row,col++) << dim.SetValue( mcd.de_shear );

      (*table)(row,col++) << dim.SetValue( mcd.dt );
      if ( lrfdVersionMgr::SixthEdition2012 <= lrfdVersionMgr::GetVersion() )
      {
         (*table)(row,col++) << strain.SetValue(mcd.et);
      }

      if ( bPositiveMoment )
      {
         (*table)(row,col++) << stress.SetValue( mcd.fpe );
         (*table)(row,col++) << strain.SetValue(mcd.e_initial * 1000);
         (*table)(row,col++) << stress.SetValue( mcd.fps );

         if ( lrfdVersionMgr::GetVersion() <= lrfdVersionMgr::FifthEdition2010 )
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

   pPara = new rptParagraph;
   *pChapter << pPara;

   if ( lrfdVersionMgr::GetVersion() <= lrfdVersionMgr::FifthEdition2010 )
   {
      if ( pSpec->GetMomentCapacityMethod() == WSDOT_METHOD || bAfter2005 )
         *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("GeneralizedFlexureResistanceFactor.png")) << rptNewLine;
   }
   else
   {
      MOMENTCAPACITYDETAILS mcd;
      pMomentCap->GetMomentCapacityDetails(stage,pois.front(),bPositiveMoment,&mcd);
      *pPara << Sub2(symbol(epsilon),_T("cl")) << _T(" = ") << strain.SetValue(mcd.ecl) << rptNewLine;
      *pPara << Sub2(symbol(epsilon),_T("tl")) << _T(" = ") << strain.SetValue(mcd.etl) << rptNewLine;

      if ( bPositiveMoment )
         *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("PositiveMomentFlexureResistanceFactor.png")) << rptNewLine;
      else
         *pPara << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("NegativeMomentFlexureResistanceFactor.png")) << rptNewLine;
   }

}

void write_crack_moment_data_table(IBroker* pBroker,
                                   IEAFDisplayUnits* pDisplayUnits,
                                   SpanIndexType span,
                                   GirderIndexType gdr,
                                   const std::vector<pgsPointOfInterest>& pois,
                                   rptChapter* pChapter,
                                   pgsTypes::Stage stage,
                                   const std::_tstring& strStageName,
                                   bool bPositiveMoment)
{
   bool bAfter2002  = ( lrfdVersionMgr::SecondEditionWith2002Interims < lrfdVersionMgr::GetVersion()     ? true : false );
   bool bBefore2012 = ( lrfdVersionMgr::GetVersion()                  < lrfdVersionMgr::SixthEdition2012 ? true : false );
   // Setup the table
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << (bPositiveMoment ? _T("Positive") : _T("Negative")) << _T(" Cracking Moment Details [5.7.3.3.2] - ") << strStageName << rptNewLine;
  
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   ColumnIndexType nColumns = bAfter2002 ? 8 : 7;
   if ( lrfdVersionMgr::SixthEdition2012 <= lrfdVersionMgr::GetVersion() )
      nColumns--; // No Scfr column for LRFD 6th, 2012 and later

   
   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(nColumns,_T(""));

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

   (*table)(0,1)  << COLHDR( RPT_STRESS(_T("r")), rptPressureUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,2)  << COLHDR( RPT_STRESS(_T("cpe")), rptPressureUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,3)  << COLHDR( Sub2(_T("S"),_T("nc")), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
   (*table)(0,4)  << COLHDR( Sub2(_T("S"),_T("c")), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
   (*table)(0,5)  << COLHDR( Sub2(_T("M"),_T("dnc")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,6)  << COLHDR( Sub2(_T("M"),_T("cr")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   if ( bAfter2002 && bBefore2012 )
      (*table)(0,7)  << COLHDR(Sub2(_T("S"),_T("c")) << RPT_STRESS(_T("r")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptLength3UnitValue, sect_mod, pDisplayUnits->GetSectModulusUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), false );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, fr_coefficient, pDisplayUnits->GetTensionCoefficientUnit(), false );
   
   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(2);
   scalar.SetTolerance(1.0e-6);

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IMomentCapacity,pMomentCapacity);

   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      const pgsPointOfInterest &poi = *i;
      CRACKINGMOMENTDETAILS cmd;
      pMomentCapacity->GetCrackingMomentDetails(stage,poi,bPositiveMoment,&cmd);

      if ( i == pois.begin() )
      {
         if ( lrfdVersionMgr::SixthEdition2012 <= lrfdVersionMgr::GetVersion() )
         {
            *pPara << rptNewLine;
            *pPara << _T("Flexural cracking variability factor, ") << Sub2(symbol(gamma),_T("1")) << _T(" = ") << scalar.SetValue(cmd.g1) << rptNewLine;
            *pPara << _T("Prestress variability factor, ") << Sub2(symbol(gamma),_T("2")) << _T(" = ") << scalar.SetValue(cmd.g2) << rptNewLine;
            *pPara << _T("Ratio of specified minimum yield strength to ultimate tensile strength of the reinforcement," ) << Sub2(symbol(gamma),_T("3")) << _T(" = ") << scalar.SetValue(cmd.g3) << rptNewLine;
            *pPara << rptNewLine;
         }
      }

      (*table)(row,0) << location.SetValue( pgsTypes::BridgeSite3, poi, end_size );
      (*table)(row,1) << stress.SetValue( cmd.fr );
      (*table)(row,2) << stress.SetValue( cmd.fcpe);
      (*table)(row,3) << sect_mod.SetValue( cmd.Sb );
      (*table)(row,4) << sect_mod.SetValue( cmd.Sbc );
      (*table)(row,5) << moment.SetValue( cmd.Mdnc);
      (*table)(row,6) << moment.SetValue( cmd.Mcr );

      if ( bAfter2002 && bBefore2012 )
         (*table)(row,7) << moment.SetValue( cmd.McrLimit );

      row++;
   }

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << pParagraph;

   if ( bBefore2012 )
   {
      if ( bAfter2002 )
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Mcr_2005.png")) << rptNewLine;
      else
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Mcr.png")) << rptNewLine;
   }
   else
   {
      // LRFD 6th Edition, 2012 and later
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Mcr_2012.png")) << rptNewLine;
   }

   GET_IFACE2(pBroker,IBridgeMaterialEx,pMaterial);
   *pParagraph << RPT_STRESS(_T("r")) << _T(" = ") << fr_coefficient.SetValue(pMaterial->GetFlexureFrCoefficient(span,gdr)) << symbol(ROOT) << RPT_FC << rptNewLine;

   *pParagraph << RPT_STRESS(_T("cpe")) << _T(" = compressive stress in concrete due to effective prestress force only (after allowance for all prestress losses) at extreme fiber of section where tensile stress is caused by externally applied loads.") << rptNewLine;
   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("fcpe.png")) << rptNewLine;
   *pParagraph << Sub2(_T("S"),_T("nc")) << _T(" = section modulus for the extreme fiber of the monolithic or noncomposite section where tensile stress is caused by externally applied loads") << rptNewLine;
   *pParagraph << Sub2(_T("S"),_T("c")) << _T(" = section modulus for the extreme fiber of the composite section where tensile stress is caused by externally applied loads") << rptNewLine;
   *pParagraph << Sub2(_T("M"),_T("dnc")) << _T(" = total unfactored dead load moment acting on the monolithic or noncomposite section") << rptNewLine;
   *pParagraph << rptNewLine;
}

void write_min_moment_data_table(IBroker* pBroker,
                                 IEAFDisplayUnits* pDisplayUnits,
                                 SpanIndexType span,
                                 GirderIndexType gdr,
                                 const std::vector<pgsPointOfInterest>& pois,
                                 rptChapter* pChapter,
                                 pgsTypes::Stage stage,
                                 const std::_tstring& strStageName,
                                 bool bPositiveMoment)
{
   // Setup the table
   bool bBefore2012 = ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::SixthEdition2012 ? true : false );

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << _T("Minimum Reinforcement [5.7.3.3.2] - ") << strStageName << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(bBefore2012 ? 7 : 6,_T(""));

   if ( span == ALL_SPANS )
   {
      table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;
   if ( stage == pgsTypes::CastingYard )
      (*table)(0,col++)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,col++)  << COLHDR( _T("M") << Sub(_T("cr")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   if ( bBefore2012 )
   {
      (*table)(0,col++)  << COLHDR( _T("1.2M") << Sub(_T("cr")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   }

   (*table)(0,col++)  << _T("Loading");
   (*table)(0,col++)  << COLHDR( _T("M") << Sub(_T("u")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++)  << COLHDR( _T("1.33M") << Sub(_T("u")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++)  << COLHDR( symbol(phi) << _T("M") << Sub(_T("n")) << _T(" Min"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), false );

   location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   RowIndexType row = table->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IMomentCapacity,pMomentCapacity);

   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      col = 0;
      const pgsPointOfInterest& poi = *i;
      MINMOMENTCAPDETAILS mmcd;
      pMomentCapacity->GetMinMomentCapacityDetails(stage,poi,bPositiveMoment,&mmcd);

      (*table)(row,col++) << location.SetValue( pgsTypes::BridgeSite3, poi, end_size );
      if ( bBefore2012 )
      {
         (*table)(row,col++) << moment.SetValue( mmcd.Mcr );
      }
      (*table)(row,col++) << moment.SetValue( mmcd.MrMin1 );
      (*table)(row,col++) << mmcd.LimitState;
      (*table)(row,col++) << moment.SetValue( mmcd.Mu );
      (*table)(row,col++) << moment.SetValue( mmcd.MrMin2 );
      (*table)(row,col++) << moment.SetValue( mmcd.MrMin );

      row++;
   }

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << pParagraph;
   if ( bBefore2012 )
      *pParagraph << symbol(phi) << Sub2(_T("M"),_T("n")) << _T(" Min = ") << _T("min(") << Sub2(_T("1.2M"),_T("cr")) << _T(", ") << Sub2(_T("1.33M"),_T("u")) << _T(")") << rptNewLine;
   else
      *pParagraph << symbol(phi) << Sub2(_T("M"),_T("n")) << _T(" Min = ") << _T("min(") << Sub2(_T("M"),_T("cr")) << _T(", ") << Sub2(_T("1.33M"),_T("u")) << _T(")") << rptNewLine;
}

void write_over_reinforced_moment_data_table(IBroker* pBroker,
                                 IEAFDisplayUnits* pDisplayUnits,
                                 SpanIndexType span,
                                 GirderIndexType gdr,
                                 const std::vector<pgsPointOfInterest>& pois,
                                 rptChapter* pChapter,
                                 pgsTypes::Stage stage,
                                 const std::_tstring& strStageName,
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
      *pParagraph << _T("Limiting Capacity of Over Reinforced Sections - Positive Moment - ") << strStageName << rptNewLine;
   else
      *pParagraph << _T("Limiting Capacity of Over Reinforced Sections - Negative Moment - ") << strStageName << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;
   *pParagraph << _T("Over reinforced sections may be considered adequate if the flexural demand does not exceed the flexural resistance suggested by LRFD C5.7.3.3.1.") << rptNewLine;
   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("LimitingCapacityOfOverReinforcedSections.jpg")) << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(10,_T("Nominal Resistance of Over Reinforced Sections [C5.7.3.3.1]"));

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

   (*table)(0,1) << symbol(beta) << Sub(_T("1"));
   (*table)(0,2) << COLHDR(_T("f") << Sub(_T("c")), rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*table)(0,3) << COLHDR(_T("b"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,4) << COLHDR(_T("b") << Sub(_T("w")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,5) << COLHDR(_T("d") << Sub(_T("e")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,6) << COLHDR(_T("h") << Sub(_T("f")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,7) << _T("Equation");
   (*table)(0,8) << COLHDR(_T("M") << Sub(_T("n")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,9) << COLHDR(symbol(phi) << _T("M") << Sub(_T("n")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

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
         (*table)(row,7) << (mcd.bRectSection ? _T("C5.7.3.3.1-1") : _T("C5.7.3.3.1-2"));
         (*table)(row,8) << moment.SetValue( mcd.MnMin );
         (*table)(row,9) << moment.SetValue( mcd.Phi * mcd.MnMin );

         row++;
      }
   }
}