///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\ReportPointOfInterest.h>

#include <IFace\DocumentType.h>
#include <IFace\Bridge.h>
#include <IFace\MomentCapacity.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\BeamFactory.h>
#include <IFace\DocumentType.h>

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
                             const CGirderKey& girderKey,
                             const PoiList& vPoi,
                             rptChapter* pChapter,
                             IntervalIndexType intervalIdx,
                             const CString& strStageName,
                                 bool bPositiveMoment);

void write_crack_moment_data_table(IBroker* pBroker,
                                   IEAFDisplayUnits* pDisplayUnits,
                             const CGirderKey& girderKey,
                                   const PoiList& vPoi,
                                   rptChapter* pChapter,
                                   IntervalIndexType intervalIdx,
                                   const CString& strStageName,
                                 bool bPositiveMoment);

void write_min_moment_data_table(IBroker* pBroker,
                                 IEAFDisplayUnits* pDisplayUnits,
                             const CGirderKey& girderKey,
                                 const PoiList& vPoi,
                                 rptChapter* pChapter,
                                 IntervalIndexType intervalIdx,
                                 const CString& strStageName,
                                 bool bPositiveMoment);

void write_over_reinforced_moment_data_table(IBroker* pBroker,
                                 IEAFDisplayUnits* pDisplayUnits,
                             const CGirderKey& girderKey,
                                 const PoiList& vPoi,
                                 rptChapter* pChapter,
                                 IntervalIndexType intervalIdx,
                                 const CString& strStageName,
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
      strLabel.Format(_T("Interval %d - %s"),LABEL_INTERVAL(lastIntervalIdx),pIntervals->GetDescription(lastIntervalIdx));

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
   bool bConsiderReinforcementStrainLimits = pSpecEntry->ConsiderReinforcementStrainLimitForMomentCapacity();

   bool bAfter2005 = (lrfdVersionMgr::ThirdEditionWith2006Interims <= pSpecEntry->GetSpecificationType() ? true : false);

   CComPtr<IBeamFactory> pFactory;
   pGdrEntry->GetBeamFactory(&pFactory);

   pgsTypes::SupportedDeckType deckType = pBridgeDesc->GetDeckDescription()->GetDeckType();

   SegmentIndexType nSegments = pGroup->GetGirder(girderKey.girderIndex)->GetSegmentCount();

   DuctIndexType nMaxSegmentTendons = 0;
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CSegmentKey segmentKey(girderKey, segIdx);
      nMaxSegmentTendons = Max(nMaxSegmentTendons, pSegmentTendonGeometry->GetDuctCount(segmentKey));
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
   strLabel.Format(_T("Moment Capacity [%s] - %s"), LrfdCw8th(_T("5.7.3.2.4"), _T("5.6.3.2.4")), strStageName);

   ColumnIndexType nColumns;
   if (bPositiveMoment || 0 < nGirderTendons || 0 < nMaxSegmentTendons)
   {
      nColumns = 11;// 12;
      if (lrfdVersionMgr::GetVersion() <= lrfdVersionMgr::FifthEdition2010)
      {
         nColumns++; // for PPR
      }
   }
   else
   {
      nColumns = 11;
   }

   if (lrfdVersionMgr::SixthEdition2012 <= lrfdVersionMgr::GetVersion())
   {
      nColumns++; // for epsilon_t
   }

   //if ( 0 < nMaxSegmentTendons )
   //{
   //   nColumns += 2*nMaxSegmentTendons; // fpe and epsilon_pti
   //}

   //if ( 0 < nGirderTendons )
   //{
   //   nColumns += 2*nGirderTendons; // fpe and epsilon_pti
   //}

   if (bPositiveMoment)
   {
      nColumns += 1; // for de_shear
   }

   if (bPositiveMoment || 0 < nGirderTendons || 0 < nMaxSegmentTendons)
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


   if (bPositiveMoment)
   {
      rptParagraph* pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
      *pChapter << pPara;
      (*pPara) << _T("* Used to compute ") << Sub2(_T("d"), _T("v")) << _T(" for shear. Depth to resultant tension force for strands on the tension half of the section. See PCI Bridge Design Manual, 3rd Edition, MNL-133-11, §8.4.1.2") << rptNewLine;
   }

   pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pPara;
   *pPara << _T("+ Controlling: C=Concrete crushing, T=Tension strain limit of reinforcement, D=Reduced strand stress due to lack of full development per LRFD ") << LrfdCw8th(_T("5.11.4.2"), _T("5.9.4.3.2"));
   if (!bConsiderReinforcementStrainLimits)
   {
      *pPara << _T(", E=Reinforcement strain exceeds minimum elongation per the material specification");
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
   
   if ( bPositiveMoment )
   {
      (*table)(0,col++) << COLHDR(_T("d") << Sub(_T("e")) << rptNewLine << _T("(Shear *)"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   }

   (*table)(0,col++) << COLHDR(_T("d") << Sub(_T("t")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   if ( lrfdVersionMgr::SixthEdition2012 <= lrfdVersionMgr::GetVersion() )
   {
      (*table)(0,col++) << Sub2(symbol(epsilon),_T("t")) << _T(" x 1000");
   }

   if ( bPositiveMoment )
   {
      //if ( 0 == nGirderTendons+nMaxSegmentTendons )
      //{
      //   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("pe")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      //   (*table)(0,col++) << Sub2(symbol(epsilon),_T("psi")) << rptNewLine << _T("x 1000");
      //}
      //else
      //{
      //   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("pe ps")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      //   (*table)(0,col++) << Sub2(symbol(epsilon),_T("psi")) << rptNewLine << _T("x 1000");

      //   for (DuctIndexType tendonIdx = 0; tendonIdx < nMaxSegmentTendons; tendonIdx++)
      //   {
      //      (*table)(0, col++) << COLHDR(_T("Segment Tendon ") << LABEL_DUCT(tendonIdx) << rptNewLine << RPT_STRESS(_T("pe pt")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      //      (*table)(0, col++) << _T("Segment Tendon ") << LABEL_DUCT(tendonIdx) << rptNewLine << Sub2(symbol(epsilon), _T("pti")) << rptNewLine << _T("x 1000");
      //   }

      //   for ( DuctIndexType tendonIdx = 0; tendonIdx < nGirderTendons; tendonIdx++ )
      //   {
      //      (*table)(0,col++) << COLHDR(_T("Girder Tendon ") << LABEL_DUCT(tendonIdx) << rptNewLine << RPT_STRESS(_T("pe pt")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      //      (*table)(0,col++) << _T("Girder Tendon ") << LABEL_DUCT(tendonIdx) << rptNewLine << Sub2(symbol(epsilon),_T("pti")) << rptNewLine << _T("x 1000");
      //   }
      //}

      (*table)(0,col++) << COLHDR(RPT_STRESS(_T("ps,avg")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      if (0 < nMaxSegmentTendons)
      {
         (*table)(0, col++) << COLHDR(_T("Segment Tendons") << rptNewLine << RPT_STRESS(_T("pt,avg")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
      }

      if ( 0 < nGirderTendons )
      {
         (*table)(0,col++) << COLHDR(_T("Girder Tendons") << rptNewLine << RPT_STRESS(_T("pt,avg")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }

      if ( lrfdVersionMgr::GetVersion() <= lrfdVersionMgr::FifthEdition2010 )
      {
         (*table)(0,col++) << _T("PPR");
      }
   }
   else
   {
      if ( 0 < nGirderTendons+nMaxSegmentTendons)
      {
         //(*table)(0,col++) << COLHDR(RPT_STRESS(_T("pe ps")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         //(*table)(0,col++) << Sub2(symbol(epsilon),_T("psi")) << rptNewLine << _T("x 1000");

         //for (DuctIndexType tendonIdx = 0; tendonIdx < nMaxSegmentTendons; tendonIdx++)
         //{
         //   (*table)(0, col++) << COLHDR(_T("Segment Tendon ") << LABEL_DUCT(tendonIdx) << rptNewLine << RPT_STRESS(_T("pe pt")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
         //   (*table)(0, col++) << _T("Segment Tendon ") << LABEL_DUCT(tendonIdx) << rptNewLine << Sub2(symbol(epsilon), _T("pti")) << rptNewLine << _T("x 1000");
         //}

         //for (DuctIndexType tendonIdx = 0; tendonIdx < nGirderTendons; tendonIdx++)
         //{
         //   (*table)(0, col++) << COLHDR(_T("Girder Tendon ") << LABEL_DUCT(tendonIdx) << rptNewLine << RPT_STRESS(_T("pe pt")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
         //   (*table)(0, col++) << _T("Girder Tendon ") << LABEL_DUCT(tendonIdx) << rptNewLine << Sub2(symbol(epsilon), _T("pti")) << rptNewLine << _T("x 1000");
         //}

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
   (*table)(0,col++) << COLHDR(_T("M") << Sub(_T("n")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0, col++) << _T("Controlling +");

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,    pDisplayUnits->GetGeneralForceUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment,   pDisplayUnits->GetMomentUnit(),       false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),       false );

   GET_IFACE2(pBroker, IDocumentType, pDocType);
   location.IncludeSpanAndGirder(pDocType->IsPGSpliceDocument() || girderKey.groupIndex == ALL_GROUPS);

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   rptRcScalar strain;
   strain.SetFormat( sysNumericFormatTool::Automatic);
   strain.SetWidth(7);
   strain.SetPrecision(3);

   Int16 count = 0;
   RowIndexType row = table->GetNumberOfHeaderRows();

   std::array<std::_tstring, 3> strControlling{ _T("C"), _T("T"), _T("D") };

   GET_IFACE2(pBroker,IMomentCapacity,pMomentCap);
   for (const pgsPointOfInterest& poi : vPoi)
   {
      const CSegmentKey& segmentKey(poi.GetSegmentKey());
      bool bIsOnSegment = pPoi->IsOnSegment(poi);

      const MOMENTCAPACITYDETAILS* pmcd = pMomentCap->GetMomentCapacityDetails(intervalIdx,poi,bPositiveMoment);

      col = 0;

      (*table)(row,col++) << location.SetValue( POI_SPAN, poi );
      (*table)(row,col++) << dim.SetValue( pmcd->c );
      (*table)(row,col++) << dim.SetValue( pmcd->dc );
      (*table)(row,col++) << dim.SetValue( pmcd->de );

      if ( bPositiveMoment )
      {
         (*table)(row,col++) << dim.SetValue( pmcd->de_shear );
      }

      (*table)(row,col++) << dim.SetValue( pmcd->dt );
      if ( lrfdVersionMgr::SixthEdition2012 <= lrfdVersionMgr::GetVersion() )
      {
         (*table)(row,col++) << strain.SetValue(pmcd->et * 1000.0);
      }

      if ( bPositiveMoment)
      {
         //(*table)(row,col++) << stress.SetValue( pmcd->fpe_ps );
         //(*table)(row,col++) << strain.SetValue(pmcd->eps_initial * 1000);

         //DuctIndexType nSegmentTendons = pSegmentTendonGeometry->GetDuctCount(segmentKey);
         //for ( DuctIndexType tendonIdx = 0; tendonIdx < nMaxSegmentTendons; tendonIdx++ )
         //{
         //   if (tendonIdx < nSegmentTendons && bIsOnSegment)
         //   {
         //      (*table)(row, col++) << stress.SetValue(pmcd->fpe_pt_segment[tendonIdx]);
         //      (*table)(row, col++) << strain.SetValue(pmcd->ept_initial_segment[tendonIdx] * 1000);
         //   }
         //   else
         //   {
         //      (*table)(row, col++) << _T("");
         //      (*table)(row, col++) << _T("");
         //   }
         //}

         //for (DuctIndexType tendonIdx = 0; tendonIdx < nGirderTendons; tendonIdx++)
         //{
         //   (*table)(row, col++) << stress.SetValue(pmcd->fpe_pt_girder[tendonIdx]);
         //   (*table)(row, col++) << strain.SetValue(pmcd->ept_initial_girder[tendonIdx] * 1000);
         //}

         (*table)(row,col++) << stress.SetValue( pmcd->fps_avg );

         if (0 < nMaxSegmentTendons)
         {
            (*table)(row, col++) << stress.SetValue(pmcd->fpt_avg_segment);
         }

         if ( 0 < nGirderTendons )
         {
            (*table)(row,col++) << stress.SetValue( pmcd->fpt_avg_girder );
         }

         if ( lrfdVersionMgr::GetVersion() <= lrfdVersionMgr::FifthEdition2010 )
         {
            (*table)(row,col++) << scalar.SetValue( pmcd->PPR );
         }
      }
      else
      {
         if ( 0 < nGirderTendons+nMaxSegmentTendons )
         {
            //(*table)(row,col++) << stress.SetValue( pmcd->fpe_ps );
            //(*table)(row,col++) << strain.SetValue(pmcd->eps_initial * 1000);

            //DuctIndexType nSegmentTendons = pSegmentTendonGeometry->GetDuctCount(segmentKey);
            //for (DuctIndexType tendonIdx = 0; tendonIdx < nMaxSegmentTendons; tendonIdx++)
            //{
            //   if (tendonIdx < nSegmentTendons && bIsOnSegment)
            //   {
            //      (*table)(row, col++) << stress.SetValue(pmcd->fpe_pt_segment[tendonIdx]);
            //      (*table)(row, col++) << strain.SetValue(pmcd->ept_initial_segment[tendonIdx] * 1000);
            //   }
            //   else
            //   {
            //      (*table)(row, col++) << _T("");
            //      (*table)(row, col++) << _T("");
            //   }
            //}

            //for ( DuctIndexType tendonIdx = 0; tendonIdx < nGirderTendons; tendonIdx++ )
            //{
            //   (*table)(row,col++) << stress.SetValue(pmcd->fpe_pt_girder[tendonIdx]);
            //   (*table)(row,col++) << strain.SetValue(pmcd->ept_initial_girder[tendonIdx]*1000);
            //}

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
      (*table)(row,col++) << moment.SetValue( pmcd->Mn );

      if (bConsiderReinforcementStrainLimits)
      {
      (*table)(row, col++) << strControlling[std::underlying_type<MOMENTCAPACITYDETAILS::ControllingType>::type(pmcd->Controlling)];
      }
      else
      {
         (*table)(row, col) << strControlling[std::underlying_type<MOMENTCAPACITYDETAILS::ControllingType>::type(pmcd->Controlling)];
         if (pmcd->Controlling != MOMENTCAPACITYDETAILS::ControllingType::Concrete)
            (*table)(row, col) << _T(", E");
         col++;
      }

      row++;
      count++;
   }

   pPara = new rptParagraph;
   *pChapter << pPara;

   if ( lrfdVersionMgr::GetVersion() <= lrfdVersionMgr::FifthEdition2010 )
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

void write_crack_moment_data_table(IBroker* pBroker,
                                   IEAFDisplayUnits* pDisplayUnits,
                             const CGirderKey& girderKey,
                                   const PoiList& vPoi,
                                   rptChapter* pChapter,
                                   IntervalIndexType intervalIdx,
                                   const CString& strStageName,
                                 bool bPositiveMoment)
{
   bool bAfter2002  = ( lrfdVersionMgr::SecondEditionWith2002Interims < lrfdVersionMgr::GetVersion()     ? true : false );
   bool bBefore2012 = ( lrfdVersionMgr::GetVersion()                  < lrfdVersionMgr::SixthEdition2012 ? true : false );

   // Setup the table
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << (bPositiveMoment ? _T("Positive") : _T("Negative")) << _T(" Cracking Moment Details [") << LrfdCw8th(_T("5.7.3.3.2"),_T("5.6.3.3")) << _T("] - ") << strStageName << rptNewLine;
  
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   ColumnIndexType nColumns = bAfter2002 ? 8 : 7;
   if ( lrfdVersionMgr::SixthEdition2012 <= lrfdVersionMgr::GetVersion() )
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
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(2);
   scalar.SetTolerance(1.0e-6);

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

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
         if ( lrfdVersionMgr::SixthEdition2012 <= lrfdVersionMgr::GetVersion() )
         {
            *pPara << rptNewLine;
            *pPara << _T("Flexural cracking variability factor, ") << Sub2(symbol(gamma),_T("1")) << _T(" = ") << scalar.SetValue(pcmd->g1) << rptNewLine;
            *pPara << _T("Prestress variability factor, ") << Sub2(symbol(gamma),_T("2")) << _T(" = ") << scalar.SetValue(pcmd->g2) << rptNewLine;
            *pPara << _T("Ratio of specified minimum yield strength to ultimate tensile strength of the reinforcement," ) << Sub2(symbol(gamma),_T("3")) << _T(" = ") << scalar.SetValue(pcmd->g3) << rptNewLine;
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
   bool bLambda = (lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion() ? true : false);

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

void write_min_moment_data_table(IBroker* pBroker,
                                 IEAFDisplayUnits* pDisplayUnits,
                             const CGirderKey& girderKey,
                                 const PoiList& vPoi,
                                 rptChapter* pChapter,
                                 IntervalIndexType intervalIdx,
                                 const CString& strStageName,
                                 bool bPositiveMoment)
{
   bool bBefore2012 = ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::SixthEdition2012 ? true : false );

   // Setup the table
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << _T("Minimum Reinforcement [") << LrfdCw8th(_T("5.7.3.3.2"),_T("5.6.3.3")) << _T("] - ") << strStageName << rptNewLine;

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

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

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

void write_over_reinforced_moment_data_table(IBroker* pBroker,
                                 IEAFDisplayUnits* pDisplayUnits,
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

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType lastCompositeDeckIntervalIdx = pIntervals->GetLastCompositeDeckInterval();
   if ( intervalIdx < lastCompositeDeckIntervalIdx)
   {
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   }
   else
   {
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   }

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

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

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
         (*table)(row,0) << location.SetValue( POI_SPAN, poi );
         (*table)(row,1) << scalar.SetValue( pmcd->Beta1Slab );
         (*table)(row,2) << stress.SetValue( pmcd->FcSlab );
         (*table)(row,3) << dim.SetValue( pmcd->b );
         (*table)(row,4) << dim.SetValue( pmcd->bw );
         (*table)(row,5) << dim.SetValue( pmcd->de );
         (*table)(row,6) << dim.SetValue( pmcd->hf );
         (*table)(row,7) << (pmcd->bRectSection ? _T("C5.7.3.3.1-1") : _T("C5.7.3.3.1-2"));
         (*table)(row,8) << moment.SetValue( pmcd->MnMin );
         (*table)(row,9) << moment.SetValue( pmcd->Phi * pmcd->MnMin );

         row++;
      }
   }
}