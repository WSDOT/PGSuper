///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2021  Washington State Department of Transportation
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
#include <Reporting\StrandEccentricities.h>
#include <Reporting\LongRebarLocations.h>

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\GirderHandling.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\BeamFactory.h>
#include <IFace\Intervals.h>

#include <PsgLib\ConnectionLibraryEntry.h>
#include <PsgLib\ConcreteLibraryEntry.h>
#include <PsgLib\GirderLibraryEntry.h>
#include <PsgLib\TrafficBarrierEntry.h>

#include <PgsExt\BridgeDescription2.h>

#include <Material\PsStrand.h>
#include <Lrfd\RebarPool.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void write_girder_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const CSegmentKey& segmentKey,Uint16 level);
void write_intermedate_diaphragm_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const CSegmentKey& segmentKey,Uint16 level);
void write_deck_width_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const CSegmentKey& segmentKey,Uint16 level);
void write_traffic_barrier_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level,const TrafficBarrierEntry* pBarrierEntry);
void write_strand_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level,const CSegmentKey& segmentKey);
void write_rebar_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const CSegmentKey& segmentKey,Uint16 level);
void write_handling(rptChapter* pChapter,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,const CSegmentKey& segmentKey);
void write_debonding(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits,const CSegmentKey& segmentKey);
void write_camber_factors(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits,const CSegmentKey& segmentKey);


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
   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CGirderLineReportSpecification* pGdrLineRptSpec = dynamic_cast<CGirderLineReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   CGirderKey girderKey;

   if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      girderKey = pGdrRptSpec->GetGirderKey();
   }
   else
   {
      pGdrLineRptSpec->GetBroker(&pBroker);
      girderKey = pGdrLineRptSpec->GetGirderKey();
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IBridge,pBridge);

   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey, &vGirderKeys);
   for(const auto& thisGirderKey : vGirderKeys)
   {
      SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey thisSegmentKey(thisGirderKey,segIdx);

         rptParagraph* pHead = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter<<pHead;
         if ( nSegments == 1 )
         {
            *pHead << _T("Span ") << LABEL_SPAN(thisSegmentKey.groupIndex) << _T(" Girder ") << LABEL_GIRDER(thisSegmentKey.girderIndex) << rptNewLine;
         }
         else
         {
            *pHead << _T("Group ") << LABEL_GROUP(thisSegmentKey.groupIndex) <<  _T(" Girder ") << LABEL_GIRDER(thisSegmentKey.girderIndex) << _T(" Segment ") << LABEL_SEGMENT(thisSegmentKey.segmentIndex) << rptNewLine;
         }

         write_deck_width_details(pBroker, pDisplayUnits, pChapter, thisSegmentKey, level);
         write_intermedate_diaphragm_details(pBroker, pDisplayUnits, pChapter, thisSegmentKey, level);
         write_girder_details( pBroker, pDisplayUnits, pChapter, thisSegmentKey, level);

         write_handling(pChapter,pBroker,pDisplayUnits, thisSegmentKey);

         if ( !m_bOmitStrandLocations )
         {
            CStrandLocations strandLocations;
            strandLocations.Build(pChapter,pBroker, thisSegmentKey,pDisplayUnits);

            CStrandEccentricities strandEccentricities;
            strandEccentricities.Build(pChapter,pBroker, thisSegmentKey,pDisplayUnits);
         }

         write_debonding(pChapter, pBroker, pDisplayUnits, thisSegmentKey);

         write_camber_factors(pChapter, pBroker, pDisplayUnits, thisSegmentKey);

         pHead = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter << pHead;
         *pHead << _T("Transverse Reinforcement Stirrup Zones") << rptNewLine;

         CStirrupTable stirrup_table;
         stirrup_table.Build(pChapter,pBroker, thisSegmentKey,pDisplayUnits);

         CLongRebarLocations long_rebar_table;
         long_rebar_table.Build(pChapter,pBroker, thisSegmentKey,pDisplayUnits);

         pHead = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter << pHead;
         *pHead << _T("Materials") << rptNewLine;
#pragma Reminder("write out concrete details")
         //write_segment_concrete_details(pBroker,pDisplayUnits,pChapter,level,thisSegmentKey);
         if (thisSegmentKey.segmentIndex != nSegments-1 )
         {
            //write_closure_concrete_details(pBroker,pDisplayUnits,pChapter,level,thisSegmentKey);
         }
         write_strand_details( pBroker, pDisplayUnits, pChapter, level, thisSegmentKey);

	      write_rebar_details( pBroker, pDisplayUnits, pChapter, thisSegmentKey, level);
      } // next segment
   } // next girder

   //write_deck_concrete_details(pBroker,pDisplayUnits,pChapter,level);

