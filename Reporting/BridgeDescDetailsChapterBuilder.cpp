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

#include "StdAfx.h"
#include <Reporting\BridgeDescDetailsChapterBuilder.h>
#include <Reporting\StirrupTable.h>
#include <Reporting\StrandLocations.h>
#include <Reporting\StrandEccentricities.h>
#include <Reporting\LongRebarLocations.h>

#include <IFace/Tools.h>
#include <EAF/EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\GirderHandling.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\BeamFactory.h>
#include <IFace\Intervals.h>
#include <IFace/PointOfInterest.h>

#include <PsgLib\ConnectionLibraryEntry.h>
#include <PsgLib\ConcreteLibraryEntry.h>
#include <PsgLib\GirderLibraryEntry.h>
#include <PsgLib\TrafficBarrierEntry.h>

#include <PsgLib\BridgeDescription2.h>

#include <Materials/PsStrand.h>
#include <LRFD\RebarPool.h>



void write_segment_details(std::shared_ptr<WBFL::EAF::Broker> pBroker,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,rptChapter* pChapter,const CSegmentKey& segmentKey,Uint16 level);
void write_intermedate_diaphragm_details(std::shared_ptr<WBFL::EAF::Broker> pBroker,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,rptChapter* pChapter,const CGirderKey& girderKey,Uint16 level);
void write_deck_width_details(std::shared_ptr<WBFL::EAF::Broker> pBroker,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,rptChapter* pChapter,const CGirderKey& girderKey,Uint16 level);
void write_traffic_barrier_details(std::shared_ptr<WBFL::EAF::Broker> pBroker,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,rptChapter* pChapter,Uint16 level,const TrafficBarrierEntry* pBarrierEntry);
void write_strand_details(std::shared_ptr<WBFL::EAF::Broker> pBroker,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,rptChapter* pChapter,Uint16 level,const CSegmentKey& segmentKey);
void write_rebar_details(std::shared_ptr<WBFL::EAF::Broker> pBroker,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,rptChapter* pChapter,const CSegmentKey& segmentKey,Uint16 level);
void write_handling(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,const CSegmentKey& segmentKey);
void write_debonding(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,const CSegmentKey& segmentKey);
void write_segment_pt(rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const CSegmentKey& segmentKey);
void write_camber_factors(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,const CSegmentKey& segmentKey);
void write_girder_pt(rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const CGirderKey& girderKey);

void write_linear_tendon_geometry(rptParagraph* pPara, const CDuctData* pDuct, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);
void write_parabolic_tendon_geometry(rptParagraph* pPara, const CDuctData* pDuct, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);



CBridgeDescDetailsChapterBuilder::CBridgeDescDetailsChapterBuilder(bool bOmitStrandLocations,bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
   m_bOmitStrandLocations = bOmitStrandLocations;
}

LPCTSTR CBridgeDescDetailsChapterBuilder::GetName() const
{
   return TEXT("Bridge Description Details");
}

rptChapter* CBridgeDescDetailsChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pGdrRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   auto pGdrLineRptSpec = std::dynamic_pointer_cast<const CGirderLineReportSpecification>(pRptSpec);

   std::shared_ptr<WBFL::EAF::Broker> pBroker;
   CGirderKey girderKey;

   if ( pGdrRptSpec )
   {
      pBroker = pGdrRptSpec->GetBroker();
      girderKey = pGdrRptSpec->GetGirderKey();
   }
   else
   {
      pBroker = pGdrLineRptSpec->GetBroker();
      girderKey = pGdrLineRptSpec->GetGirderKey();
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IBridge,pBridge);

   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey, &vGirderKeys);
   for(const auto& thisGirderKey : vGirderKeys)
   {
      write_deck_width_details(pBroker, pDisplayUnits, pChapter, thisGirderKey, level);
      write_intermedate_diaphragm_details(pBroker, pDisplayUnits, pChapter, thisGirderKey, level);

      SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey thisSegmentKey(thisGirderKey,segIdx);

         rptParagraph* pHead = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter<<pHead;
         std::_tostringstream os;
         if ( nSegments == 1 )
         {
            os << _T("Span ") << LABEL_SPAN(thisSegmentKey.groupIndex) << _T(" Girder ") << LABEL_GIRDER(thisSegmentKey.girderIndex) << std::endl;
         }
         else
         {
            os << _T("Group ") << LABEL_GROUP(thisSegmentKey.groupIndex) << _T(" Girder ") << LABEL_GIRDER(thisSegmentKey.girderIndex) << _T(" Segment ") << LABEL_SEGMENT(thisSegmentKey.segmentIndex) << std::endl;
         }
         pHead->SetName(os.str().c_str());
         *pHead << pHead->GetName() << rptNewLine;


         write_segment_details( pBroker, pDisplayUnits, pChapter, thisSegmentKey, level);

         write_handling(pChapter,pBroker,pDisplayUnits, thisSegmentKey);

         if ( !m_bOmitStrandLocations )
         {
            CStrandLocations strandLocations;
            strandLocations.Build(pChapter,pBroker, thisSegmentKey,pDisplayUnits);

            CStrandEccentricities strandEccentricities;
            strandEccentricities.Build(pChapter,pBroker, thisSegmentKey,pDisplayUnits);
         }

         write_debonding(pChapter, pBroker, pDisplayUnits, thisSegmentKey);

         write_segment_pt(pChapter, pBroker, pDisplayUnits, thisSegmentKey);

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

      write_girder_pt(pChapter, pBroker, pDisplayUnits, thisGirderKey);
   } // next girder

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

