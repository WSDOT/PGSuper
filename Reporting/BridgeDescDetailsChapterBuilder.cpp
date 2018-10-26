///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
#include <Reporting\BridgeDescDetailsChapterBuilder.h>
#include <Reporting\StirrupTable.h>
#include <Reporting\StrandLocations.h>
#include <Reporting\LongRebarLocations.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\GirderHandling.h>
#include <IFace\GirderHandlingSpecCriteria.h>

#include <PsgLib\ConnectionLibraryEntry.h>
#include <PsgLib\ConcreteLibraryEntry.h>
#include <PsgLib\GirderLibraryEntry.h>
#include <PsgLib\TrafficBarrierEntry.h>

#include <PgsExt\BridgeDescription.h>
#include <PgsExt\GirderData.h>

#include <Material\PsStrand.h>
#include <Lrfd\RebarPool.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static void write_connection_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level);
static void write_girder_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,SpanIndexType span,GirderIndexType gdr,Uint16 level);
static void write_intermedate_diaphragm_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,SpanIndexType span,GirderIndexType gdr,Uint16 level);
static void write_traffic_barrier_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level,const TrafficBarrierEntry* pBarrierEntry);
static void write_strand_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level,SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType);
static void write_rebar_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level,SpanIndexType span,GirderIndexType gdr);
static void write_concrete_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,SpanIndexType span,GirderIndexType gdr,Uint16 level);
static void write_handling(rptChapter* pChapter,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,SpanIndexType span,GirderIndexType gdr);
void write_debonding(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits, SpanIndexType span,GirderIndexType gdr);

std::_tstring get_connection_image_name(ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType,ConnectionLibraryEntry::EndDistanceMeasurementType endType);


std::_tstring get_bearing_measure_string(ConnectionLibraryEntry::BearingOffsetMeasurementType type)
{
   switch( type )
   {
   case ConnectionLibraryEntry::AlongGirder:
      return _T("Measured From Pier Centerline and Along Girder Centerline");

   case ConnectionLibraryEntry::NormalToPier:
      return _T("Measured From and Normal to Pier Centerline");

   default:
      ATLASSERT(0);
      return _T("");
   }
}

inline std::_tstring get_end_distance_measure_string(ConnectionLibraryEntry::EndDistanceMeasurementType type)
{
   switch( type )
   {
   case ConnectionLibraryEntry::FromBearingAlongGirder:
      return _T("Measured From Bearing along Girder Centerline");

   case ConnectionLibraryEntry::FromBearingNormalToPier:
      return _T("Measured From Bearing and Normal to Pier Centerline");

   case ConnectionLibraryEntry::FromPierAlongGirder:
      return _T("Measured From Pier Centerline and Along Girder Centerline");

   case ConnectionLibraryEntry::FromPierNormalToPier:
      return _T("Measured From and Normal to Pier Centerline");

   default:
      ATLASSERT(0);
      return _T("");
   }
}