#pragma Reminder("write out tendon information : geometry and material")

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   write_traffic_barrier_details( pBroker, pDisplayUnits, pChapter, level, pBridgeDesc->GetLeftRailingSystem()->GetExteriorRailing() );

   if ( pBridgeDesc->GetRightRailingSystem()->GetExteriorRailing() != pBridgeDesc->GetLeftRailingSystem()->GetExteriorRailing() )
   {
      write_traffic_barrier_details( pBroker, pDisplayUnits, pChapter, level, pBridgeDesc->GetRightRailingSystem()->GetExteriorRailing());
   }

   const TrafficBarrierEntry* pLftInt = pBridgeDesc->GetLeftRailingSystem()->GetInteriorRailing();
   const TrafficBarrierEntry* pRgtInt = pBridgeDesc->GetRightRailingSystem()->GetInteriorRailing();
   if (nullptr != pLftInt)
   {
      if ( pLftInt != pBridgeDesc->GetLeftRailingSystem()->GetExteriorRailing()  &&
           pLftInt != pBridgeDesc->GetRightRailingSystem()->GetExteriorRailing()  )
      {
         write_traffic_barrier_details( pBroker, pDisplayUnits, pChapter, level, pBridgeDesc->GetLeftRailingSystem()->GetInteriorRailing());
      }
   }

   if (nullptr != pRgtInt)
   {
      if ( pRgtInt != pBridgeDesc->GetLeftRailingSystem()->GetExteriorRailing()  &&
           pRgtInt != pBridgeDesc->GetRightRailingSystem()->GetExteriorRailing() &&
           pRgtInt != pLftInt  )
      {
         write_traffic_barrier_details( pBroker, pDisplayUnits, pChapter, level, pBridgeDesc->GetRightRailingSystem()->GetInteriorRailing());
      }
   }

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