void write_segment_details(std::shared_ptr<WBFL::EAF::Broker> pBroker,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,rptChapter* pChapter,const CSegmentKey& segmentKey,Uint16 level)
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

   auto factory = pGdrEntry->GetBeamFactory();

   (*pTable)(0,0) << rptRcImage( std::_tstring(rptStyleManager::GetImagePath()) + factory->GetImage());

   std::vector<const WBFL::Units::Length*> units = factory->GetDimensionUnits(bUnitsSI);
   GirderLibraryEntry::Dimensions dimensions = pGdrEntry->GetDimensions();
   GirderLibraryEntry::Dimensions::iterator dim_iter;
   std::vector<const WBFL::Units::Length*>::iterator unit_iter;
   for ( dim_iter = dimensions.begin(), unit_iter = units.begin(); 
         dim_iter != dimensions.end() && unit_iter != units.end(); 
         dim_iter++, unit_iter++ )
   {
      const WBFL::Units::Length* pUnit = *unit_iter;
      if ( pUnit == (const WBFL::Units::Length*)BFDIMUNITBOOLEAN)
      {
         (*pTable)(0,1) << (*dim_iter).first.c_str() << ((*dim_iter).second==0 ? _T(" = False") : _T(" = True")) << rptNewLine;
      }
      else if ( pUnit == (const WBFL::Units::Length*)BFDIMUNITSCALAR)
      {
         (*pTable)(0,1) << (*dim_iter).first.c_str() << _T(" = ") << (*dim_iter).second << rptNewLine;
      }
      else
      {
         const WBFL::Units::LengthData& length_unit(pDisplayUnits->GetComponentDimUnit());
         rptFormattedLengthUnitValue cmpdim(pUnit,length_unit.Tol, true, !bUnitsSI, 8, false, rptFormattedLengthUnitValue::RoundOff);
         cmpdim.SetFormat(length_unit.Format);
         cmpdim.SetWidth(length_unit.Width);
         cmpdim.SetPrecision(length_unit.Precision);

         (*pTable)(0,1) << (*dim_iter).first.c_str() << _T(" = ") << cmpdim.SetValue( (*dim_iter).second ) << rptNewLine;
      }
   }

   if (IsTopWidthSpacing(pBridgeDesc->GetGirderSpacingType()))
   {
      const WBFL::Units::LengthData& length_unit(pDisplayUnits->GetComponentDimUnit());
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

void write_debonding(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,const CSegmentKey& segmentKey)
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
      *pPara << _T("Longitudinal spacing of debonding termination sections = ") << ndb << Sub2(_T("d"), _T("b"));
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
         *pPara << _T("Maximum debonded length is the lesser of: The half-girder length minus maximum development length (") << WBFL::LRFD::LrfdCw8th(_T("5.11.4.3)"),_T("5.9.4.3.3")) << _T(")");

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

void write_segment_pt(rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const CSegmentKey& segmentKey)
{
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);
   DuctIndexType nDucts = pSegment->Tendons.GetDuctCount();

   if (nDucts == 0) return;

   INIT_UV_PROTOTYPE(rptLengthUnitValue, cmpdim, pDisplayUnits->GetComponentDimUnit(), true);
   INIT_UV_PROTOTYPE(rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), true);

   GET_IFACE2(pBroker, IGirder, pIGirder);
   WebIndexType nWebs = pIGirder->GetWebCount(segmentKey);

   std::array<std::_tstring, 2> strDuctGeometryType{ _T("Straight"),_T("Parabolic") };
   std::array<std::_tstring, 3> strJackingEnd{ _T("Left"), _T("Right"), _T("Both") };
   std::array<std::_tstring, 4> strDuctType{ _T("Galvanized ferrous metal"),_T("Polyethylene"), _T("Formed in concrete with removable cores"), _T("Corrugated Polymer") };
   std::array<std::_tstring, 2> strInstallationMethod{ _T("Push"),_T("Pull") };
   std::array<std::_tstring, 2> strGirderFace{ _T("Top"),_T("Bottom") };

   for (DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++)
   {
      const auto* pDuct = pSegment->Tendons.GetDuct(ductIdx);;

      rptParagraph* pHead = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pHead;

      if (nWebs == 1)
      {
         *pHead << _T("Duct ") << LABEL_DUCT(ductIdx) << rptNewLine;
      }
      else
      {
         DuctIndexType firstDuctIdx = nWebs*ductIdx;
         DuctIndexType lastDuctIdx = firstDuctIdx + nWebs - 1;
         *pHead << _T("Duct ") << LABEL_DUCT(firstDuctIdx) << _T(" - ") << LABEL_DUCT(lastDuctIdx) << rptNewLine;
      }

      rptParagraph* pPara = new rptParagraph();
      *pChapter << pPara;

      switch (pSegment->Tendons.InstallationEvent)
      {
      case pgsTypes::sptetRelease:
         *pPara << _T("Tendons are post-tensioned immediately after release") << rptNewLine;
         break;
      case pgsTypes::sptetStorage:
         *pPara << _T("Tendons are post-tensioned immediately at the beginning of storage") << rptNewLine;
         break;
      case pgsTypes::sptetHauling:
         *pPara << _T("Tendons are post-tensioned immediately before hauling") << rptNewLine;
         break;
      default:
         ATLASSERT(false); // is there a new option?
      }

      *pPara << _T("Type : ") << pDuct->Name << rptNewLine;
      *pPara << _T("Shape : ") << strDuctGeometryType[pDuct->DuctGeometryType] << rptNewLine;
      *pPara << _T("Left End : Y = ") << cmpdim.SetValue(pDuct->DuctPoint[CSegmentDuctData::Left].first) << _T(" from ") << strGirderFace[pDuct->DuctPoint[CSegmentDuctData::Left].second] << _T(" face") << rptNewLine;
      if (pDuct->DuctGeometryType == CSegmentDuctData::Parabolic)
      {
         *pPara << _T("Middle : Y = ") << cmpdim.SetValue(pDuct->DuctPoint[CSegmentDuctData::Middle].first) << _T(" from ") << strGirderFace[pDuct->DuctPoint[CSegmentDuctData::Middle].second] << _T(" face") << rptNewLine;
      }
      *pPara << _T("Right End : Y = ") << cmpdim.SetValue(pDuct->DuctPoint[CSegmentDuctData::Right].first) << _T(" from ") << strGirderFace[pDuct->DuctPoint[CSegmentDuctData::Right].second] << _T(" face") << rptNewLine;
      *pPara << _T("Number of Strands : ") << pDuct->nStrands << rptNewLine;
      *pPara << Sub2(_T("P"), _T("jack")) << _T(" : ") << force.SetValue(pDuct->Pj) << rptNewLine;
      *pPara << _T("Jacking End : ") << strJackingEnd[pDuct->JackingEnd] << rptNewLine;
      *pPara << _T("Duct Material : ") << strDuctType[pSegment->Tendons.DuctType] << rptNewLine;
      *pPara << _T("Installation Method : ") << strInstallationMethod[pSegment->Tendons.InstallationType] << rptNewLine;
   }
}