/****************************************************************************
CLASS
   CBridgeDescDetailsChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CBridgeDescDetailsChapterBuilder::CBridgeDescDetailsChapterBuilder(bool bOmitStrandLocations,bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
   m_bOmitStrandLocations = bOmitStrandLocations;
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CBridgeDescDetailsChapterBuilder::GetName() const
{
   return TEXT("Bridge Description Details");
}

rptChapter* CBridgeDescDetailsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
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

   GET_IFACE2(pBroker,IStrandGeometry,pStrand);

   write_connection_details( pBroker, pDisplayUnits, pChapter, level);

   GET_IFACE2(pBroker,IBridge,pBridge);
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
         rptParagraph* pHead = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
         *pChapter<<pHead;
         *pHead << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << rptNewLine;

         write_intermedate_diaphragm_details(pBroker, pDisplayUnits, pChapter, spanIdx, gdrIdx, level);
         write_girder_details( pBroker, pDisplayUnits, pChapter, spanIdx, gdrIdx, level);

         write_handling(pChapter,pBroker,pDisplayUnits,spanIdx, gdrIdx);

         if ( !m_bOmitStrandLocations )
         {
            CStrandLocations strand_table;
            strand_table.Build(pChapter,pBroker,spanIdx, gdrIdx,pDisplayUnits);
         }

         write_debonding(pChapter, pBroker, pDisplayUnits, spanIdx, gdrIdx);

         pHead = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
         *pChapter<<pHead;
         *pHead<<_T("Transverse Reinforcement Stirrup Zones")<<rptNewLine;
         CStirrupTable stirrup_table;
         stirrup_table.Build(pChapter,pBroker,spanIdx, gdrIdx,pDisplayUnits);

         CLongRebarLocations long_rebar_table;
         long_rebar_table.Build(pChapter,pBroker,spanIdx, gdrIdx,pDisplayUnits);

         pHead = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
         *pChapter<<pHead;
         *pHead<<_T("Materials")<<rptNewLine;
         write_concrete_details( pBroker, pDisplayUnits, pChapter, spanIdx, gdrIdx, level);

         write_strand_details( pBroker, pDisplayUnits, pChapter, level, spanIdx, gdrIdx, pgsTypes::Permanent );

         if ( 0 < pStrand->GetMaxStrands(spanIdx,gdrIdx,pgsTypes::Temporary) )
            write_strand_details( pBroker, pDisplayUnits, pChapter, level, spanIdx, gdrIdx, pgsTypes::Temporary );

         write_rebar_details( pBroker, pDisplayUnits, pChapter, level, spanIdx, gdrIdx);
      } // gdrIdx
   } // spanIdx
   
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   write_traffic_barrier_details( pBroker, pDisplayUnits, pChapter, level, pBridgeDesc->GetLeftRailingSystem()->GetExteriorRailing() );

   if ( pBridgeDesc->GetRightRailingSystem()->GetExteriorRailing() != pBridgeDesc->GetLeftRailingSystem()->GetExteriorRailing() )
      write_traffic_barrier_details( pBroker, pDisplayUnits, pChapter, level, pBridgeDesc->GetRightRailingSystem()->GetExteriorRailing());

   return pChapter;
}

CChapterBuilder* CBridgeDescDetailsChapterBuilder::Clone() const
{
   return new CBridgeDescDetailsChapterBuilder;
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

void write_connection_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level)
{
   GET_IFACE2(pBroker, ILibrary,   pLib );
   GET_IFACE2(pBroker, IBridge,    pBridge ); 
   INIT_UV_PROTOTYPE( rptLengthUnitValue, xdim,   pDisplayUnits->GetComponentDimUnit(),  true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, cmpdim, pDisplayUnits->GetComponentDimUnit(), true );

   rptParagraph* pHead = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter<<pHead;
   *pHead<<_T("Girder Connection Library Entries")<<rptNewLine;

   rptParagraph* pPara;
   pPara = new rptParagraph;
   *pChapter << pPara;

   std::set<std::_tstring> used_connections;

   PierIndexType cPiers = pBridge->GetPierCount();
   for ( PierIndexType pier = 0; pier < cPiers; pier++ )
   {
      for ( int side = 0; side < 2; side++ )
      {
         std::_tstring strConnection;
         if ( side == 0 )
            strConnection = pBridge->GetLeftSidePierConnection( pier );
         else
            strConnection = pBridge->GetRightSidePierConnection( pier );

         std::set<std::_tstring>::iterator found = used_connections.find( strConnection );
         if ( found == used_connections.end() )
         {
            const ConnectionLibraryEntry* pEntry = pLib->GetConnectionEntry( strConnection.c_str() );

            rptRcTable* pTable = 0;
            pTable = pgsReportStyleHolder::CreateDefaultTable(3,_T(""));
            *pPara << rptNewLine << pTable;

            pTable->SetColumnStyle(1, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
            pTable->SetStripeRowColumnStyle(1, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

            pTable->SetColumnStyle(2, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
            pTable->SetStripeRowColumnStyle(2, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

            // header
            pTable->SetNumberOfHeaderRows(1);
            pTable->SetColumnSpan(0,0,3);
            pTable->SetColumnSpan(0,1,SKIP_CELL);
            pTable->SetColumnSpan(0,2,SKIP_CELL);
            (*pTable)(0,0) << strConnection;

            // picture in first column
            pTable->SetRowSpan(1,0,6);
            pTable->SetRowSpan(2,0,SKIP_CELL);
            pTable->SetRowSpan(3,0,SKIP_CELL);
            pTable->SetRowSpan(4,0,SKIP_CELL);
            pTable->SetRowSpan(5,0,SKIP_CELL);
            pTable->SetRowSpan(6,0,SKIP_CELL);
            (*pTable)(1,0) << rptRcImage( pgsReportStyleHolder::GetImagePath() + get_connection_image_name(pEntry->GetBearingOffsetMeasurementType(),pEntry->GetEndDistanceMeasurementType()) );


            (*pTable)(1,1) << _T("Girder End Distance");
            (*pTable)(1,2) << xdim.SetValue( pEntry->GetGirderEndDistance() ) 
                             << rptNewLine << get_end_distance_measure_string(pEntry->GetEndDistanceMeasurementType());

            (*pTable)(2,1) << _T("Girder Bearing Offset");
            (*pTable)(2,2)   << xdim.SetValue( pEntry->GetGirderBearingOffset() )
                               << rptNewLine << get_bearing_measure_string(pEntry->GetBearingOffsetMeasurementType());

            (*pTable)(3,1) << _T("Support Width");
            (*pTable)(3,2) << xdim.SetValue( pEntry->GetSupportWidth() );

            (*pTable)(4,1) << _T("End Diaphragm Width (W)");
            (*pTable)(4,2) << cmpdim.SetValue( pEntry->GetDiaphragmWidth() );

            (*pTable)(5,1) << _T("End Diaphragm Height (H)");
            (*pTable)(5,2) << cmpdim.SetValue( pEntry->GetDiaphragmHeight() );

            (*pTable)(6,1) << _T("End Diaphragm C.G. Distance");

            ConnectionLibraryEntry::DiaphragmLoadType ltype = pEntry->GetDiaphragmLoadType();
            if (ltype==ConnectionLibraryEntry::ApplyAtBearingCenterline)
               (*pTable)(6,2) << _T("Applied at CL bearing");
            else if(ltype==ConnectionLibraryEntry::ApplyAtSpecifiedLocation)
               (*pTable)(6,2) << xdim.SetValue( pEntry->GetDiaphragmLoadLocation())<<_T(" from CL Pier");
            else if(ltype==ConnectionLibraryEntry::DontApply)
               (*pTable)(6,2) << _T("N/A");
         }

         used_connections.insert( strConnection );
      }
   }
}


void write_girder_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,SpanIndexType span,GirderIndexType gdr,Uint16 level)
{
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);
   const GirderLibraryEntry* pGdrEntry = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdr);

   std::_tstring title = std::_tstring(pSpan->GetGirderTypes()->GetGirderName(gdr)) + std::_tstring(_T(" Dimensions"));
   rptRcTable* pTable = pgsReportStyleHolder::CreateTableNoHeading(1,title);
   pTable->EnableRowStriping(false);
   *pPara << pTable;

   bool bUnitsSI = IS_SI_UNITS(pDisplayUnits);

   CComPtr<IBeamFactory> factory;
   pGdrEntry->GetBeamFactory(&factory);

   std::vector<const unitLength*> units = factory->GetDimensionUnits(bUnitsSI);
   GirderLibraryEntry::Dimensions dimensions = pGdrEntry->GetDimensions();
   GirderLibraryEntry::Dimensions::iterator dim_iter;
   std::vector<const unitLength*>::iterator unit_iter;
   for ( dim_iter = dimensions.begin(), unit_iter = units.begin(); 
         dim_iter != dimensions.end() && unit_iter != units.end(); 
         dim_iter++, unit_iter++ )
   {

      const unitLength* pUnit = *unit_iter;
      if ( pUnit )
      {
         unitmgtLengthData length_unit(pDisplayUnits->GetComponentDimUnit());
         rptFormattedLengthUnitValue cmpdim(pUnit,length_unit.Tol, true, !bUnitsSI, 8, false);
         cmpdim.SetFormat(length_unit.Format);
         cmpdim.SetWidth(length_unit.Width);
         cmpdim.SetPrecision(length_unit.Precision);

         (*pTable)(1,0) << (*dim_iter).first.c_str() << _T(" = ") << cmpdim.SetValue( (*dim_iter).second ) << rptNewLine;
      }
      else
      {
         (*pTable)(1,0) << (*dim_iter).first.c_str() << _T(" = ") << (*dim_iter).second << rptNewLine;
      }
   }


   CComPtr<IBeamFactory> pFactory;
   pGdrEntry->GetBeamFactory(&pFactory);
   (*pTable)(0,0) << rptRcImage( pgsReportStyleHolder::GetImagePath() + pFactory->GetImage());

      // Write out strand pattern data and all other data in the girder library entry
#pragma Reminder("Implement")
}

void write_debonding(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits,SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   if ( pStrandGeom->CanDebondStrands(span,gdr,pgsTypes::Straight) || 
        pStrandGeom->CanDebondStrands(span,gdr,pgsTypes::Harped)   || 
        pStrandGeom->CanDebondStrands(span,gdr,pgsTypes::Temporary) )
   {
      INIT_UV_PROTOTYPE( rptLengthUnitValue,  cmpdim,  pDisplayUnits->GetSpanLengthUnit(), true );

      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      const CSpanData* pSpan = pBridgeDesc->GetSpan(span);
      const GirderLibraryEntry* pGdrEntry = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdr);

      rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;
      *pPara<<_T("Debonding Criteria")<<rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      *pPara << _T("Maximum number of debonded strands = ") << pGdrEntry->GetMaxTotalFractionDebondedStrands()*100 << _T("% of total number of strands") << rptNewLine;
      *pPara << _T("Maximum number of debonded strands per row = ") << pGdrEntry->GetMaxFractionDebondedStrandsPerRow()*100 << _T("% of strands in any row") << rptNewLine;

      StrandIndexType nMax;
      double fMax;

      pGdrEntry->GetMaxDebondedStrandsPerSection(&nMax,&fMax);   
      *pPara << _T("Maximum number of debonded strands per section. The greater of ") << nMax << _T(" strands or ") << fMax*100 << _T("% of strands debonded at any section") << rptNewLine;

      fMax = pGdrEntry->GetMinDebondSectionLength();
      *pPara << _T("Maximum distance between debond sections = ")<<cmpdim.SetValue(fMax)<< rptNewLine;

      bool useSpanFraction, useHardDistance;
      Float64 spanFraction, hardDistance;
      pGdrEntry->GetMaxDebondedLength(&useSpanFraction, &spanFraction, &useHardDistance, &hardDistance);

      if (useSpanFraction || useHardDistance)
      {
         *pPara << _T("Maximum debonded length is the lesser of: The half-girder length minus maximum development length (5.11.4.3)");

         if (useSpanFraction)
         {
            *pPara <<_T("; and ")<<spanFraction*100<<_T("% of the girder length");
         }

         if (useHardDistance)
         {
            *pPara << _T("; and ")<< cmpdim.SetValue(hardDistance) << rptNewLine;
         }
      }
   }
}


void write_intermedate_diaphragm_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,SpanIndexType span,GirderIndexType gdr,Uint16 level)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cmpdim,  pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  locdim,  pDisplayUnits->GetSpanLengthUnit(), true );

   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Intermediate Diaphragms") << rptNewLine;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);
   const GirderLibraryEntry* pGdrEntry = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdr);

   const GirderLibraryEntry::DiaphragmLayoutRules& rules = pGdrEntry->GetDiaphragmLayoutRules();

   if ( rules.size() == 0 )
   {
      pParagraph = new rptParagraph();
      *pChapter << pParagraph;
      *pParagraph << _T("No intermediate diaphragms defined") << rptNewLine;
      return;
   }

   GirderLibraryEntry::DiaphragmLayoutRules::const_iterator iter;
   for ( iter = rules.begin(); iter != rules.end(); iter++ )
   {
      const GirderLibraryEntry::DiaphragmLayoutRule& rule = *iter;

      pParagraph = new rptParagraph();
      *pChapter << pParagraph;

      *pParagraph << _T("Description: ") << rule.Description << rptNewLine;
      
      *pParagraph << _T("Use when span length is between ") << locdim.SetValue(rule.MinSpan);
      *pParagraph << _T(" and ") << locdim.SetValue(rule.MaxSpan) << rptNewLine;

      *pParagraph << _T("Height: ") << cmpdim.SetValue(rule.Height) << rptNewLine;
      *pParagraph << _T("Thickness: ") << cmpdim.SetValue(rule.Thickness) << rptNewLine;

      *pParagraph << _T("Diaphragm Type: ") << (rule.Type == GirderLibraryEntry::dtExternal ? _T("External") : _T("Internal")) << rptNewLine;
      *pParagraph << _T("Construction Stage: ") << (rule.Construction == GirderLibraryEntry::ctCastingYard ? _T("Casting Yard") : _T("Bridge Site")) << rptNewLine;

      switch( rule.MeasureType )
      {
         case GirderLibraryEntry::mtFractionOfSpanLength:
            *pParagraph << _T("Diarphagm location is measured as a fraction of the span length") << rptNewLine;
            break;

         case GirderLibraryEntry::mtFractionOfGirderLength:
            *pParagraph << _T("Diarphagm location is measured as a fraction of the girder length") << rptNewLine;
            break;

         case GirderLibraryEntry::mtAbsoluteDistance:
               *pParagraph << _T("Diarphagm location is measured as a fixed distance") << rptNewLine;
            break;
      }

      switch( rule.MeasureLocation )
      {
         case GirderLibraryEntry::mlEndOfGirder:
            *pParagraph << _T("Diaphragm location is measured from the end of the girder") << rptNewLine;
            break;

         case GirderLibraryEntry::mlBearing:
            *pParagraph << _T("Diaphragm location is measured from the centerline of bearing") << rptNewLine;
            break;

         case GirderLibraryEntry::mlCenterlineOfGirder:
            *pParagraph << _T("Diaphragm location is measured from the centerline of the girder") << rptNewLine;
            break;
      }

      if ( rule.MeasureType == GirderLibraryEntry::mtAbsoluteDistance )
         *pParagraph << _T("Diaphragm Location: ") << locdim.SetValue(rule.Location) << rptNewLine;
      else
         *pParagraph << _T("Diaphragm Location: ") << rule.Location << rptNewLine;
   }

}

void write_traffic_barrier_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level,const TrafficBarrierEntry* pBarrierEntry)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  xdim,  pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  ydim,  pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue,  weight,  pDisplayUnits->GetForcePerLengthUnit(), true );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;


   std::_tstring title(pBarrierEntry->GetName() + _T(" Dimensions"));
   rptRcTable* pTable = pgsReportStyleHolder::CreateTableNoHeading(2,title);
   pTable->EnableRowStriping(false);
   *pPara << pTable;

   // Dump barrier points
   (*pTable)(0,0) << _T("Barrier Points") << rptNewLine;
   CComPtr<IPoint2dCollection> points;
   pBarrierEntry->GetBarrierPoints(&points);

   CComPtr<IEnumPoint2d> enum_points;
   points->get__Enum(&enum_points);
   CComPtr<IPoint2d> point;
   long i = 1;
   while ( enum_points->Next(1,&point,NULL) != S_FALSE )
   {
      double x,y;
      point->get_X(&x);
      point->get_Y(&y);

      (*pTable)(0,0) << _T("Point ") << i++ << _T("= (") << xdim.SetValue(x) << _T(",") << ydim.SetValue(y) << _T(")") << rptNewLine;
      point.Release();
   }

   (*pTable)(0,0) << rptNewLine << rptNewLine;

   if ( pBarrierEntry->GetWeightMethod() == TrafficBarrierEntry::Compute )
   {
      (*pTable)(0,0) << _T("Weight computed from area of barrier") << rptNewLine;
   }
   else
   {
      (*pTable)(0,0) << _T("Weight = ") << weight.SetValue( pBarrierEntry->GetWeight() ) << _T("/barrier") << rptNewLine;
   }

   (*pTable)(0,1) << rptRcImage( pgsReportStyleHolder::GetImagePath() + _T("ExteriorBarrier.jpg"));
}


void write_concrete_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,SpanIndexType span,GirderIndexType gdr,Uint16 level)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cmpdim,  pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,  pDisplayUnits->GetStressUnit(),       true );
   INIT_UV_PROTOTYPE( rptDensityUnitValue, density, pDisplayUnits->GetDensityUnit(),      true );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  modE,    pDisplayUnits->GetModEUnit(),         true );

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool b2005Edition = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::ThirdEditionWith2005Interims ? true : false );

   bool bSIUnits = IS_SI_UNITS(pDisplayUnits);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker, IGirderData,pGirderData);
   const CGirderMaterial* pGirderMaterial = pGirderData->GetGirderMaterial(span,gdr);

   rptRcTable* pTable = pgsReportStyleHolder::CreateTableNoHeading(2,_T("Girder Concrete"));
   *pPara << pTable << rptNewLine;

   Int16 row = 0;
   (*pTable)(row,0) << _T("Type");
   (*pTable)(row,1) << matConcrete::GetTypeName( (matConcrete::Type)pGirderMaterial->Type, true );
   row++;

   (*pTable)(row,0) << RPT_FC;
   (*pTable)(row,1) << stress.SetValue( pGirderMaterial->Fc );
   row++;

   (*pTable)(row,0) << RPT_FCI;
   (*pTable)(row,1) << stress.SetValue( pGirderMaterial->Fci );
   row++;

   (*pTable)(row,0) << _T("Unit Weight ") << Sub2(_T("w"),_T("c"));
   (*pTable)(row,1) << density.SetValue( pGirderMaterial->StrengthDensity );
   row++;

   (*pTable)(row,0) << _T("Unit Weight with Reinforcement ") << Sub2(_T("w"),_T("c"));
   (*pTable)(row,1) << density.SetValue( pGirderMaterial->WeightDensity );
   row++;

   (*pTable)(row,0) << _T("Max Aggregate Size");
   (*pTable)(row,1) << cmpdim.SetValue( pGirderMaterial->MaxAggregateSize );
   row++;

   GET_IFACE2(pBroker,IBridgeMaterial,pMaterial);

   if ( pGirderMaterial->bUserEc )
   {
      (*pTable)(row,0) << _T("User specified value");
   }
   else
   {
      if ( b2005Edition )
      {
         (*pTable)(row,0) << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSIUnits ? _T("ModE_SI_2005.png") : _T("ModE_US_2005.png"))) << rptNewLine;
      }
      else
      {
         (*pTable)(row,0) << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSIUnits ? _T("ModE_SI.png") : _T("ModE_US.png"))) << rptNewLine;
      }
   }
   (*pTable)(row,1) << Sub2(_T("E"),_T("c")) << _T(" = ") << modE.SetValue(pMaterial->GetEcGdr(span,gdr)) << rptNewLine;
   (*pTable)(row,1) << Sub2(_T("E"),_T("ci")) << _T(" = ") << modE.SetValue(pMaterial->GetEciGdr(span,gdr));
   row++;

   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      (*pTable)(row,0) << Sub2(_T("E"),_T("c")) << _T(" ") << Sub2(_T("K"),_T("1"));
      (*pTable)(row,1) << pGirderMaterial->EcK1;
      row++;

      (*pTable)(row,0) << Sub2(_T("E"),_T("c")) << _T(" ") << Sub2(_T("K"),_T("2"));
      (*pTable)(row,1) << pGirderMaterial->EcK2;
      row++;

      (*pTable)(row,0) << _T("Creep") << _T(" ") << Sub2(_T("K"),_T("1"));
      (*pTable)(row,1) << pGirderMaterial->CreepK1;
      row++;

      (*pTable)(row,0) << _T("Creep") << _T(" ") << Sub2(_T("K"),_T("2"));
      (*pTable)(row,1) << pGirderMaterial->CreepK2;
      row++;

      (*pTable)(row,0) << _T("Shrinkage") << _T(" ") << Sub2(_T("K"),_T("1"));
      (*pTable)(row,1) << pGirderMaterial->ShrinkageK1;
      row++;

      (*pTable)(row,0) << _T("Shrinkage") << _T(" ") << Sub2(_T("K"),_T("2"));
      (*pTable)(row,1) << pGirderMaterial->ShrinkageK2;
      row++;
   }

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();
   if ( pBridgeDesc->GetDeckDescription()->DeckType != pgsTypes::sdtNone )
   {
      row = 0;
      pTable = pgsReportStyleHolder::CreateTableNoHeading(2,_T("Deck Concrete"));
      *pPara << pTable << rptNewLine;

      (*pTable)(row,0) << _T("Type");
      (*pTable)(row,1) << matConcrete::GetTypeName( (matConcrete::Type)pDeck->SlabConcreteType, true );
      row++;

      (*pTable)(row,0) << RPT_FC;
      (*pTable)(row,1) << stress.SetValue( pDeck->SlabFc );
      row++;

      (*pTable)(row,0) << _T("Unit Weight ") << Sub2(_T("w"),_T("c"));
      (*pTable)(row,1) << density.SetValue( pDeck->SlabStrengthDensity );
      row++;

      (*pTable)(row,0) << _T("Unit Weight including Reinforcement ") << Sub2(_T("w"),_T("c"));
      (*pTable)(row,1) << density.SetValue( pDeck->SlabWeightDensity );
      row++;

      (*pTable)(row,0) << _T("Max Aggregate Size");
      (*pTable)(row,1) << cmpdim.SetValue( pDeck->SlabMaxAggregateSize );
      row++;

      if ( pDeck->SlabUserEc )
      {
         (*pTable)(row,0) << _T("User specified value");
      }
      else
      {
         if ( b2005Edition )
         {
            (*pTable)(row,0) << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSIUnits ? _T("ModE_SI_2005.png") : _T("ModE_US_2005.png"))) << rptNewLine;
         }
         else
         {
            (*pTable)(row,0) << rptRcImage(pgsReportStyleHolder::GetImagePath() + (bSIUnits ? _T("ModE_SI.png") : _T("ModE_US.png"))) << rptNewLine;
         }
      }
      (*pTable)(row,1) << Sub2(_T("E"),_T("c")) << _T(" = ") << modE.SetValue(pMaterial->GetEcSlab());
      row++;

      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         (*pTable)(row,0) << Sub2(_T("E"),_T("c")) << _T(" ") << Sub2(_T("K"),_T("1"));
         (*pTable)(row,1) << pDeck->SlabEcK1;
         row++;

         (*pTable)(row,0) << Sub2(_T("E"),_T("c")) << _T(" ") << Sub2(_T("K"),_T("2"));
         (*pTable)(row,1) << pDeck->SlabEcK2;
         row++;

         (*pTable)(row,0) << _T("Creep") << _T(" ") << Sub2(_T("K"),_T("1"));
         (*pTable)(row,1) << pDeck->SlabCreepK1;
         row++;

         (*pTable)(row,0) << _T("Creep") << _T(" ") << Sub2(_T("K"),_T("2"));
         (*pTable)(row,1) << pDeck->SlabCreepK2;
         row++;

         (*pTable)(row,0) << _T("Shrinkage") << _T(" ") << Sub2(_T("K"),_T("1"));
         (*pTable)(row,1) << pDeck->SlabShrinkageK1;
         row++;

         (*pTable)(row,0) << _T("Shrinkage") << _T(" ") << Sub2(_T("K"),_T("2"));
         (*pTable)(row,1) << pDeck->SlabShrinkageK2;
         row++;
      }
   }
}

void write_strand_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level,SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cmpdim,  pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,  pDisplayUnits->GetStressUnit(),       true );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  modE,    pDisplayUnits->GetModEUnit(),         true );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,    area,    pDisplayUnits->GetAreaUnit(),         true );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   if ( strandType == pgsTypes::Temporary )
      *pPara << _T("Temporary Strand") << rptNewLine;
   else
      *pPara << _T("Permanent Strand") << rptNewLine;

   GET_IFACE2(pBroker, IGirderData, pGirderData);
   const matPsStrand* pstrand = pGirderData->GetStrandMaterial(span,gdr,strandType);
   CHECK(pstrand!=0);

   rptRcTable* pTable = pgsReportStyleHolder::CreateTableNoHeading(2,pstrand->GetName());
   *pPara << pTable << rptNewLine;

   Int16 row = 0;

   (*pTable)(row,0) << _T("Type");
   (*pTable)(row,1) << (pstrand->GetType() == matPsStrand::LowRelaxation ? _T("Low Relaxation") : _T("Stress Relieved"));
   row++;

   (*pTable)(row,0) << RPT_FPU;
   (*pTable)(row,1) << stress.SetValue( pstrand->GetUltimateStrength() );
   row++;

   (*pTable)(row,0) << RPT_FPY;
   (*pTable)(row,1) << stress.SetValue( pstrand->GetYieldStrength() );
   row++;

   (*pTable)(row,0) << RPT_EPS;
   (*pTable)(row,1) << modE.SetValue( pstrand->GetE() );
   row++;

   (*pTable)(row,0) << RPT_APS;
   (*pTable)(row,1) << area.SetValue( pstrand->GetNominalArea() );
   row++;
}

void write_rebar_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level,SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cmpdim,  pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,  pDisplayUnits->GetStressUnit(),       true );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  modE,    pDisplayUnits->GetModEUnit(),         true );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,    area,    pDisplayUnits->GetAreaUnit(),         true );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   lrfdRebarPool* pPool = lrfdRebarPool::GetInstance();

   const matRebar* pDeckRebar = NULL;
   if ( pBridgeDesc->GetDeckDescription()->DeckType != pgsTypes::sdtNone )
      pDeckRebar = pPool->GetRebar(pBridgeDesc->GetDeckDescription()->DeckRebarData.TopRebarType,pBridgeDesc->GetDeckDescription()->DeckRebarData.TopRebarGrade,matRebar::bs3);

   const CGirderData& girderData = pBridgeDesc->GetSpan(spanIdx)->GetGirderTypes()->GetGirderData(gdrIdx);
   const matRebar* pShearRebar = pPool->GetRebar(girderData.ShearData.ShearBarType,girderData.ShearData.ShearBarGrade,matRebar::bs3);
   const matRebar* pLongRebar  = pPool->GetRebar(girderData.LongitudinalRebarData.BarType,girderData.LongitudinalRebarData.BarGrade,matRebar::bs3);

   if ( pDeckRebar )
   {
      rptRcTable* pTable = pgsReportStyleHolder::CreateTableNoHeading(2,_T("Deck Reinforcing Material"));
      *pPara << pTable << rptNewLine;

      Int16 row = 0;

      (*pTable)(row,0) << _T("Type");
      (*pTable)(row,1) << lrfdRebarPool::GetMaterialName(pDeckRebar->GetType(),pDeckRebar->GetGrade()).c_str();
      row++;

      (*pTable)(row,0) << RPT_FY;
      (*pTable)(row,1) << stress.SetValue( pDeckRebar->GetYieldStrength() );
      row++;

      (*pTable)(row,0) << RPT_FU;
      (*pTable)(row,1) << stress.SetValue( pDeckRebar->GetUltimateStrength() );
   }

   if ( pShearRebar )
   {
      rptRcTable* pTable = pgsReportStyleHolder::CreateTableNoHeading(2,_T("Transverse Reinforcing Material (Stirrups, Confinement, and Horizontal Interface Shear Bars)"));
      *pPara << pTable << rptNewLine;

      Int16 row = 0;

      (*pTable)(row,0) << _T("Type");
      (*pTable)(row,1) << lrfdRebarPool::GetMaterialName(pShearRebar->GetType(),pShearRebar->GetGrade()).c_str();
      row++;

      (*pTable)(row,0) << RPT_FY;
      (*pTable)(row,1) << stress.SetValue( pShearRebar->GetYieldStrength() );
      row++;

      (*pTable)(row,0) << RPT_FU;
      (*pTable)(row,1) << stress.SetValue( pShearRebar->GetUltimateStrength() );
   }

   if ( pLongRebar )
   {
      rptRcTable* pTable = pgsReportStyleHolder::CreateTableNoHeading(2,_T("Longitudinal Girder Reinforcing Material"));
      *pPara << pTable << rptNewLine;

      Int16 row = 0;

      (*pTable)(row,0) << _T("Type");
      (*pTable)(row,1) << lrfdRebarPool::GetMaterialName(pLongRebar->GetType(),pLongRebar->GetGrade()).c_str();
      row++;

      (*pTable)(row,0) << RPT_FY;
      (*pTable)(row,1) << stress.SetValue( pLongRebar->GetYieldStrength() );
      row++;

      (*pTable)(row,0) << RPT_FU;
      (*pTable)(row,1) << stress.SetValue( pLongRebar->GetUltimateStrength() );
   }
}

void write_handling(rptChapter* pChapter,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,SpanIndexType span,GirderIndexType girder)
{
   GET_IFACE2(pBroker,IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   bool dolift = pGirderLiftingSpecCriteria->IsLiftingCheckEnabled();

   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   bool dohaul = pGirderHaulingSpecCriteria->IsHaulingCheckEnabled();

   if (dolift || dohaul)
   {
      rptParagraph* pHead = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter<<pHead;
      *pHead<<_T("Lifting and Shipping Locations (From End of Girder)")<<rptNewLine;

      INIT_UV_PROTOTYPE( rptLengthUnitValue, loc, pDisplayUnits->GetSpanLengthUnit(), true );

      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;

      if (dolift)
      {
         GET_IFACE2(pBroker,IGirderLifting,pGirderLifting);
         *pPara<<_T("Left Lifting Loop  = ")<<loc.SetValue(pGirderLifting->GetLeftLiftingLoopLocation(span,girder))<<rptNewLine;
         *pPara<<_T("Right Lifting Loop  = ")<<loc.SetValue(pGirderLifting->GetRightLiftingLoopLocation(span,girder))<<rptNewLine;
      }

      if (dohaul)
      {
         GET_IFACE2(pBroker,IGirderHauling,pGirderHauling);
         *pPara<<_T("Leading Truck Support = ")<<loc.SetValue(pGirderHauling->GetLeadingOverhang(span,girder))<<rptNewLine;
         *pPara<<_T("Trailing Truck Support = ")<<loc.SetValue(pGirderHauling->GetTrailingOverhang(span,girder))<<rptNewLine;
      }
   }
}


std::_tstring get_connection_image_name(ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType,ConnectionLibraryEntry::EndDistanceMeasurementType endType)
{
   std::_tstring strName;
   if ( brgOffsetType == ConnectionLibraryEntry::AlongGirder )
   {
      switch( endType )
      {
      case ConnectionLibraryEntry::FromBearingAlongGirder:
         strName = _T("Connection_BrgAlongGdr_EndAlongGdrFromBrg.gif");
         break;

      case ConnectionLibraryEntry::FromBearingNormalToPier:
         strName = _T("Connection_BrgAlongGdr_EndAlongNormalFromBrg.gif");
         break;

      case ConnectionLibraryEntry::FromPierAlongGirder:
         strName = _T("Connection_BrgAlongGdr_EndAlongGdrFromPier.gif");
         break;

      case ConnectionLibraryEntry::FromPierNormalToPier:
         strName = _T("Connection_BrgAlongGdr_EndAlongNormalFromPier.gif");
         break;
      }
   }
   else if ( brgOffsetType == ConnectionLibraryEntry::NormalToPier )
   {
      switch( endType )
      {
      case ConnectionLibraryEntry::FromBearingAlongGirder:
         strName = _T("Connection_BrgAlongNormal_EndAlongGdrFromBrg.gif");
         break;

      case ConnectionLibraryEntry::FromBearingNormalToPier:
         strName = _T("Connection_BrgAlongNormal_EndAlongNormalFromBrg.gif");
         break;

      case ConnectionLibraryEntry::FromPierAlongGirder:
         strName = _T("Connection_BrgAlongNormal_EndAlongGdrFromPier.gif");
         break;

      case ConnectionLibraryEntry::FromPierNormalToPier:
         strName = _T("Connection_BrgAlongNormal_EndAlongNormalFromPier.gif");
         break;
      }
   }

   return strName;
}