void write_girder_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const CSegmentKey& segmentKey,Uint16 level)
{
#pragma Reminder("UPDATE: need to write out spliced girder taper information")
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const GirderLibraryEntry* pGdrEntry = pGirder->GetGirderLibraryEntry();

   std::_tstring title = std::_tstring(pGirder->GetGirderName()) + std::_tstring(_T(" Dimensions"));
   rptRcTable* pTable = rptStyleManager::CreateTableNoHeading(2,title.c_str());
   pTable->EnableRowStriping(false);
   *pPara << pTable;

   bool bUnitsSI = IS_SI_UNITS(pDisplayUnits);

   CComPtr<IBeamFactory> factory;
   pGdrEntry->GetBeamFactory(&factory);

   (*pTable)(0,0) << rptRcImage( std::_tstring(rptStyleManager::GetImagePath()) + factory->GetImage());

   std::vector<const unitLength*> units = factory->GetDimensionUnits(bUnitsSI);
   GirderLibraryEntry::Dimensions dimensions = pGdrEntry->GetDimensions();
   GirderLibraryEntry::Dimensions::iterator dim_iter;
   std::vector<const unitLength*>::iterator unit_iter;
   for ( dim_iter = dimensions.begin(), unit_iter = units.begin(); 
         dim_iter != dimensions.end() && unit_iter != units.end(); 
         dim_iter++, unit_iter++ )
   {
      const unitLength* pUnit = *unit_iter;
      if ( pUnit == (const unitLength*)BFDIMUNITBOOLEAN)
      {
         (*pTable)(0,1) << (*dim_iter).first.c_str() << ((*dim_iter).second==0 ? _T(" = False") : _T(" = True")) << rptNewLine;
      }
      else if ( pUnit == (const unitLength*)BFDIMUNITSCALAR)
      {
         (*pTable)(0,1) << (*dim_iter).first.c_str() << _T(" = ") << (*dim_iter).second << rptNewLine;
      }
      else
      {
         const unitmgtLengthData& length_unit(pDisplayUnits->GetComponentDimUnit());
         rptFormattedLengthUnitValue cmpdim(pUnit,length_unit.Tol, true, !bUnitsSI, 8, false, rptFormattedLengthUnitValue::RoundOff);
         cmpdim.SetFormat(length_unit.Format);
         cmpdim.SetWidth(length_unit.Width);
         cmpdim.SetPrecision(length_unit.Precision);

         (*pTable)(0,1) << (*dim_iter).first.c_str() << _T(" = ") << cmpdim.SetValue( (*dim_iter).second ) << rptNewLine;
      }
   }

   if (IsTopWidthSpacing(pBridgeDesc->GetGirderSpacingType()))
   {
      const unitmgtLengthData& length_unit(pDisplayUnits->GetComponentDimUnit());
      rptFormattedLengthUnitValue cmpdim(&length_unit.UnitOfMeasure, length_unit.Tol, true, !bUnitsSI, 8, false, rptFormattedLengthUnitValue::RoundOff);
      cmpdim.SetFormat(length_unit.Format);
      cmpdim.SetWidth(length_unit.Width);
      cmpdim.SetPrecision(length_unit.Precision);

#pragma Reminder("reporting top width values (is start and end supported or start only?")
      pgsTypes::TopWidthType type;
      Float64 leftStart, rightStart, leftEnd, rightEnd;
      pGirder->GetTopWidth(&type, &leftStart, &rightStart, &leftEnd, &rightEnd);
      if (type == pgsTypes::twtSymmetric)
      {
         (*pTable)(0, 1) << _T("Top Width = ") << cmpdim.SetValue(leftStart) << rptNewLine;
      }
      else if (type == pgsTypes::twtAsymmetric)
      {
         (*pTable)(0, 1) << _T("Left Overhang = ") << cmpdim.SetValue(leftStart) << rptNewLine;
         (*pTable)(0, 1) << _T("Right Overhang = ") << cmpdim.SetValue(rightStart) << rptNewLine;
      }
      else
      {
         ATLASSERT(type == pgsTypes::twtCenteredCG);
         
         GET_IFACE2(pBroker, IPointOfInterest, pPoi);
         PoiList vPoi;
         pPoi->GetPointsOfInterest(segmentKey, POI_START_FACE,&vPoi);
         ATLASSERT(vPoi.size() == 1);
         pgsPointOfInterest poi(vPoi.front());

         GET_IFACE2(pBroker, IGirder, pIGirder);
         Float64 left, right;
         pIGirder->GetTopWidth(poi, &left, &right);
         (*pTable)(0, 1) << _T("Left Overhang = ") << cmpdim.SetValue(left) << rptNewLine;
         (*pTable)(0, 1) << _T("Right Overhang = ") << cmpdim.SetValue(right) << rptNewLine;
      }
   }

      // Write out strand pattern data and all other data in the girder library entry
#pragma Reminder("Implement")
}