void write_camber_factors(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,const CSegmentKey& segmentKey)
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
   strLabel.Format(_T("Slab, User Defined Loads at %s"),pIntervals->GetDescription(noncompositeUserLoadIntervalIdx).c_str());
   (*pTable)(row,0) << strLabel;
   (*pTable)(row++,1) << scalar.SetValue(cm.CreepFactor);

   (*pTable)(row,0) << _T("Haunch");
   (*pTable)(row++,1) << scalar.SetValue(cm.SlabPadLoadFactor);

   strLabel.Format(_T("Railing System (Traffic Barrier, Sidewalks), Overlay, User Defined Loads at %s"),pIntervals->GetDescription(compositeUserLoadIntervalIdx).c_str());
   (*pTable)(row,0) << strLabel;
   (*pTable)(row++,1) << scalar.SetValue(cm.BarrierSwOverlayUser2Factor);
}

void write_deck_width_details(std::shared_ptr<WBFL::EAF::Broker> pBroker,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,rptChapter* pChapter,const CGirderKey& girderKey,Uint16 level)
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
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CSegmentKey segmentKey(girderKey, segIdx);
      PoiList vSegmentPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_SPAN, &vSegmentPoi);
      pPoi->MergePoiLists(vPoi, vSegmentPoi, &vPoi);
   }
   pPoi->SortPoiList(&vPoi);

   RowIndexType row = pTable1->GetNumberOfHeaderRows();
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

