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

#include <Reporting\MomentCapacityDetailsChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <PsgLib\BridgeDescription2.h>
#include <PgsExt\ReportPointOfInterest.h>

#include <IFace/Tools.h>
#include <EAF/EAFDisplayUnits.h>
#include <IFace\DocumentType.h>
#include <IFace\Bridge.h>
#include <IFace\MomentCapacity.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\BeamFactory.h>
#include <IFace\ResistanceFactors.h>
#include <IFace\ReportOptions.h>
#include <IFace/PointOfInterest.h>

#include <System\AutoVariable.h>

#include <psgLib/SpecificationCriteria.h>
#include <psgLib/MomentCapacityCriteria.h>
#include <psgLib/SpecLibraryEntry.h>
#include <psglib/GirderLibraryEntry.h>


void write_moment_data_table(std::shared_ptr<WBFL::EAF::Broker> pBroker,
                             std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,
                             const CGirderKey& girderKey,
                             const PoiList& vPoi,
                             rptChapter* pChapter,
                             IntervalIndexType intervalIdx,
                             const CString& strStageName,
                                 bool bPositiveMoment);

void write_crack_moment_data_table(std::shared_ptr<WBFL::EAF::Broker> pBroker,
                                   std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,
                             const CGirderKey& girderKey,
                                   const PoiList& vPoi,
                                   rptChapter* pChapter,
                                   IntervalIndexType intervalIdx,
                                   const CString& strStageName,
                                 bool bPositiveMoment);

void write_min_moment_data_table(std::shared_ptr<WBFL::EAF::Broker> pBroker,
                                 std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,
                             const CGirderKey& girderKey,
                                 const PoiList& vPoi,
                                 rptChapter* pChapter,
                                 IntervalIndexType intervalIdx,
                                 const CString& strStageName,
                                 bool bPositiveMoment);

void write_over_reinforced_moment_data_table(std::shared_ptr<WBFL::EAF::Broker> pBroker,
                                 std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,
                             const CGirderKey& girderKey,
                                 const PoiList& vPoi,
                                 rptChapter* pChapter,
                                 IntervalIndexType intervalIdx,
                                 const CString& strStageName,
                                 bool bPositiveMoment);


CMomentCapacityDetailsChapterBuilder::CMomentCapacityDetailsChapterBuilder(bool bReportCapacityOnly,bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
   m_bCapacityOnly = bReportCapacityOnly;
}

LPCTSTR CMomentCapacityDetailsChapterBuilder::GetName() const
{
   return TEXT("Moment Capacity Details");
}

rptChapter* CMomentCapacityDetailsChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
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
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2_NOCHECK(pBroker, IDocumentType, pDocType);

   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

// NOTE
// No longer designing/checking for ultimate moment in temporary construction state
// per e-mail from Bijan Khaleghi, dated 4/28/1999.  See project log.
//   vPoi = pIPOI->GetGirderPointsOfInterest(pgsTypes::BridgeSite1, span,girder);
//   write_moment_data_table(pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, pgsTypes::BridgeSite1, "Bridge Site Stage 1");
//   write_crack_moment_data_table(pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, pgsTypes::BridgeSite1, "Bridge Site Stage 1");
//   write_min_moment_data_table(pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, pgsTypes::BridgeSite1, "Bridge Site Stage 1");

   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey, &vGirderKeys);
   for(const auto& thisGirderKey : vGirderKeys)
   {
      CString strLabel;
      strLabel.Format(_T("Interval %d - %s"),LABEL_INTERVAL(lastIntervalIdx),pIntervals->GetDescription(lastIntervalIdx).c_str());

      rptParagraph* pPara;

      pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
      *pChapter << pPara;


      if (girderKey.groupIndex == ALL_GROUPS)
      {
         if (pDocType->IsPGSuperDocument())
         {
            *pPara << _T("Span ") << LABEL_SPAN(thisGirderKey.groupIndex) << _T(" Girder ") << LABEL_GIRDER(thisGirderKey.girderIndex) << rptNewLine << rptNewLine;
         }
         else
         {
            *pPara << _T("Group ") << LABEL_GROUP(thisGirderKey.groupIndex) << _T(" Girder ") << LABEL_GIRDER(thisGirderKey.girderIndex) << rptNewLine << rptNewLine;
         }
      }

      *pPara << _T("Positive Moment Capacity Details") << rptNewLine;
      *pPara << _T("Complete strain compatibility analysis results are available in the Moment Capacity Details report") << rptNewLine;

      PoiList vPoi;
      pIPOI->GetPointsOfInterest(CSegmentKey(thisGirderKey, ALL_SEGMENTS), &vPoi);

      write_moment_data_table(pBroker,pDisplayUnits,thisGirderKey, vPoi, pChapter, lastIntervalIdx, strLabel, true);
      if ( !m_bCapacityOnly )
      {
         write_crack_moment_data_table(          pBroker, pDisplayUnits, thisGirderKey, vPoi, pChapter, lastIntervalIdx, strLabel, true);
         write_min_moment_data_table(            pBroker, pDisplayUnits, thisGirderKey, vPoi, pChapter, lastIntervalIdx, strLabel, true);
         write_over_reinforced_moment_data_table(pBroker, pDisplayUnits, thisGirderKey, vPoi, pChapter, lastIntervalIdx, strLabel, true);
      }

      SpanIndexType startSpanIdx, endSpanIdx;
      startSpanIdx = pBridge->GetGirderGroupStartSpan(thisGirderKey.groupIndex);
      endSpanIdx   = pBridge->GetGirderGroupEndSpan(thisGirderKey.groupIndex);
      bool bProcessNegativeMoments = false;
      for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
      {
         if ( pBridge->ProcessNegativeMoments(spanIdx) )
         {
            bProcessNegativeMoments = true;
            break;
         }
      }

      if ( bProcessNegativeMoments )
      {
         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         *pChapter << pPara;
         *pPara << _T("Negative Moment Capacity Details") << rptNewLine;
         *pPara << _T("Complete strain compatibility analysis results are available in the Moment Capacity Details report") << rptNewLine;

         write_moment_data_table(pBroker,pDisplayUnits,thisGirderKey, vPoi, pChapter, lastIntervalIdx, strLabel, false);
         if ( !m_bCapacityOnly )
         {
            write_crack_moment_data_table(pBroker,pDisplayUnits,thisGirderKey, vPoi, pChapter, lastIntervalIdx, strLabel, false);
            write_min_moment_data_table(pBroker,pDisplayUnits,thisGirderKey, vPoi, pChapter, lastIntervalIdx, strLabel, false);
            write_over_reinforced_moment_data_table(pBroker,pDisplayUnits,thisGirderKey, vPoi, pChapter, lastIntervalIdx, strLabel, false);
         }
      }
   } // next girder

   return pChapter;
}