void write_debonding(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits,const CSegmentKey& segmentKey)
{
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   if ( pStrandGeom->CanDebondStrands(segmentKey,pgsTypes::Straight) || 
        pStrandGeom->CanDebondStrands(segmentKey,pgsTypes::Harped)   || 
        pStrandGeom->CanDebondStrands(segmentKey,pgsTypes::Temporary) )
   {
      INIT_UV_PROTOTYPE( rptLengthUnitValue,  cmpdim,  pDisplayUnits->GetSpanLengthUnit(), true );

      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
      const GirderLibraryEntry* pGdrEntry = pGirder->GetGirderLibraryEntry();

      rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;
      *pPara<<_T("Debonding Criteria")<<rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      if (pGdrEntry->CheckMaxTotalFractionDebondedStrands())
      {
         *pPara << _T("Maximum number of debonded strands = ") << pGdrEntry->GetMaxTotalFractionDebondedStrands() * 100 << _T("% of total number of strands") << rptNewLine;
      }
      *pPara << _T("Maximum number of debonded strands per row = ") << pGdrEntry->GetMaxFractionDebondedStrandsPerRow()*100 << _T("% of strands in any row") << rptNewLine;

      StrandIndexType nDebonded10orLess, nDebonded;
      bool bCheckMax;
      Float64 fMax;
      pGdrEntry->GetMaxDebondedStrandsPerSection(&nDebonded10orLess,&nDebonded,&bCheckMax,&fMax);   
      *pPara << _T("Maximum number of debonded strands per section. For 10 or fewer total strands ") << nDebonded10orLess << _T(" otherwise ") << nDebonded << _T(" strands");
      if (bCheckMax)
      {
         *pPara << _T(" but not more than ") << fMax * 100 << _T("% of strands debonded at any section");
      }
      *pPara << rptNewLine;

      Float64 ndb, minDist;
      bool bMinDist;
      pGdrEntry->GetMinDistanceBetweenDebondSections(&ndb, &bMinDist, &minDist);
      *pPara << _T("Longitudinal spacing of debonding termnation sections = ") << ndb << Sub2(_T("d"), _T("b"));
      if (bMinDist)
      {
         *pPara << _T(" but not less than ") << cmpdim.SetValue(minDist) << rptNewLine;
      }
      *pPara << rptNewLine;

      bool useSpanFraction, useHardDistance;
      Float64 spanFraction, hardDistance;
      pGdrEntry->GetMaxDebondedLength(&useSpanFraction, &spanFraction, &useHardDistance, &hardDistance);

      if (useSpanFraction || useHardDistance)
      {
         *pPara << _T("Maximum debonded length is the lesser of: The half-girder length minus maximum development length (") << LrfdCw8th(_T("5.11.4.3)"),_T("5.9.4.3.3")) << _T(")");

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

void write_camber_factors(rptChapter* pChapter,IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits,const CSegmentKey& segmentKey)
{
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const GirderLibraryEntry* pGdrEntry = pGirder->GetGirderLibraryEntry();

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   CamberMultipliers cm = pGdrEntry->GetCamberMultipliers();

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType noncompositeUserLoadIntervalIdx = pIntervals->GetNoncompositeUserLoadInterval();
   IntervalIndexType compositeUserLoadIntervalIdx = pIntervals->GetCompositeUserLoadInterval();

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<<_T("Deflection Multipliers")<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << _T("The factors below are multiplied by the specified load case's deflections when computing deflections and excess camber") << rptNewLine;

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(2);
   *pPara << pTable;

   pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   (*pTable)(0,0) << _T("Load Cases");
   (*pTable)(0,1) << _T("Factor");

   ColumnIndexType row = 1;
   (*pTable)(row,0) << _T("Erection (Girder, Prestress)");
   (*pTable)(row++,1) << scalar.SetValue(cm.ErectionFactor);

   (*pTable)(row,0) << _T("Creep");
   (*pTable)(row++,1) << scalar.SetValue(cm.CreepFactor);

   (*pTable)(row,0) << _T("Diaphragm, Construction, Shear key");
   (*pTable)(row++,1) << scalar.SetValue(cm.DiaphragmFactor);

   (*pTable)(row,0) << _T("Deck Panels");
   (*pTable)(row++,1) << scalar.SetValue(cm.DeckPanelFactor);

   CString strLabel;
   strLabel.Format(_T("Slab, User Defined Loads at %s"),pIntervals->GetDescription(noncompositeUserLoadIntervalIdx));
   (*pTable)(row,0) << strLabel;
   (*pTable)(row++,1) << scalar.SetValue(cm.CreepFactor);

   (*pTable)(row,0) << _T("Haunch");
   (*pTable)(row++,1) << scalar.SetValue(cm.SlabPadLoadFactor);

   strLabel.Format(_T("Railing System (Traffic Barrier, Sidewalks), Overlay, User Defined Loads at %s"),pIntervals->GetDescription(compositeUserLoadIntervalIdx));
   (*pTable)(row,0) << strLabel;
   (*pTable)(row++,1) << scalar.SetValue(cm.BarrierSwOverlayUser2Factor);
}

void write_deck_width_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const CSegmentKey& segmentKey,Uint16 level)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBarriers,pBarriers);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   bool has_sw = pBarriers->HasSidewalk(pgsTypes::tboLeft) || pBarriers->HasSidewalk(pgsTypes::tboRight);
   bool has_overlay = pBridge->HasOverlay();

   ColumnIndexType ncols = 4;
   if (has_sw)
   {
      ncols++;
   }

   if(has_overlay)
   {
      ncols++;
   }
   
   rptRcTable* pTable1 = rptStyleManager::CreateDefaultTable(ncols,_T("Deck and Roadway Widths"));
   *pPara << pTable1;

   ColumnIndexType col = 0;
   (*pTable1)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable1)(0,col++) << _T("Station");
   (*pTable1)(0,col++) << COLHDR(_T("Deck")<<rptNewLine<<_T("Width"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable1)(0,col++) << COLHDR(_T("Exterior")<<rptNewLine<<_T("Curb-Curb")<<rptNewLine<<_T("Width"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   if (has_sw)
   {
      (*pTable1)(0,col++) << COLHDR(_T("Interior")<<rptNewLine<<_T("Curb-Curb")<<rptNewLine<<_T("Width"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   }

   if (has_overlay)
   {
      (*pTable1)(0,col++) << COLHDR(_T("Overlay")<<rptNewLine<<_T("Toe-Toe")<<rptNewLine<<_T("Width"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   }

   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_SPAN, &vPoi);
   RowIndexType row(1);
   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;
      Float64 station, offset;
      pBridge->GetStationAndOffset(poi,&station,&offset);
      Float64 Xb = pPoi->ConvertRouteToBridgeLineCoordinate(station);

      (*pTable1)(row,col++) << location.SetValue( POI_SPAN, poi );
      (*pTable1)(row,col++) << rptRcStation(station, &pDisplayUnits->GetStationFormat() );

       Float64 lft_off = pBridge->GetLeftSlabEdgeOffset(Xb);
       Float64 rgt_off = pBridge->GetRightSlabEdgeOffset(Xb);
      (*pTable1)(row,col++) << dim.SetValue(rgt_off-lft_off);

       Float64 width = pBridge->GetCurbToCurbWidth(Xb);
      (*pTable1)(row,col++) << dim.SetValue(width);

      if (has_sw)
      {
         lft_off = pBridge->GetLeftInteriorCurbOffset(Xb);
         rgt_off = pBridge->GetRightInteriorCurbOffset(Xb);
         (*pTable1)(row,col++) << dim.SetValue(rgt_off-lft_off);
      }

      if (has_overlay)
      {
         lft_off = pBridge->GetLeftOverlayToeOffset(Xb);
         rgt_off = pBridge->GetRightOverlayToeOffset(Xb);
         (*pTable1)(row,col++) << dim.SetValue(rgt_off-lft_off);
      }

      row++;
   }
}

void write_intermedate_diaphragm_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const CSegmentKey& segmentKey,Uint16 level)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cmpdim,  pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  locdim,  pDisplayUnits->GetSpanLengthUnit(), true );
   INIT_UV_PROTOTYPE(rptForcePerLengthUnitValue, fpl, pDisplayUnits->GetForcePerLengthUnit(), true);

   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Intermediate Diaphragms") << rptNewLine;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const GirderLibraryEntry* pGdrEntry = pGirder->GetGirderLibraryEntry();

   const auto& rules = pGdrEntry->GetDiaphragmLayoutRules();

   if ( rules.size() == 0 )
   {
      pParagraph = new rptParagraph();
      *pChapter << pParagraph;
      *pParagraph << _T("No intermediate diaphragms defined") << rptNewLine;
      return;
   }

   for( const auto& rule : rules)
   {
      pParagraph = new rptParagraph();
      *pChapter << pParagraph;

      *pParagraph << _T("Description: ") << rule.Description << rptNewLine;
      
      *pParagraph << _T("Use when span length is between ") << locdim.SetValue(rule.MinSpan);
      *pParagraph << _T(" and ") << locdim.SetValue(rule.MaxSpan) << rptNewLine;

      if (rule.Method == GirderLibraryEntry::dwmCompute)
      {
         *pParagraph << _T("Height: ") << cmpdim.SetValue(rule.Height) << rptNewLine;
         *pParagraph << _T("Thickness: ") << cmpdim.SetValue(rule.Thickness) << rptNewLine;
      }
      else
      {
         *pParagraph << _T("Weight: ") << fpl.SetValue(rule.Weight) << rptNewLine;
      }

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
      {
         *pParagraph << _T("Diaphragm Location: ") << locdim.SetValue(rule.Location) << rptNewLine;
      }
      else
      {
         *pParagraph << _T("Diaphragm Location: ") << rule.Location << Sub2(_T("L"),_T("s")) << rptNewLine;
      }

      *pParagraph << rptNewLine;
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
   rptRcTable* pTable = rptStyleManager::CreateTableNoHeading(2,title.c_str());
   pTable->EnableRowStriping(false);
   *pPara << pTable;

   // schematic
   (*pTable)(0,0) << rptRcImage( std::_tstring(rptStyleManager::GetImagePath()) + _T("ExteriorBarrier.jpg"));

   // Dump barrier points
   (*pTable)(0,1) << _T("Barrier Points") << rptNewLine;
   CComPtr<IPoint2dCollection> points;
   pBarrierEntry->GetBarrierPoints(&points);

   CComPtr<IEnumPoint2d> enum_points;
   points->get__Enum(&enum_points);
   CComPtr<IPoint2d> point;
   long i = 1;
   while ( enum_points->Next(1,&point,nullptr) != S_FALSE )
   {
      Float64 x,y;
      point->get_X(&x);
      point->get_Y(&y);

      (*pTable)(0,1) << _T("Point ") << i++ << _T("= (") << xdim.SetValue(x) << _T(",") << ydim.SetValue(y) << _T(")") << rptNewLine;
      point.Release();
   }

   (*pTable)(0,1) << rptNewLine << rptNewLine;
   (*pTable)(0,1) << _T("Curb Offset = ") << xdim.SetValue( pBarrierEntry->GetCurbOffset() ) << rptNewLine;

   if ( pBarrierEntry->GetWeightMethod() == TrafficBarrierEntry::Compute )
   {
      (*pTable)(0,1) << _T("Weight computed from area of barrier") << rptNewLine;
   }
   else
   {
      (*pTable)(0,1) << _T("Weight = ") << weight.SetValue( pBarrierEntry->GetWeight() ) << _T("/barrier") << rptNewLine;
   }
}

void write_strand_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,Uint16 level,const CSegmentKey& segmentKey)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cmpdim,  pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,  pDisplayUnits->GetStressUnit(),       true );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  modE,    pDisplayUnits->GetModEUnit(),         true );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,    area,    pDisplayUnits->GetAreaUnit(),         true );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker, ISegmentData,pSegmentData);
   GET_IFACE2(pBroker,IStrandGeometry,pStrand);

   rptRcTable* pLayoutTable = rptStyleManager::CreateLayoutTable(2);
   *pPara << pLayoutTable << rptNewLine;

   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::StrandType strandType = (i == 0 ? pgsTypes::Permanent : pgsTypes::Temporary);
      if ( strandType == pgsTypes::Temporary && pStrand->GetMaxStrands(segmentKey,pgsTypes::Temporary) == 0)
      {
         continue;
      }

      std::_tstring strTitle;
      if ( strandType == pgsTypes::Temporary )
      {
         strTitle = _T("Temporary Strands");
      }
      else
      {
         strTitle = _T("Permanent Strands");
      }

      const matPsStrand* pstrand = pSegmentData->GetStrandMaterial(segmentKey,strandType);
      ATLASSERT(pstrand!=0);

      rptRcTable* pTable = rptStyleManager::CreateTableNoHeading(2,strTitle.c_str());
      (*pLayoutTable)(0,i) << pTable;

      RowIndexType row = 0;

      (*pTable)(row,0) << _T("Name");
      (*pTable)(row,1) << pstrand->GetName().c_str();
      row++;

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
}