void write_intermedate_diaphragm_details(std::shared_ptr<WBFL::EAF::Broker> pBroker,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,rptChapter* pChapter,const CGirderKey& girderKey,Uint16 level)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cmpdim,  pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  locdim,  pDisplayUnits->GetSpanLengthUnit(), true );
   INIT_UV_PROTOTYPE(rptForcePerLengthUnitValue, fpl, pDisplayUnits->GetForcePerLengthUnit(), true);

   rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Intermediate Diaphragms") << rptNewLine;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);
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
            *pParagraph << _T("Diaphragm location is measured as a fraction of the span length") << rptNewLine;
            break;

         case GirderLibraryEntry::mtFractionOfGirderLength:
            *pParagraph << _T("Diaphragm location is measured as a fraction of the girder length") << rptNewLine;
            break;

         case GirderLibraryEntry::mtAbsoluteDistance:
               *pParagraph << _T("Diaphragm location is measured as a fixed distance") << rptNewLine;
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

void write_traffic_barrier_details(std::shared_ptr<WBFL::EAF::Broker> pBroker,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,rptChapter* pChapter,Uint16 level,const TrafficBarrierEntry* pBarrierEntry)
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
   IndexType i = 1;
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

rptRcTable* write_strand_material(LPCTSTR strStrandType, const WBFL::Materials::PsStrand* pStrand, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits)
{
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true);
   INIT_UV_PROTOTYPE(rptStressUnitValue, modE, pDisplayUnits->GetModEUnit(), true);
   INIT_UV_PROTOTYPE(rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), true);

   rptRcTable* pTable = rptStyleManager::CreateTableNoHeading(2, strStrandType);

   RowIndexType row = 0;

   (*pTable)(row, 0) << _T("Name");
   (*pTable)(row, 1) << pStrand->GetName().c_str();
   row++;

   (*pTable)(row, 0) << _T("Type");
   (*pTable)(row, 1) << WBFL::Materials::PsStrand::GetType(pStrand->GetType());
   row++;

   (*pTable)(row, 0) << RPT_FPU;
   (*pTable)(row, 1) << stress.SetValue(pStrand->GetUltimateStrength());
   row++;

   (*pTable)(row, 0) << RPT_FPY;
   (*pTable)(row, 1) << stress.SetValue(pStrand->GetYieldStrength());
   row++;

   (*pTable)(row, 0) << RPT_EPS;
   (*pTable)(row, 1) << modE.SetValue(pStrand->GetE());
   row++;

   (*pTable)(row, 0) << RPT_APS;
   (*pTable)(row, 1) << area.SetValue(pStrand->GetNominalArea());
   row++;

   return pTable;
}