void write_moment_data_table(std::shared_ptr<WBFL::EAF::Broker> pBroker,
                             std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,
                             const CGirderKey& girderKey,
                             const PoiList& vPoi,
                             rptChapter* pChapter,
                             IntervalIndexType intervalIdx,
                             const CString& strStageName,
                             bool bPositiveMoment)
{
   rptParagraph* pPara = new rptParagraph();
   *pChapter << pPara;

   ASSERT_GIRDER_KEY(girderKey);

   GET_IFACE2(pBroker, IPointOfInterest, pPoi);
   GET_IFACE2(pBroker, ISegmentTendonGeometry, pSegmentTendonGeometry);
   GET_IFACE2(pBroker, IGirderTendonGeometry, pGirderTendonGeometry);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
   const GirderLibraryEntry* pGdrEntry = pGroup->GetGirder(girderKey.girderIndex)->GetGirderLibraryEntry();

   GET_IFACE2(pBroker, ILibrary, pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
   const auto& moment_capacity_criteria = pSpecEntry->GetMomentCapacityCriteria();
   bool bConsiderReinforcementStrainLimits = moment_capacity_criteria.bConsiderReinforcementStrainLimit;

   bool bAfter2005 = (WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2006Interims <= pSpecEntry->GetSpecificationCriteria().GetEdition() ? true : false);

   auto pFactory = pGdrEntry->GetBeamFactory();

   pgsTypes::SupportedDeckType deckType = pBridgeDesc->GetDeckDescription()->GetDeckType();

   SegmentIndexType nSegments = pGroup->GetGirder(girderKey.girderIndex)->GetSegmentCount();

   bool bUHPC = false;
   GET_IFACE2(pBroker, IMaterials, pMaterials);

   DuctIndexType nMaxSegmentTendons = 0;
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CSegmentKey segmentKey(girderKey, segIdx);
      nMaxSegmentTendons = Max(nMaxSegmentTendons, pSegmentTendonGeometry->GetDuctCount(segmentKey));
      if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC)
         bUHPC = true;
   }

   DuctIndexType nGirderTendons = pGirderTendonGeometry->GetDuctCount(girderKey);

   std::_tstring strPicture;
   if (bPositiveMoment)
   {
      strPicture = pFactory->GetPositiveMomentCapacitySchematicImage(deckType);
   }
   else
   {
      strPicture = pFactory->GetNegativeMomentCapacitySchematicImage(deckType);
   }

   *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + strPicture) << rptNewLine;

   *pPara << rptNewLine;

   // Setup the table
   pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;

   CString strLabel;
   strLabel.Format(_T("Moment Capacity - %s"), strStageName);

   ColumnIndexType nColumns;
   if (bPositiveMoment || 0 < nGirderTendons || 0 < nMaxSegmentTendons)
   {
      nColumns = 12;
      if (WBFL::LRFD::BDSManager::GetEdition() <= WBFL::LRFD::BDSManager::Edition::FifthEdition2010)
      {
         nColumns++; // for PPR
      }
   }
   else
   {
      nColumns = 12;
   }

   if (bUHPC)
   {
      nColumns += 2; // curvatures
   }

   if (WBFL::LRFD::BDSManager::Edition::SixthEdition2012 <= WBFL::LRFD::BDSManager::GetEdition())
   {
      nColumns++; // for epsilon_t
   }

   nColumns += 1; // for de_shear

   if (bPositiveMoment || (!bPositiveMoment && moment_capacity_criteria.bIncludeStrandForNegMoment) || 0 < nGirderTendons || 0 < nMaxSegmentTendons)
   {
      nColumns += 1; // for fps_avg
   }

   if (0 < nMaxSegmentTendons)
   {
      nColumns += 1; // for fpt_avg_segment
   }

   if (0 < nGirderTendons)
   {
      nColumns += 1; // for fpt_avg_girder
   }

   rptRcTable* table = rptStyleManager::CreateDefaultTable(nColumns, strLabel);

   *pPara << table << rptNewLine;


   pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pPara;
   (*pPara) << _T("* Used to compute ") << Sub2(_T("d"), _T("v")) << _T(" for shear. Depth to resultant tension force for reinforcement on the tension half of the section. See PCI Bridge Design Manual, 3rd Edition, MNL-133-11, §8.4.1.2") << rptNewLine;

   pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pPara;
   if (bUHPC)
   {
      *pPara << _T("+ Controlling: C=Crushing of extreme concrete fibers, G=Crushing of girder concrete, L=Crack localization, D=Reduced strand stress due to lack of full development per LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.11.4.2"), _T("5.9.4.3.2")) << _T(", R=Reinforcement strain limit");
   }
   else
   {
      *pPara << _T("+ Controlling: C=Concrete crushing, D=Reduced strand stress due to lack of full development per LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.11.4.2"), _T("5.9.4.3.2"));
      if (bConsiderReinforcementStrainLimits)
      {
         *pPara << _T(", R=Reinforcement strain exceeds minimum elongation per the material specification");
      }  
      else
      {
         *pPara << _T(", E=Reinforcement strain was not limited to minimum elongation per the material specification");
      }
   }


   *pPara << rptNewLine;


   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   ColumnIndexType col = 0;

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();
   if ( intervalIdx < lastCompositeDeckIntervalIdx)
   {
      (*table)(0,col++)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   }
   else
   {
      (*table)(0,col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   }

   (*table)(0,col++) << COLHDR(_T("c"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(_T("d") << Sub(_T("c")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(_T("d") << Sub(_T("e")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   
   (*table)(0,col++) << COLHDR(_T("d") << Sub(_T("e")) << rptNewLine << _T("(Shear *)"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   (*table)(0,col++) << COLHDR(_T("d") << Sub(_T("t")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   if (bUHPC)
   {
      (*table)(0, col++) << COLHDR(Sub2(symbol(psi), _T("n")), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());
      (*table)(0, col++) << COLHDR(Sub2(symbol(psi), _T("sl")), rptPerLengthUnitTag, pDisplayUnits->GetCurvatureUnit());
   }

   if (WBFL::LRFD::BDSManager::Edition::SixthEdition2012 <= WBFL::LRFD::BDSManager::GetEdition())
   {
      (*table)(0, col++) << Sub2(symbol(epsilon), _T("t")) << _T(" x 1000");
   }

   if ( bPositiveMoment )
   {
      (*table)(0,col++) << COLHDR(RPT_STRESS(_T("ps,avg")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      if (0 < nMaxSegmentTendons)
      {
         (*table)(0, col++) << COLHDR(_T("Segment Tendons") << rptNewLine << RPT_STRESS(_T("pt,avg")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      }

      if ( 0 < nGirderTendons )
      {
         (*table)(0,col++) << COLHDR(_T("Girder Tendons") << rptNewLine << RPT_STRESS(_T("pt,avg")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }

      if ( WBFL::LRFD::BDSManager::GetEdition() <= WBFL::LRFD::BDSManager::Edition::FifthEdition2010 )
      {
         (*table)(0,col++) << _T("PPR");
      }
   }
   else
   {
      if (moment_capacity_criteria.bIncludeStrandForNegMoment || 0 < nGirderTendons+nMaxSegmentTendons)
      {
         (*table)(0,col++) << COLHDR(RPT_STRESS(_T("ps,avg")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

         if (0 < nMaxSegmentTendons)
         {
            (*table)(0, col++) << COLHDR(_T("Segment Tendons") << rptNewLine << RPT_STRESS(_T("pt,avg")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
         }

         if (0 < nGirderTendons)
         {
            (*table)(0, col++) << COLHDR(_T("Girder Tendons") << rptNewLine << RPT_STRESS(_T("pt,avg")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
         }
      }
   }

   (*table)(0,col++) << symbol(phi);
   (*table)(0,col++) << COLHDR(_T("Moment") << rptNewLine << _T("Arm"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(_T("C"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*table)(0,col++) << COLHDR(_T("T"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*table)(0, col++) << COLHDR(Sub2(_T("M"),_T("n")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
   (*table)(0, col++) << COLHDR(Sub2(_T("M"), _T("r")) << _T(" = ") << symbol(phi) << Sub2(_T("M"),_T("n")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
   (*table)(0, col++) << _T("Controlling +");

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,    pDisplayUnits->GetGeneralForceUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment,   pDisplayUnits->GetMomentUnit(),       false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false);
   INIT_UV_PROTOTYPE(rptPerLengthUnitValue, curvature, pDisplayUnits->GetCurvatureUnit(), false);

   GET_IFACE2(pBroker,IReportOptions,pReportOptions);
   location.IncludeSpanAndGirder(pReportOptions->IncludeSpanAndGirder4Pois(girderKey));

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   rptRcScalar strain;
   strain.SetFormat(WBFL::System::NumericFormatTool::Format::Automatic);
   strain.SetWidth(9);
   strain.SetPrecision(4);

   Int16 count = 0;
   RowIndexType row = table->GetNumberOfHeaderRows();

   std::array<std::_tstring, 4> strControlling{ _T("C"), _T("G"), _T("L"), _T("R")};

   GET_IFACE2(pBroker,IMomentCapacity,pMomentCap);
   for (const pgsPointOfInterest& poi : vPoi)
   {
      const CSegmentKey& segmentKey(poi.GetSegmentKey());
      bool bIsOnSegment = pPoi->IsOnSegment(poi);

      bool bUHPC_this_poi = (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC) ? true : false;

      const MOMENTCAPACITYDETAILS* pmcd = pMomentCap->GetMomentCapacityDetails(intervalIdx,poi,bPositiveMoment);

      if (pPoi->IsInBoundaryPierDiaphragm(poi))
      {
         // no UHPC for pier diaphragms
         bUHPC_this_poi = false;
      }

      col = 0;

      (*table)(row,col++) << location.SetValue( POI_SPAN, poi );
      (*table)(row,col++) << dim.SetValue( pmcd->c );
      (*table)(row,col++) << dim.SetValue( pmcd->dc );
      (*table)(row,col++) << dim.SetValue( pmcd->de );
      (*table)(row,col++) << dim.SetValue( pmcd->de_shear );
      (*table)(row,col++) << dim.SetValue( pmcd->dt );

      if (bIsOnSegment && bUHPC_this_poi)
      {
         CComPtr<IMomentCapacitySolution> solution;
         const_cast<MOMENTCAPACITYDETAILS*>(pmcd)->GetControllingSolution(&solution);
         Float64 k;
         solution->get_Curvature(&k);
         (*table)(row, col++) << curvature.SetValue(k);

         k = 0;
         if (pmcd->ReinforcementStressLimitStateSolution)
         {
            pmcd->ReinforcementStressLimitStateSolution->get_Curvature(&k);
         }
         (*table)(row, col++) << curvature.SetValue(k);
      }
      else if (bUHPC && !bUHPC_this_poi)
      {
         // there is UHPC in the girder line so there are two
         // columns in the table for curvature, Y.n and Y.sl,
         // but the material at this POI is not UHPC so
         // skip those columns
         (*table)(row, col++) << _T("");
         (*table)(row, col++) << _T("");
      }

      if (WBFL::LRFD::BDSManager::Edition::SixthEdition2012 <= WBFL::LRFD::BDSManager::GetEdition())
      {
         (*table)(row, col++) << strain.SetValue(pmcd->et * 1000.0);
      }

      if ( bPositiveMoment)
      {
         (*table)(row,col++) << stress.SetValue( pmcd->fps_avg );

         if (0 < nMaxSegmentTendons)
         {
            (*table)(row, col++) << stress.SetValue(pmcd->fpt_avg_segment);
         }

         if ( 0 < nGirderTendons )
         {
            (*table)(row,col++) << stress.SetValue( pmcd->fpt_avg_girder );
         }

         if ( WBFL::LRFD::BDSManager::GetEdition() <= WBFL::LRFD::BDSManager::Edition::FifthEdition2010 )
         {
            (*table)(row,col++) << scalar.SetValue( pmcd->PPR );
         }
      }
      else
      {
         if (moment_capacity_criteria.bIncludeStrandForNegMoment || 0 < nGirderTendons+nMaxSegmentTendons )
         {
            (*table)(row,col++) << stress.SetValue( pmcd->fps_avg );

            if (0 < nMaxSegmentTendons)
            {
               (*table)(row, col++) << stress.SetValue(pmcd->fpt_avg_segment);
            }

            if (0 < nGirderTendons)
            {
               (*table)(row, col++) << stress.SetValue(pmcd->fpt_avg_girder);
            }
         }
      }

      (*table)(row,col++) << scalar.SetValue( pmcd->Phi );
      (*table)(row,col++) << dim.SetValue( pmcd->MomentArm );
      (*table)(row,col++) << force.SetValue( -pmcd->C );
      (*table)(row,col++) << force.SetValue( pmcd->T );
      (*table)(row, col++) << moment.SetValue(pmcd->Mn);
      (*table)(row, col++) << moment.SetValue(pmcd->Mr);

#if defined _DEBUG
      // sanity check - if this is not UHPC (which reinforcement strains are always checked)
      // and we are not checking reinforcement strain limits, then the controlling type
      // should be ReinforcementFracture - If this assert fires, verify that there isn't a good
      // reason for Controlling to be ReinforcementFracture before assuming there is a bug.
      if (!bUHPC && !bConsiderReinforcementStrainLimits)
      {
         ASSERT(pmcd->Controlling != MOMENTCAPACITYDETAILS::ControllingType::ReinforcementFracture);
      }
#endif
      (*table)(row, col) << strControlling[+(pmcd->Controlling)];
      
      if (pmcd->bDevelopmentLengthReducedStress)
         (*table)(row, col) << _T(", D");

      if (!bUHPC && !bConsiderReinforcementStrainLimits)
         (*table)(row, col) << _T(", E");
      
      col++;

      row++;
      count++;
   }

   pPara = new rptParagraph;
   *pChapter << pPara;

   if (bUHPC)
   {
      *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("UHPC_FlexureResistanceFactor.png")) << rptNewLine;
      GET_IFACE2(pBroker, IResistanceFactors, pResistanceFactors);
      *pPara << Sub2(symbol(mu), _T("l")) << _T(" = ") << pResistanceFactors->GetDuctilityCurvatureRatioLimit() << _T(" GS 1.6.2") << rptNewLine;
   }
   else if ( WBFL::LRFD::BDSManager::GetEdition() <= WBFL::LRFD::BDSManager::Edition::FifthEdition2010 )
   {
      if ( pSpec->GetMomentCapacityMethod() == WSDOT_METHOD || bAfter2005 )
      {
         *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("GeneralizedFlexureResistanceFactor.png")) << rptNewLine;
      }
   }
   else
   {
      const MOMENTCAPACITYDETAILS* pmcd = pMomentCap->GetMomentCapacityDetails(intervalIdx,vPoi.front(),bPositiveMoment);
      *pPara << Sub2(symbol(epsilon),_T("cl")) << _T(" = ") << strain.SetValue(pmcd->ecl) << rptNewLine;
      *pPara << Sub2(symbol(epsilon),_T("tl")) << _T(" = ") << strain.SetValue(pmcd->etl) << rptNewLine;

      if ( 0 < nGirderTendons+nMaxSegmentTendons )
      {
         *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("PositiveMomentFlexureResistanceFactor_SplicedGirders.png")) << rptNewLine;
      }
      else
      {
         if ( bPositiveMoment )
         {
            *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("PositiveMomentFlexureResistanceFactor.png")) << rptNewLine;
         }
         else
         {
            *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("NegativeMomentFlexureResistanceFactor.png")) << rptNewLine;
         }
      }
   }

}

void write_crack_moment_data_table(std::shared_ptr<WBFL::EAF::Broker> pBroker,
                                   std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,
                             const CGirderKey& girderKey,
                                   const PoiList& vPoi,
                                   rptChapter* pChapter,
                                   IntervalIndexType intervalIdx,
                                   const CString& strStageName,
                                 bool bPositiveMoment)
{
   bool bAfter2002  = ( WBFL::LRFD::BDSManager::Edition::SecondEditionWith2002Interims < WBFL::LRFD::BDSManager::GetEdition()     ? true : false );
   bool bBefore2012 = ( WBFL::LRFD::BDSManager::GetEdition()                  < WBFL::LRFD::BDSManager::Edition::SixthEdition2012 ? true : false );

   // Setup the table
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << (bPositiveMoment ? _T("Positive") : _T("Negative")) << _T(" Cracking Moment Details [") << WBFL::LRFD::LrfdCw8th(_T("5.7.3.3.2"),_T("5.6.3.3")) << _T("] - ") << strStageName << rptNewLine;
  
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   ColumnIndexType nColumns = bAfter2002 ? 8 : 7;
   if ( WBFL::LRFD::BDSManager::Edition::SixthEdition2012 <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      nColumns--; // No Scfr column for LRFD 6th, 2012 and later
   }
   
   rptRcTable* table = rptStyleManager::CreateDefaultTable(nColumns,_T(""));

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();
   if ( intervalIdx < lastCompositeDeckIntervalIdx)
   {
      (*table)(0, col++)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   }
   else
   {
      (*table)(0, col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   }

   (*table)(0, col++)  << COLHDR( RPT_STRESS(_T("r")), rptPressureUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0, col++)  << COLHDR( RPT_STRESS(_T("cpe")), rptPressureUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0, col++)  << COLHDR( Sub2(_T("S"),_T("nc")), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
   (*table)(0, col++)  << COLHDR( Sub2(_T("S"),_T("c")), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
   (*table)(0, col++)  << COLHDR( Sub2(_T("M"),_T("dnc")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0, col++)  << COLHDR( Sub2(_T("M"),_T("cr")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   if ( bAfter2002 && bBefore2012 )
   {
      (*table)(0, col++)  << COLHDR(Sub2(_T("S"),_T("c")) << RPT_STRESS(_T("r")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   }

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptLength3UnitValue, sect_mod, pDisplayUnits->GetSectModulusUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), false );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, fr_coefficient, pDisplayUnits->GetTensionCoefficientUnit(), false );
   
   rptRcScalar scalar;
   scalar.SetFormat( WBFL::System::NumericFormatTool::Format::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(2);
   scalar.SetTolerance(1.0e-6);

   GET_IFACE2(pBroker,IReportOptions,pReportOptions);
   location.IncludeSpanAndGirder(pReportOptions->IncludeSpanAndGirder4Pois(girderKey));

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(CSegmentKey(girderKey,0));

   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IMomentCapacity,pMomentCapacity);

   bool bFirstPoi = true;
   for (const pgsPointOfInterest& poi : vPoi)
   {
      const CRACKINGMOMENTDETAILS* pcmd = pMomentCapacity->GetCrackingMomentDetails(intervalIdx,poi,bPositiveMoment);

      if ( bFirstPoi )
      {
         bFirstPoi = false;
         if ( WBFL::LRFD::BDSManager::Edition::SixthEdition2012 <= WBFL::LRFD::BDSManager::GetEdition() )
         {
            *pPara << rptNewLine;
            *pPara << _T("Flexural cracking variability factor, ") << Sub2(symbol(gamma),_T("1")) << _T(" = ") << scalar.SetValue(pcmd->g1) << rptNewLine;
            *pPara << _T("Prestress variability factor, ") << Sub2(symbol(gamma),_T("2")) << _T(" = ") << scalar.SetValue(pcmd->g2) << rptNewLine;
            *pPara << _T("Ratio of specified minimum yield strength to ultimate tensile strength of the reinforcement, " ) << Sub2(symbol(gamma),_T("3")) << _T(" = ") << scalar.SetValue(pcmd->g3) << rptNewLine;
            *pPara << rptNewLine;
         }
      }

      col = 0;
      (*table)(row, col++) << location.SetValue( POI_SPAN, poi );
      (*table)(row, col++) << stress.SetValue( pcmd->fr );
      (*table)(row, col++) << stress.SetValue( pcmd->fcpe);
      (*table)(row, col++) << sect_mod.SetValue( pcmd->Sb );
      (*table)(row, col++) << sect_mod.SetValue( pcmd->Sbc );
      (*table)(row, col++) << moment.SetValue( pcmd->Mdnc);
      (*table)(row, col++) << moment.SetValue( pcmd->Mcr );

      if ( bAfter2002 && bBefore2012 )
      {
         (*table)(row, col++) << moment.SetValue( pcmd->McrLimit );
      }

      row++;
   }

   pParagraph = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pParagraph;


   if ( bBefore2012 )
   {
      if ( bAfter2002 )
      {
         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Mcr_2005.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Mcr.png")) << rptNewLine;
      }
   }
   else
   {
      // LRFD 6th Edition, 2012 and later
      *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Mcr_2012.png")) << rptNewLine;
   }

   GET_IFACE2(pBroker,IMaterials,pMaterial);
   bool bLambda = (WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims <= WBFL::LRFD::BDSManager::GetEdition() ? true : false);

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      if (1 < nSegments)
      {
         *pParagraph << _T("Segment") << LABEL_SEGMENT(segIdx) << _T(" ");
      }

      if (pMaterial->GetSegmentConcreteType(segmentKey) == pgsTypes::PCI_UHPC)
      {
         *pParagraph << RPT_STRESS(_T("r")) << _T(" = tensile stress limit at service limit state") << rptNewLine;
      }
      else if (pMaterial->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC)
      {
         *pParagraph << RPT_STRESS(_T("r")) << _T(" = effective cracking strength = ") << RPT_STRESS(_T("t,cr")) << _T(" GS 1.6.3.3") << rptNewLine;
      }
      else
      {
         *pParagraph << RPT_STRESS(_T("r")) << _T(" = ") << fr_coefficient.SetValue(pMaterial->GetSegmentFlexureFrCoefficient(segmentKey));
         if (bLambda)
         {
            *pParagraph << symbol(lambda);
         }
         *pParagraph << symbol(ROOT) << RPT_FC << rptNewLine;
      }

      if ( segIdx != nSegments-1 )
      {
         CClosureKey closureKey(segmentKey);
         *pParagraph << _T("Closure Joint ") << LABEL_SEGMENT(segIdx) << _T(" ") << RPT_STRESS(_T("r")) << _T(" = ");
         if (pMaterial->GetClosureJointConcreteType(closureKey) == pgsTypes::PCI_UHPC)
         {
            *pParagraph << _T(" = tensile stress limit at service limit state") << rptNewLine;
         }
         else if (pMaterial->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC)
         {
            *pParagraph << RPT_STRESS(_T("r")) << _T(" = effective cracking strength = ") << RPT_STRESS(_T("t,cr")) << _T(" GS 1.6.3.3") << rptNewLine;
         }
         else
         {
            *pParagraph << fr_coefficient.SetValue(pMaterial->GetClosureJointFlexureFrCoefficient(closureKey));
            if (bLambda)
            {
               *pParagraph << symbol(lambda);
            }
            *pParagraph << symbol(ROOT) << RPT_FC << rptNewLine;
         }
      }
   }

   *pParagraph << RPT_STRESS(_T("cpe")) << _T(" = compressive stress in concrete due to effective prestress force only (after allowance for all prestress losses) at extreme fiber of section where tensile stress is caused by externally applied loads.") << rptNewLine;
   *pParagraph << Sub2(_T("S"),_T("nc")) << _T(" = section modulus for the extreme fiber of the monolithic or noncomposite section where tensile stress is caused by externally applied loads") << rptNewLine;
   *pParagraph << Sub2(_T("S"),_T("c")) << _T(" = section modulus for the extreme fiber of the composite section where tensile stress is caused by externally applied loads") << rptNewLine;
   *pParagraph << Sub2(_T("M"),_T("dnc")) << _T(" = total unfactored dead load moment acting on the monolithic or noncomposite section") << rptNewLine;
   *pParagraph << rptNewLine;
}

void write_min_moment_data_table(std::shared_ptr<WBFL::EAF::Broker> pBroker,
                                 std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,
                             const CGirderKey& girderKey,
                                 const PoiList& vPoi,
                                 rptChapter* pChapter,
                                 IntervalIndexType intervalIdx,
                                 const CString& strStageName,
                                 bool bPositiveMoment)
{
   bool bBefore2012 = ( WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::SixthEdition2012 ? true : false );

   // Setup the table
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << _T("Minimum Reinforcement [") << WBFL::LRFD::LrfdCw8th(_T("5.7.3.3.2"),_T("5.6.3.3")) << _T("] - ") << strStageName << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   rptRcTable* table = rptStyleManager::CreateDefaultTable(bBefore2012 ? 7 : 6,_T(""));

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();
   if ( intervalIdx < lastCompositeDeckIntervalIdx)
   {
      (*table)(0,col++)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   }
   else
   {
      (*table)(0,col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   }

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

   GET_IFACE2(pBroker,IReportOptions,pReportOptions);
   location.IncludeSpanAndGirder(pReportOptions->IncludeSpanAndGirder4Pois(girderKey));

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(CSegmentKey(girderKey,0));
   if ( intervalIdx < lastCompositeDeckIntervalIdx)
   {
      end_size = 0; // don't adjust if CY stage
   }

   RowIndexType row = table->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IMomentCapacity,pMomentCapacity);

   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;
      const MINMOMENTCAPDETAILS* pmmcd = pMomentCapacity->GetMinMomentCapacityDetails(intervalIdx,poi,bPositiveMoment);

      (*table)(row,col++) << location.SetValue( POI_SPAN, poi );
      if ( bBefore2012 )
      {
         (*table)(row,col++) << moment.SetValue( pmmcd->Mcr );
      }
      (*table)(row,col++) << moment.SetValue( pmmcd->MrMin1 );
      (*table)(row,col++) << pmmcd->LimitState;
      (*table)(row,col++) << moment.SetValue( pmmcd->Mu );
      (*table)(row,col++) << moment.SetValue( pmmcd->MrMin2 );
      (*table)(row,col++) << moment.SetValue( pmmcd->MrMin );

      row++;
   }

   pParagraph = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pParagraph;
   if ( bBefore2012 )
   {
      *pParagraph << symbol(phi) << Sub2(_T("M"),_T("n")) << _T(" Min = ") << _T("min(") << Sub2(_T("1.2M"),_T("cr")) << _T(", ") << Sub2(_T("1.33M"),_T("u")) << _T(")") << rptNewLine;
   }
   else
   {
      *pParagraph << symbol(phi) << Sub2(_T("M"),_T("n")) << _T(" Min = ") << _T("min(") << Sub2(_T("M"),_T("cr")) << _T(", ") << Sub2(_T("1.33M"),_T("u")) << _T(")") << rptNewLine;
   }
}

void write_over_reinforced_moment_data_table(std::shared_ptr<WBFL::EAF::Broker> pBroker,
                                 std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,
                             const CGirderKey& girderKey,
                                 const PoiList& vPoi,
                                 rptChapter* pChapter,
                                 IntervalIndexType intervalIdx,
                                 const CString& strStageName,
                                 bool bPositiveMoment)
{
   // Determine if this table is even needed...
   // It isn't needed if there aren't any over reinforced sections
   bool bTableNeeded = false;
   GET_IFACE2(pBroker,IMomentCapacity,pMomentCap);
   for (const pgsPointOfInterest& poi : vPoi)
   {
      const MOMENTCAPACITYDETAILS* pmcd = pMomentCap->GetMomentCapacityDetails(intervalIdx,poi,bPositiveMoment);
      if ( pmcd->bOverReinforced )
      {
         bTableNeeded = true;
      }
   }

   if ( !bTableNeeded )
   {
      return;
   }

   // If we get here, the table is needed.

   // Setup the table
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   if ( bPositiveMoment )
   {
      *pParagraph << _T("Limiting Capacity of Over Reinforced Sections - Positive Moment - ") << strStageName << rptNewLine;
   }
   else
   {
      *pParagraph << _T("Limiting Capacity of Over Reinforced Sections - Negative Moment - ") << strStageName << rptNewLine;
   }

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;
   *pParagraph << _T("Over reinforced sections may be considered adequate if the flexural demand does not exceed the flexural resistance suggested by LRFD C5.7.3.3.1. (removed from spec 2005)") << rptNewLine;
   *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("LimitingCapacityOfOverReinforcedSections.jpg")) << rptNewLine;

   rptRcTable* table = rptStyleManager::CreateDefaultTable(10,_T("Nominal Resistance of Over Reinforced Sections [C5.7.3.3.1]"));

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   *pParagraph << table << rptNewLine;
   
   ColumnIndexType col = 0;

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();
   if ( intervalIdx < lastCompositeDeckIntervalIdx)
   {
      (*table)(0, col++)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   }
   else
   {
      (*table)(0, col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   }

   (*table)(0,col++) << symbol(beta) << Sub(_T("1"));
   (*table)(0,col++) << COLHDR(_T("f") << Sub(_T("c")), rptStressUnitTag,pDisplayUnits->GetStressUnit());
   (*table)(0,col++) << COLHDR(_T("b"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(_T("b") << Sub(_T("w")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(_T("d") << Sub(_T("e")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(_T("h") << Sub(_T("f")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << _T("Equation");
   (*table)(0,col++) << COLHDR(_T("M") << Sub(_T("n")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << COLHDR(symbol(phi) << _T("M") << Sub(_T("n")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment,   pDisplayUnits->GetMomentUnit(),       false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   GET_IFACE2(pBroker,IReportOptions,pReportOptions);
   location.IncludeSpanAndGirder(pReportOptions->IncludeSpanAndGirder4Pois(girderKey));

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(CSegmentKey(girderKey,0));
   if ( intervalIdx < lastCompositeDeckIntervalIdx)
   {
      end_size = 0; // don't adjust if CY stage
   }


   RowIndexType row = table->GetNumberOfHeaderRows();

   for( const pgsPointOfInterest& poi : vPoi)
   {
      const MOMENTCAPACITYDETAILS* pmcd = pMomentCap->GetMomentCapacityDetails(intervalIdx,poi,bPositiveMoment);

      if ( pmcd->bOverReinforced )
      {
         col = 0;
         (*table)(row,col++) << location.SetValue( POI_SPAN, poi );
         (*table)(row,col++) << scalar.SetValue( pmcd->Beta1Slab );
         (*table)(row,col++) << stress.SetValue( pmcd->FcSlab );
         (*table)(row,col++) << dim.SetValue( pmcd->b );
         (*table)(row,col++) << dim.SetValue( pmcd->bw );
         (*table)(row,col++) << dim.SetValue( pmcd->de );
         (*table)(row,col++) << dim.SetValue( pmcd->hf );
         (*table)(row,col++) << (pmcd->bRectSection ? _T("C5.7.3.3.1-1") : _T("C5.7.3.3.1-2"));
         (*table)(row,col++) << moment.SetValue( pmcd->MnMin );
         (*table)(row,col++) << moment.SetValue( pmcd->Phi * pmcd->MnMin );

         row++;
      }
   }
}