void write_rebar_details(IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,rptChapter* pChapter,const CSegmentKey& segmentKey,Uint16 level)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cmpdim,  pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,  pDisplayUnits->GetStressUnit(),       true );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  modE,    pDisplayUnits->GetModEUnit(),         true );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,    area,    pDisplayUnits->GetAreaUnit(),         true );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   lrfdRebarPool* pPool = lrfdRebarPool::GetInstance();

   const matRebar* pDeckRebar = nullptr;
   if ( pBridgeDesc->GetDeckDescription()->GetDeckType() != pgsTypes::sdtNone )
   {
      pDeckRebar = pPool->GetRebar(pBridgeDesc->GetDeckDescription()->DeckRebarData.TopRebarType,pBridgeDesc->GetDeckDescription()->DeckRebarData.TopRebarGrade,pBridgeDesc->GetDeckDescription()->DeckRebarData.TopRebarSize);
   }

   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);
   const matRebar* pShearRebar = pPool->GetRebar(pSegment->ShearData.ShearBarType,pSegment->ShearData.ShearBarGrade,matRebar::bs3);
   const matRebar* pLongRebar  = pPool->GetRebar(pSegment->LongitudinalRebarData.BarType,pSegment->LongitudinalRebarData.BarGrade,matRebar::bs3);

   if ( pDeckRebar )
   {
      rptRcTable* pTable = rptStyleManager::CreateTableNoHeading(2,_T("Deck Reinforcing Material"));
      *pPara << pTable << rptNewLine;

      RowIndexType row = 0;

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
      rptRcTable* pTable = rptStyleManager::CreateTableNoHeading(2,_T("Transverse Reinforcing Material (Stirrups, Confinement, and Horizontal Interface Shear Bars)"));
      *pPara << pTable << rptNewLine;

      RowIndexType row = 0;

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
      rptRcTable* pTable = rptStyleManager::CreateTableNoHeading(2,_T("Longitudinal Girder Reinforcing Material"));
      *pPara << pTable << rptNewLine;

      RowIndexType row = 0;

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

void write_handling(rptChapter* pChapter,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,const CSegmentKey& segmentKey)
{
   GET_IFACE2(pBroker,ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
   bool dolift = pSegmentLiftingSpecCriteria->IsLiftingAnalysisEnabled();

   GET_IFACE2(pBroker,ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria);
   bool dohaul = pSegmentHaulingSpecCriteria->IsHaulingAnalysisEnabled();

   if (dolift || dohaul)
   {
      rptParagraph* pHead = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter<<pHead;
      *pHead<<_T("Lifting and Shipping Locations (From End of Girder)")<<rptNewLine;

      INIT_UV_PROTOTYPE( rptLengthUnitValue, loc, pDisplayUnits->GetSpanLengthUnit(), true );

      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;

      if (dolift)
      {
         GET_IFACE2(pBroker,ISegmentLifting,pSegmentLifting);
         *pPara<<_T("Left Lifting Loop  = ")<<loc.SetValue(pSegmentLifting->GetLeftLiftingLoopLocation(segmentKey))<<rptNewLine;
         *pPara<<_T("Right Lifting Loop  = ")<<loc.SetValue(pSegmentLifting->GetRightLiftingLoopLocation(segmentKey))<<rptNewLine;
      }

      if (dohaul)
      {
         GET_IFACE2(pBroker,ISegmentHauling,pSegmentHauling);
         *pPara<<_T("Leading Truck Support = ")<<loc.SetValue(pSegmentHauling->GetLeadingOverhang(segmentKey))<<rptNewLine;
         *pPara<<_T("Trailing Truck Support = ")<<loc.SetValue(pSegmentHauling->GetTrailingOverhang(segmentKey))<<rptNewLine;
      }
   }
}