void write_strand_details(std::shared_ptr<WBFL::EAF::Broker> pBroker,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,rptChapter* pChapter,Uint16 level,const CSegmentKey& segmentKey)
{
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker, ISegmentData,pSegmentData);
   GET_IFACE2(pBroker,IStrandGeometry,pStrand);
   GET_IFACE2(pBroker, ISegmentTendonGeometry, pTendon);

   StrandIndexType Nt = pStrand->GetMaxStrands(segmentKey, pgsTypes::Temporary);
   DuctIndexType nDucts = pTendon->GetDuctCount(segmentKey);

   rptRcTable* pLayoutTable = rptStyleManager::CreateLayoutTable((Nt == 0 ? 2 : 3) + (nDucts == 0 ? 0 : 1));
   *pPara << pLayoutTable << rptNewLine;

   std::array<std::_tstring, 3> strStrandType{ _T("Straight"),_T("Harped"),_T("Temporary") };

   ColumnIndexType col;
   for ( col = 0; col < (Nt == 0 ? 2 : 3); col++ )
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)col;

      const auto* pStrand = pSegmentData->GetStrandMaterial(segmentKey,strandType);
      ATLASSERT(pStrand != nullptr);

      rptRcTable* pTable = write_strand_material(strStrandType[strandType].c_str(), pStrand, pDisplayUnits);
      (*pLayoutTable)(0,col) << pTable;
   }

   if (0 < nDucts)
   {
      const auto* pStrand = pSegmentData->GetSegmentPTData(segmentKey)->m_pStrand;
      ATLASSERT(pStrand != nullptr);

      rptRcTable* pTable = write_strand_material(_T("Tendons"), pStrand, pDisplayUnits);
      (*pLayoutTable)(0, col) << pTable;
   }
}

void write_rebar_details(std::shared_ptr<WBFL::EAF::Broker> pBroker,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,rptChapter* pChapter,const CSegmentKey& segmentKey,Uint16 level)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cmpdim,  pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,  pDisplayUnits->GetStressUnit(),       true );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  modE,    pDisplayUnits->GetModEUnit(),         true );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,    area,    pDisplayUnits->GetAreaUnit(),         true );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const auto* pPool = WBFL::LRFD::RebarPool::GetInstance();

   const WBFL::Materials::Rebar* pDeckRebar = nullptr;
   if ( pBridgeDesc->GetDeckDescription()->GetDeckType() != pgsTypes::sdtNone )
   {
      pDeckRebar = pPool->GetRebar(pBridgeDesc->GetDeckDescription()->DeckRebarData.TopRebarType,pBridgeDesc->GetDeckDescription()->DeckRebarData.TopRebarGrade,pBridgeDesc->GetDeckDescription()->DeckRebarData.TopRebarSize);
   }

   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);
   const auto* pShearRebar = pPool->GetRebar(pSegment->ShearData.ShearBarType,pSegment->ShearData.ShearBarGrade,WBFL::Materials::Rebar::Size::bs3);
   const auto* pLongRebar  = pPool->GetRebar(pSegment->LongitudinalRebarData.BarType,pSegment->LongitudinalRebarData.BarGrade,WBFL::Materials::Rebar::Size::bs3);

   if ( pShearRebar )
   {
      rptRcTable* pTable = rptStyleManager::CreateTableNoHeading(2,_T("Transverse Reinforcing Material (Stirrups, Confinement, and Horizontal Interface Shear Bars)"));
      *pPara << pTable << rptNewLine;

      RowIndexType row = 0;

      (*pTable)(row,0) << _T("Type");
      (*pTable)(row,1) << WBFL::LRFD::RebarPool::GetMaterialName(pShearRebar->GetType(),pShearRebar->GetGrade()).c_str();
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
      (*pTable)(row,1) << WBFL::LRFD::RebarPool::GetMaterialName(pLongRebar->GetType(),pLongRebar->GetGrade()).c_str();
      row++;

      (*pTable)(row,0) << RPT_FY;
      (*pTable)(row,1) << stress.SetValue( pLongRebar->GetYieldStrength() );
      row++;

      (*pTable)(row,0) << RPT_FU;
      (*pTable)(row,1) << stress.SetValue( pLongRebar->GetUltimateStrength() );
   }

   if (pDeckRebar)
   {
      rptRcTable* pTable = rptStyleManager::CreateTableNoHeading(2, _T("Deck Reinforcing Material"));
      *pPara << pTable << rptNewLine;

      RowIndexType row = 0;

      (*pTable)(row, 0) << _T("Type");
      (*pTable)(row, 1) << WBFL::LRFD::RebarPool::GetMaterialName(pDeckRebar->GetType(), pDeckRebar->GetGrade()).c_str();
      row++;

      (*pTable)(row, 0) << RPT_FY;
      (*pTable)(row, 1) << stress.SetValue(pDeckRebar->GetYieldStrength());
      row++;

      (*pTable)(row, 0) << RPT_FU;
      (*pTable)(row, 1) << stress.SetValue(pDeckRebar->GetUltimateStrength());
   }
}

void write_handling(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,const CSegmentKey& segmentKey)
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

void write_girder_pt(rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const CGirderKey& girderKey)
{
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CSplicedGirderData* pGirder = pIBridgeDesc->GetGirder(girderKey);
   const CPTData* pPT = pGirder->GetPostTensioning();
   DuctIndexType nDucts = pPT->GetDuctCount();
   if (nDucts == 0) return;

   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   GET_IFACE2(pBroker, IGirder, pIGirder);
   WebIndexType nWebs = pIGirder->GetWebCount(girderKey);

   INIT_UV_PROTOTYPE(rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), true);

   std::array<std::_tstring, 3> strDuctGeometryType{ _T("Linear"),_T("Parabolic"),_T("Offset") };
   std::array<std::_tstring, 3> strJackingEnd{ _T("Left"), _T("Right"), _T("Both") };
   std::array<std::_tstring, 4> strDuctType{ _T("Galvanized ferrous metal"),_T("Polyethylene"), _T("Formed in concrete with removable cores"), _T("Corrugated Polymer") };
   std::array<std::_tstring, 2> strInstallationMethod{ _T("Push"),_T("Pull") };

   rptParagraph* pHead = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pHead;
   *pHead << _T("Field Installed Tendons") << rptNewLine;
   for (DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++)
   {
      const CDuctData* pDuct = pPT->GetDuct(ductIdx);

      pHead = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pHead;
      if (nWebs == 1)
      {
         *pHead << _T("Duct ") << LABEL_DUCT(ductIdx) << rptNewLine;
      }
      else
      {
         DuctIndexType firstDuctIdx = nWebs*ductIdx;
         DuctIndexType lastDuctIdx = firstDuctIdx + nWebs - 1;
         *pHead << _T("Duct ") << LABEL_DUCT(firstDuctIdx) << _T(" - ") << LABEL_DUCT(lastDuctIdx) << rptNewLine;
      }

      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;

      *pPara << _T("Type : ") << pDuct->Name << rptNewLine;
      *pPara << _T("Shape : ") << strDuctGeometryType[pDuct->DuctGeometryType] << rptNewLine;
      switch (pDuct->DuctGeometryType)
      {
      case CDuctGeometry::Linear:
         write_linear_tendon_geometry(pPara, pDuct, pDisplayUnits);
         break;
      case CDuctGeometry::Parabolic:
         write_parabolic_tendon_geometry(pPara, pDuct, pDisplayUnits);
         break;
      case CDuctGeometry::Offset: // drop through because this isn't supported yet
      default:
         ATLASSERT(false);
      }
      *pPara << _T("Number of Strands : ") << pDuct->nStrands << rptNewLine;
      *pPara << Sub2(_T("P"), _T("jack")) << _T(" : ") << force.SetValue(pDuct->Pj) << rptNewLine;
      *pPara << _T("Jacking End : ") << strJackingEnd[pDuct->JackingEnd] << rptNewLine;
      *pPara << _T("Duct Material : ") << strDuctType[pPT->DuctType] << rptNewLine;
      *pPara << _T("Tendon Material : ") << pPT->pStrand->GetName() << rptNewLine;
      *pPara << _T("Installation Method : ") << strInstallationMethod[pPT->InstallationType] << rptNewLine;

      EventIndexType eventIdx = pTimelineMgr->GetStressTendonEventIndex(pGirder->GetID(), ductIdx);
      *pPara << _T("Installation : Event ") << LABEL_EVENT(eventIdx) << _T(" - ") << pTimelineMgr->GetEventByIndex(eventIdx)->GetDescription();
   }
}

void write_linear_tendon_geometry(rptParagraph* pPara, const CDuctData* pDuct, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits)
{
   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim, pDisplayUnits->GetSpanLengthUnit(), true);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, cmpdim, pDisplayUnits->GetComponentDimUnit(), false);
   INIT_SCALAR_PROTOTYPE(rptRcPercentage, percentage, pDisplayUnits->GetPercentageFormat());

   std::array<std::_tstring, 2> strOffsetType{ _T("Bottom Girder"),_T("Top Girder") };

   if (pDuct->LinearDuctGeometry.GetMeasurementType() == CLinearDuctGeometry::AlongGirder)
   {
      *pPara << _T("Duct locations are measured from the left end of the girder.") << rptNewLine;
   }
   else
   {
      ATLASSERT(pDuct->LinearDuctGeometry.GetMeasurementType() == CLinearDuctGeometry::FromPrevious);
      *pPara << _T("Duct locations are measured from the left end of the previous point.") << rptNewLine;
   }

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(4, _T(""));
   *pPara << pTable;

   (*pTable)(0, 0) << _T("Point");
   (*pTable)(0, 1) << _T("Location");
   pTable->SetColumnSpan(0, 2, 2);
   (*pTable)(0, 2) << COLHDR(_T("Offset"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   RowIndexType row = pTable->GetNumberOfHeaderRows();
   IndexType nPoints = pDuct->LinearDuctGeometry.GetPointCount();
   for (IndexType pointIdx = 0; pointIdx < nPoints; pointIdx++, row++)
   {
      Float64 location, offset;
      CDuctGeometry::OffsetType offsetType;
      pDuct->LinearDuctGeometry.GetPoint(pointIdx, &location, &offset, &offsetType);
      (*pTable)(row, 0) << LABEL_INDEX(pointIdx);
      if (location < 0)
      {
         (*pTable)(row, 1) << percentage.SetValue(-location);
      }
      else
      {
         (*pTable)(row, 1) << dim.SetValue(location);
      }
      (*pTable)(row, 2) << cmpdim.SetValue(offset);
      (*pTable)(row, 3) << strOffsetType[offsetType];
   }
}

void write_parabolic_tendon_geometry(rptParagraph* pPara, const CDuctData* pDuct, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits)
{
   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim, pDisplayUnits->GetSpanLengthUnit(), true);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, cmpdim, pDisplayUnits->GetComponentDimUnit(), false);
   INIT_SCALAR_PROTOTYPE(rptRcPercentage, percentage, pDisplayUnits->GetPercentageFormat());

   PierIndexType startPierIdx;
   Float64 startDist;
   Float64 startOffset;
   CDuctGeometry::OffsetType startOffsetType;
   pDuct->ParabolicDuctGeometry.GetStartPoint(&startPierIdx, &startDist, &startOffset, &startOffsetType);

   PierIndexType endPierIdx;
   Float64 endDist;
   Float64 endOffset;
   CDuctGeometry::OffsetType endOffsetType;
   pDuct->ParabolicDuctGeometry.GetEndPoint(&endPierIdx, &endDist, &endOffset, &endOffsetType);

   *pPara << _T("Duct starts at ") << LABEL_PIER_EX(pDuct->GetGirder()->GetGirderGroup()->GetBridgeDescription()->GetPier(startPierIdx)->IsAbutment(),startPierIdx);
   *pPara << _T(" and ends at ") << LABEL_PIER_EX(pDuct->GetGirder()->GetGirderGroup()->GetBridgeDescription()->GetPier(endPierIdx)->IsAbutment(), endPierIdx);

   std::array<std::_tstring, 2> strOffsetType{ _T("Bottom Girder"),_T("Top Girder") };

   rptRcTable* pLayoutTable = rptStyleManager::CreateLayoutTable(2);
   *pPara << pLayoutTable;

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(5, _T(""));
   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CA_MIDDLE));
   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CA_MIDDLE));
   pTable->SetColumnStyle(1, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(1, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetColumnSpan(0, 0, 2);
   (*pTable)(0, 0) << _T("Point");

   (*pTable)(0, 2) << _T("Location");

   pTable->SetColumnSpan(0, 3, 2);
   (*pTable)(0, 3) << COLHDR(_T("Offset") , rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   (*pLayoutTable)(0, 0) << pTable << rptNewLine;
   (*pLayoutTable)(0, 1) << _T("Start: Start of tendon location measured from the start of the first span.") << rptNewLine;
   (*pLayoutTable)(0, 1) << _T("Low: Distance from Start point or previous High point. Measured as a distance or percentage of the distance between High points. Distance from End in last span.") << rptNewLine;
   (*pLayoutTable)(0, 1) << _T("IP: Location of inflection point measured from Hight point. Measured as distance or percentage of distance between High and Low points.") << rptNewLine;
   (*pLayoutTable)(0, 1) << _T("High: High point at centerline of pier.") << rptNewLine;
   (*pLayoutTable)(0, 1) << _T("End: End of tendon location measured from the end of the last span.") << rptNewLine;

   RowIndexType row = pTable->GetNumberOfHeaderRows();
   for (PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++)
   {
      if (pierIdx == startPierIdx)
      {
         pTable->SetRowSpan(row, 0, 3);
         (*pTable)(row, 0) << _T("Span ") << LABEL_SPAN(pierIdx);

         (*pTable)(row, 1) << _T("Start");
         if (startDist < 0)
         {
            (*pTable)(row, 2) << percentage.SetValue(-startDist);
         }
         else
         {
            (*pTable)(row, 2) << dim.SetValue(startDist);
         }
         (*pTable)(row, 3) << cmpdim.SetValue(startOffset);
         (*pTable)(row, 4) << strOffsetType[startOffsetType];

         row++;
      }

      if (pierIdx != startPierIdx && pierIdx != endPierIdx)
      {
         Float64 leftIP, highOffset, rightIP;
         CDuctGeometry::OffsetType highOffsetType;
         pDuct->ParabolicDuctGeometry.GetHighPoint(pierIdx, &leftIP, &highOffset, &highOffsetType, &rightIP);
         (*pTable)(row, 1) << _T("IP");
         if (leftIP < 0)
         {
            (*pTable)(row, 2) << percentage.SetValue(-leftIP);
         }
         else
         {
            (*pTable)(row, 2) << dim.SetValue(leftIP);
         }
         (*pTable)(row, 3) << _T("");
         (*pTable)(row, 4) << _T("");
         row++;

         (*pTable)(row, 0) << _T("Pier ") << LABEL_PIER(pierIdx);
         (*pTable)(row, 1) << _T("High");
         (*pTable)(row, 2) << _T("");
         (*pTable)(row, 3) << cmpdim.SetValue(highOffset);
         (*pTable)(row, 4) << strOffsetType[highOffsetType];
         row++;

         if (pierIdx < endPierIdx)
         {
            pTable->SetRowSpan(row, 0, 3);
            (*pTable)(row, 0) << _T("Span ") << LABEL_SPAN(pierIdx);
         }

         (*pTable)(row, 1) << _T("IP");
         if (rightIP < 0)
         {
            (*pTable)(row, 2) << percentage.SetValue(-rightIP);
         }
         else
         {
            (*pTable)(row, 2) << dim.SetValue(rightIP);
         }
         (*pTable)(row, 3) << _T("");
         (*pTable)(row, 4) << _T("");
         row++;
      }

      if (pierIdx != endPierIdx)
      {
         Float64 lowDist, lowOffset;
         CDuctGeometry::OffsetType lowOffsetType;
         pDuct->ParabolicDuctGeometry.GetLowPoint((SpanIndexType)pierIdx, &lowDist, &lowOffset, &lowOffsetType);
         (*pTable)(row, 1) << _T("Low");
         if (lowDist < 0)
         {
            (*pTable)(row, 2) << percentage.SetValue(-lowDist);
         }
         else
         {
            (*pTable)(row, 2) << dim.SetValue(lowDist);
         }
         (*pTable)(row, 3) << cmpdim.SetValue(lowOffset);
         (*pTable)(row, 4) << strOffsetType[lowOffsetType];

         row++;
      }

      if (pierIdx == endPierIdx)
      {
         (*pTable)(row, 1) << _T("End");

         if (endDist < 0)
         {
            (*pTable)(row, 2) << percentage.SetValue(-endDist);
         }
         else
         {
            (*pTable)(row, 2) << cmpdim.SetValue(endDist);
         }
         (*pTable)(row, 3) << cmpdim.SetValue(endOffset);
         (*pTable)(row, 4) << strOffsetType[endOffsetType];

         row++;
      }
   }
}
