///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include <Reporting\ADimChapterBuilder.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\DeckElevationChapterBuilder.h>
#include <IFace\Constructability.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>
#include <IFace\DocumentType.h>
#include <IFace\Intervals.h>
#include <IFace\Alignment.h>

#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CADimChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CADimChapterBuilder::CADimChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CADimChapterBuilder::GetName() const
{
   return TEXT("Finished Roadway and Haunch Details");
}

rptChapter* CADimChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pGirderRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(spec_name.c_str());

   GET_IFACE2(pBroker,IBridge,pBridge);
   pgsTypes::HaunchInputDepthType haunchInputType = pBridge->GetHaunchInputDepthType();

   if (!pSpecEntry->IsSlabOffsetCheckEnabled())
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;

      *pPara << _T("Finished Elevation and Haunch check disabled in Project Criteria. No analysis performed.") << rptNewLine;
      return pChapter;
   }
   else if (pBridge->GetDeckType() == pgsTypes::sdtNone)
   {
      // Use deck elevations report
      CDeckElevationChapterBuilder builder;
      builder.BuildNoDeckElevationContent(pChapter,pRptSpec,level);
      return pChapter;
   }
   else if (pgsTypes::hidACamber == haunchInputType)
   {
      BuildAdimContent(pChapter,pRptSpec,level,pBroker,girderKey,pSpecEntry);
   }
   else
   {
      ATLASSERT(pgsTypes::hidHaunchDirectly == haunchInputType || pgsTypes::hidHaunchPlusSlabDirectly == haunchInputType);
      BuildDirectHaunchElevationContent(pChapter, pRptSpec, level);
   }

      return pChapter;
}

void CADimChapterBuilder::BuildAdimContent(rptChapter * pChapter,const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level,IBroker* pBroker,const CGirderKey& girderKey,const SpecLibraryEntry* pSpecEntry) const
{
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   bool bIsSplicedGirder = (pDocType->IsPGSpliceDocument() ? true : false);

   GET_IFACE2(pBroker,ILossParameters,pLossParams);
   bool bTimeStepAnalysis = (pLossParams->GetLossMethod() == pgsTypes::TIME_STEP);

   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bHasElevAdj = pBridge->HasTemporarySupportElevationAdjustments();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, comp, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, compdim, pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, defl, pDisplayUnits->GetDeflectionUnit(), false );

   GET_IFACE2(pBroker,IGirderHaunch,pGdrHaunch);
   GET_IFACE2(pBroker, IProductLoads, pProductLoads);
   GET_IFACE2(pBroker, IGirder, pGdr);

   bool bTopFlangeShapeEffect = false;
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CSegmentKey segmentKey(girderKey, segIdx);
      pgsTypes::TopFlangeThickeningType tftType = pGdr->GetTopFlangeThickeningType(segmentKey);
      Float64 tft = pGdr->GetTopFlangeThickening(segmentKey);
      if (tftType != pgsTypes::tftNone && !IsZero(tft))
      {
         bTopFlangeShapeEffect = true;
      }
   }

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* pTable1 = rptStyleManager::CreateDefaultTable(bTopFlangeShapeEffect ? 12 : 11, _T("Haunch Details - Part 1"));
   *pPara << pTable1;

   pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pPara;
   *pPara << _T("Design Elevation = Elevation of the roadway surface directly above the centerline of the girder.") << rptNewLine;
   *pPara << _T("Profile Chord Elevation = Elevation of an imaginary chord that intersects the roadway surface above the point of bearing at the both ends of the girder.") << rptNewLine;
   *pPara << _T("Profile Effect = Design Elevation - Profile Chord Elevation") << rptNewLine;

   rptRcTable* pTable2 = rptStyleManager::CreateDefaultTable(bHasElevAdj ? 11 : 10, _T("Haunch Details - Part 2"));
   *pPara << pTable2;

   std::_tstring strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

   ColumnIndexType col = 0;
   (*pTable1)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable1)(0, col++) << _T("Station");
   (*pTable1)(0, col++) << COLHDR(_T("Offset"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable1)(0, col++) << COLHDR(_T("Design") << rptNewLine << _T("Elevation"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable1)(0, col++) << COLHDR(_T("Profile") << rptNewLine << _T("Chord") << rptNewLine << _T("Elevation"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable1)(0, col++) << COLHDR(_T("Profile") << rptNewLine << _T("Effect"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable1)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftSlab) << rptNewLine << _T("Thickness"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable1)(0, col++) << COLHDR(_T("Fillet"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   if (bTopFlangeShapeEffect)
   {
      (*pTable1)(0, col++) << COLHDR(_T("Top Flange") << rptNewLine << _T("Shape Effect"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   }

   Float64 days;
   if (bTimeStepAnalysis)
   {
      GET_IFACE2(pBroker, IIntervals, pIntervals);
      IntervalIndexType castDeckIntervalIdx = pIntervals->GetFirstCastDeckInterval();
      Float64 start_time = pIntervals->GetTime(0, pgsTypes::Start);
      Float64 deck_casting = pIntervals->GetTime(castDeckIntervalIdx, pgsTypes::Start);
      days = deck_casting - start_time + 1;
   }
   else
   {
      days = pSpecEntry->GetCreepDuration2Max(); // haunch is always computined using max time for construction
      days = WBFL::Units::ConvertFromSysUnits(days, WBFL::Units::Measure::Day);
   }
   std::_tostringstream os;
   os << days;
   (*pTable1)(0, col++) << COLHDR(Sub2(_T("D"), os.str().c_str()), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   (*pTable1)(0, col++) << COLHDR(_T("C"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable1)(0, col++) << COLHDR(_T("Computed") << rptNewLine << _T("Excess") << rptNewLine << _T("Camber"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   col = 0;
   (*pTable2)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable2)(0, col++) << _T("Deck") << rptNewLine << _T("Slope") << rptNewLine << _T("(") << Sub2(_T("m"), _T("d")) << _T(")") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
   (*pTable2)(0, col++) << _T("Girder Top") << rptNewLine << _T("Slope") << rptNewLine << _T("(") << Sub2(_T("m"), _T("g")) << _T(")") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
   (*pTable2)(0, col++) << COLHDR(_T("Top") << rptNewLine << _T("Width"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable2)(0, col++) << COLHDR(_T("Girder") << rptNewLine << _T("Orientation") << rptNewLine << _T("Effect"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   if (bHasElevAdj)
   {
      (*pTable2)(0, col++) << COLHDR(_T("Temp. Support") << rptNewLine << _T("Elevation") << rptNewLine << _T("Adjustment"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   }
   (*pTable2)(0, col++) << COLHDR(_T("Required") << rptNewLine << _T("Slab") << rptNewLine << _T("Offset") << (_T("*")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable2)(0, col++) << COLHDR(_T("Top") << rptNewLine << _T("Girder") << rptNewLine << _T("Elevation") << Super(_T("**")), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable2)(0, col++) << COLHDR(_T("Actual") << rptNewLine << _T("Depth") << Super(_T("***")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable2)(0, col++) << COLHDR(_T("CL") << rptNewLine << _T("Haunch") << rptNewLine << _T("Depth") << Super(_T("&")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable2)(0, col++) << COLHDR(_T("Least") << rptNewLine << _T("Haunch") << rptNewLine << _T("Depth") << Super(_T("&&")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   RowIndexType row1 = pTable1->GetNumberOfHeaderRows();
   RowIndexType row2 = pTable2->GetNumberOfHeaderRows();

   // on entry per segment
   std::vector<Float64> vRequiredSlabOffset;
   std::vector<Float64> vMaxHaunchDiff;
   vRequiredSlabOffset.resize(nSegments, 0.0);
   vMaxHaunchDiff.resize(nSegments, 0.0);

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS || 0 < nSegments);

   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CSegmentKey segmentKey(girderKey, segIdx);

      const auto& slab_offset_details = pGdrHaunch->GetSlabOffsetDetails(segmentKey);

      for ( const auto& slab_offset : slab_offset_details.SlabOffset)
      {
         col = 0;

         if (bIsSplicedGirder)
         {
            (*pTable1)(row1, col++) << location.SetValue(POI_ERECTED_SEGMENT, slab_offset.PointOfInterest);
         }
         else
         {
            (*pTable1)(row1, col++) << location.SetValue(POI_SPAN, slab_offset.PointOfInterest);
         }

         (*pTable1)(row1,col++) << rptRcStation(slab_offset.Station, &pDisplayUnits->GetStationFormat() );
         (*pTable1)(row2,col++) << RPT_OFFSET(slab_offset.Offset,dim);

         (*pTable1)(row1,col++) << dim.SetValue(slab_offset.ElevAlignment );
         (*pTable1)(row1,col++) << dim.SetValue(slab_offset.ElevGirderChord );
         (*pTable1)(row1,col++) << comp.SetValue(slab_offset.ProfileEffect);
         (*pTable1)(row1,col++) << comp.SetValue(slab_offset.tSlab );
         (*pTable1)(row1,col++) << comp.SetValue(slab_offset.Fillet );

         if (bTopFlangeShapeEffect)
         {
            (*pTable1)(row1, col++) << defl.SetValue(slab_offset.TopFlangeShapeEffect);
         }

         (*pTable1)(row1,col++) << defl.SetValue(slab_offset.D );
         (*pTable1)(row1,col++) << defl.SetValue(slab_offset.C );
         (*pTable1)(row1, col++) << defl.SetValue(slab_offset.CamberEffect);

         row1++;

         col = 0;
         if (bIsSplicedGirder)
         {
            (*pTable2)(row2, col++) << location.SetValue(POI_ERECTED_SEGMENT, slab_offset.PointOfInterest);
         }
         else
         {
            (*pTable2)(row2, col++) << location.SetValue(POI_SPAN, slab_offset.PointOfInterest);
         }

         (*pTable2)(row2,col++) << slab_offset.CrownSlope;
         (*pTable2)(row2,col++) << slab_offset.GirderTopSlope;
         (*pTable2)(row2,col++) << comp.SetValue(slab_offset.Wtop );
         (*pTable2)(row2,col++) << comp.SetValue(slab_offset.GirderOrientationEffect );
         if (bHasElevAdj)
         {
            (*pTable2)(row2, col++) << comp.SetValue(slab_offset.ElevAdjustment);
         }
         (*pTable2)(row2,col++) << comp.SetValue(slab_offset.RequiredSlabOffsetRaw);
         (*pTable2)(row2,col++) << dim.SetValue(slab_offset.ElevTopGirder);
         (*pTable2)(row2,col++) << comp.SetValue(slab_offset.TopSlabToTopGirder );

         Float64 dHaunch = slab_offset.TopSlabToTopGirder - slab_offset.tSlab;
         if ( dHaunch+TOLERANCE < slab_offset.Fillet )
         {
            (*pTable2)(row2,col++) << color(Red) << comp.SetValue( dHaunch ) << color(Black);
         }
         else
         {
            (*pTable2)(row2,col++) << comp.SetValue( dHaunch );
         }

         Float64 dHaunchMin;
         if (slab_offset.GirderOrientationEffect < 0.0 )
         {
            dHaunchMin = slab_offset.TopSlabToTopGirder; // cl girder in a valley
         }
         else
         {
            dHaunchMin = slab_offset.TopSlabToTopGirder - slab_offset.tSlab - slab_offset.GirderOrientationEffect;
         }

         if ( dHaunchMin+TOLERANCE < slab_offset.Fillet )
         {
            (*pTable2)(row2,col++) << color(Red) << comp.SetValue( dHaunchMin ) << color(Black);
         }
         else
         {
            (*pTable2)(row2,col++) << comp.SetValue( dHaunchMin );
         }

         row2++;
      }
      vRequiredSlabOffset[segIdx] = slab_offset_details.RequiredMaxSlabOffsetRounded;
      vMaxHaunchDiff[segIdx] = slab_offset_details.HaunchDiff;
   } // next segment

   comp.ShowUnitTag(true);

   // table footnotes
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   pgsTypes::SlabOffsetType slabOffsetType = pBridgeDesc->GetSlabOffsetType();

   pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pPara;

   *pPara << _T("Deck Slope (") << Sub2(_T("m"), _T("d")) << _T(") and Girder Top Slope (") << Sub2(_T("m"), _T("g")) << _T(") are positive when the slope is upwards towards the right") << rptNewLine;
   *pPara << Super(_T("*")) << _T(" required slab offset (equal at all support locations) from top of girder to top of deck at centerline bearing for geometric effects at this point. (Slab Thickness + Fillet");
   if (bTopFlangeShapeEffect)
   {
      *pPara << _T(" + Top Flange Shape Effect");
   }
   if (bHasElevAdj)
   {
      *pPara << _T(" - Temporary Support Elevation Adjustment");
   }
   *pPara << _T(" + Excess Camber + Profile Effect + Girder Orientation Effect)") << rptNewLine;

   if (slabOffsetType == pgsTypes::sotBridge)
   {
      // one slab offset for whole bridge
      Float64 slab_offset = pBridgeDesc->GetSlabOffset();
      *pPara << Super(_T("**")) << _T(" includes the effects of camber and based on Slab Offset of ") << comp.SetValue(slab_offset) << _T(".") << rptNewLine;
      *pPara << Super(_T("***")) << _T(" top of girder to top of deck based on Slab Offset of ") << comp.SetValue(slab_offset) << _T(". (Design Elevation - Top Girder Elevation)") << rptNewLine;
   }
   else
   {
      LPCTSTR lpszType = bIsSplicedGirder ? _T("segment") : _T("girder");
      if (nSegments == 1)
      {
         const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
         PierIndexType startPierIdx = pGroup->GetPierIndex(pgsTypes::metStart);
         PierIndexType endPierIdx = pGroup->GetPierIndex(pgsTypes::metEnd);

         std::array<Float64, 2> slab_offset;
         pBridge->GetSlabOffset(CSegmentKey(girderKey, 0), &slab_offset[pgsTypes::metStart],&slab_offset[pgsTypes::metEnd]);

         *pPara << Super(_T("**")) << _T(" includes the effects of camber and based on Slab Offset at the start of the ") << lpszType << _T(" of ") << comp.SetValue(slab_offset[pgsTypes::metStart]);
         *pPara << _T(" and a Slab Offset at the end of the girder of ") << comp.SetValue(slab_offset[pgsTypes::metEnd]) << _T(".") << rptNewLine;

         *pPara << Super(_T("***")) << _T(" actual depth from top C.L. of girder to top of deck based on Slab Offset at the start of the ") << lpszType << _T(" of ") << comp.SetValue(slab_offset[pgsTypes::metStart]);
         *pPara << _T(" and a Slab Offset at the end of the girder of ") << comp.SetValue(slab_offset[pgsTypes::metEnd]) << _T(". (Design Elevation - Top Girder Elevation)") << rptNewLine;
      }
      else
      {
         // can't list all of the slab offsets at the ends of all the segments in the footnote.
         *pPara << Super(_T("**")) << _T(" includes the effects of camber and based on Slab Offset at the start and end of the ") << lpszType << _T(".") << rptNewLine;
         *pPara << Super(_T("***")) << _T(" actual depth from top C.L. of girder to top of deck based on Slab Offset at the start and end of the ") << lpszType << _T(". (Design Elevation - Top Girder Elevation)") << rptNewLine;
      }
   }

   *pPara << Super(_T("&")) << _T(" CL Haunch Depth = Haunch Depth along the centerline of girder") << rptNewLine;
   *pPara << Super(_T("&&")) << _T(" Least Haunch Depth = Haunch Depth at Edge of Top Flange = CL Haunch Depth - Girder Orientation Effect") << rptNewLine;

   // this is not footnote text.... need a new paragraph with the regular style
   pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << rptNewLine;

   CString strRounding;
   pgsTypes::SlabOffsetRoundingMethod Method;
   Float64 Tolerance;
   pSpecEntry->GetRequiredSlabOffsetRoundingParameters(&Method, &Tolerance);
   if (pgsTypes::sormRoundNearest == Method)
   {
      strRounding = _T(", rounded to the nearest ");
   }
   else
   {
      strRounding = _T(", rounded upward to the nearest ");
   }

   comp.ShowUnitTag(true);
   if (nSegments == 1)
   {
      *pPara << _T("Required Slab Offset at intersection of centerline bearings and centerline girder: ") << comp.SetValue(vRequiredSlabOffset.front());
      *pPara << _T(" (") << comp.SetValue(vRequiredSlabOffset.front()) << strRounding << compdim.SetValue(Tolerance) << _T(")") << rptNewLine;
      *pPara << _T("Maximum Change in CL Haunch Depth along girder: ") << comp.SetValue(vMaxHaunchDiff.front()) << rptNewLine;
   }
   else
   {
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
      {
         *pPara << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
         *pPara << _T("Required Slab Offset at intersection of centerline bearings and centerline segment: ") << comp.SetValue(vRequiredSlabOffset[segIdx]);
         *pPara << _T(" (") << comp.SetValue(vRequiredSlabOffset[segIdx]) << strRounding << compdim.SetValue(Tolerance) << _T(")") << rptNewLine;
         *pPara << _T("Maximum Change in CL Haunch Depth along segment: ") << comp.SetValue(vMaxHaunchDiff[segIdx]) << rptNewLine << rptNewLine;
      }
   }

   pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << rptNewLine;

   *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("ProfileEffect.gif")) << rptNewLine;
   *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("GirderOrientationEffect.png"));
   *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("GirderOrientationEffectEquation.png"))  << rptNewLine;
}

class MatchPoiOffSegment
{
public:
   MatchPoiOffSegment(IPointOfInterest* pIPointOfInterest) : m_pIPointOfInterest(pIPointOfInterest) {}
   bool operator()(const pgsPointOfInterest& poi) const
   {
      return m_pIPointOfInterest->IsOffSegment(poi);
   }

   IPointOfInterest* m_pIPointOfInterest;
};

void CADimChapterBuilder::BuildDirectHaunchElevationContent(rptChapter* pChapter,const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   const CBrokerReportSpecification* pSpec = dynamic_cast<const CBrokerReportSpecification*>(pRptSpec.get());
   CComPtr<IBroker> pBroker;
   pSpec->GetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   bool bIsSplicedGirder = (pDocType->IsPGSpliceDocument() ? true : false);

   const CGirderReportSpecification* pSGRptSpec = dynamic_cast<const CGirderReportSpecification*>(pRptSpec.get());

   CGirderKey girderKey;
   if (pSGRptSpec)
   {
      girderKey = pSGRptSpec->GetGirderKey();
   }

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pPara << _T("Design and Finished Elevations") << rptNewLine;
   (*pChapter) << pPara;

   pPara = new rptParagraph();
   (*pChapter) << pPara;
   *pPara << _T("Offsets are measured from and normal to the left edge, centerline, and right edge of girder at top of girder.") << rptNewLine;
   *pPara << _T("Design elevations are the roadway deck surface elevations defined by the alignment, profile, and superelevation geometry.") << rptNewLine;
   *pPara << _T("Finished elevations are the top of the finished deck, or overlay if applicable, including deflection effects.") << rptNewLine;

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pISpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pISpec->GetSpecification().c_str());
   Float64 tolerance = pSpecEntry->GetFinishedElevationTolerance();

   INIT_UV_PROTOTYPE(rptLengthSectionValue,dim1,pDisplayUnits->GetSpanLengthUnit(),true);
   INIT_UV_PROTOTYPE(rptLengthSectionValue,dim2,pDisplayUnits->GetComponentDimUnit(),true);
   INIT_UV_PROTOTYPE(rptLengthSectionValue,dimH,pDisplayUnits->GetComponentDimUnit(),false);
   *pPara << _T("Finished elevations in red text differ from the design elevation by more than ") << symbol(PLUS_MINUS) << dim1.SetValue(tolerance) << _T(" (") << symbol(PLUS_MINUS) << dim2.SetValue(tolerance) << _T(")") << rptNewLine;

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 fillet = pBridge->GetFillet();
   *pPara << _T("Haunch dimensions in red text are less than the fillet value of ") << dim2.SetValue(fillet) << rptNewLine;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CDeckDescription2* pDeck = pIBridgeDesc->GetDeckDescription();

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType gce_interval = pIntervals->GetGeometryControlInterval();
   IntervalIndexType overlay_interval = pIntervals->GetOverlayInterval();

   Float64 overlay = 0;
   if (pDeck->WearingSurface == pgsTypes::wstOverlay)
   {
      overlay = pBridge->GetOverlayDepth(gce_interval);

      if (pDeck->bInputAsDepthAndDensity == false)
      {
         INIT_UV_PROTOTYPE(rptLengthSectionValue,olay,pDisplayUnits->GetComponentDimUnit(),true);
         *pPara << _T("Overlay is defined by weight. Overlay depth assumed to be ") << olay.SetValue(overlay) << _T(".") << rptNewLine;
      }
   }

   INIT_UV_PROTOTYPE(rptPointOfInterest,location,pDisplayUnits->GetSpanLengthUnit(),false);
   INIT_UV_PROTOTYPE(rptLengthSectionValue,webDim,pDisplayUnits->GetSpanLengthUnit(),false);
   INIT_UV_PROTOTYPE(rptLengthSectionValue,dist,pDisplayUnits->GetSpanLengthUnit(),false);
   INIT_UV_PROTOTYPE(rptLengthSectionValue,cogoPoint,pDisplayUnits->GetAlignmentLengthUnit(),true);

   GET_IFACE2(pBroker,IRoadway,pAlignment);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2(pBroker,IDeformedGirderGeometry,pDeformedGirderGeometry);
   GET_IFACE2(pBroker,IGirder,pGirder);
   std::vector<IntervalIndexType> vSpecIntervals = pIntervals->GetSpecCheckGeometryControlIntervals();
   std::vector<IntervalIndexType>::iterator iti = vSpecIntervals.begin();
   *pPara << _T("The Geometry Control Event interval is interval ") << LABEL_INTERVAL(gce_interval) << _T(". Finished Roadway Spec checks are performed for interval(s): ") <<  LABEL_INTERVAL(*iti++);
   while(iti != vSpecIntervals.end())
   {
      *pPara << _T(", ") << LABEL_INTERVAL(*iti++);
   }
   *pPara << rptNewLine;

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType startGroupIdx = girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex;
   GroupIndexType endGroupIdx   = girderKey.groupIndex == ALL_GROUPS ? nGroups : girderKey.groupIndex;

   // loop over all reporting intervals
   std::vector<IntervalIndexType> vIntervals = pIntervals->GetReportingGeometryControlIntervals();
   for (auto interval : vIntervals)
   {

      for (GroupIndexType groupIdx = startGroupIdx; groupIdx <= endGroupIdx; groupIdx++)
      {
         GirderIndexType nGirders = pBridge->GetGirderCount(groupIdx);
         GirderIndexType startGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex);
         GirderIndexType endGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? nGirders - 1 : startGirderIdx);

         for (GirderIndexType gdrIdx = startGirderIdx; gdrIdx <= endGirderIdx; gdrIdx++)
         {
            SegmentIndexType nSegments = pBridge->GetSegmentCount(groupIdx,gdrIdx);

            for (SegmentIndexType segmentIdx = 0; segmentIdx < nSegments; segmentIdx++)
            {
               CSegmentKey segmentKey(groupIdx,gdrIdx,segmentIdx);
               PoiList vPoi;
               pPoi->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_TENTH_POINTS,&vPoi);
               pPoi->GetPointsOfInterest(segmentKey,POI_SPAN | POI_0L | POI_10L,&vPoi);
               pPoi->GetPointsOfInterest(segmentKey,POI_START_FACE | POI_END_FACE,&vPoi);
               // We can't compute accurate elevations within closure joints. Get rid of any pois off segments
               vPoi.erase(std::remove_if(vPoi.begin(),vPoi.end(),MatchPoiOffSegment(pPoi)),std::end(vPoi));
               pPoi->SortPoiList(&vPoi); // sorts and removes duplicates

               std::_tostringstream os;
               if (bIsSplicedGirder)
               {
                  os << _T("Group ") << LABEL_GROUP(groupIdx) << _T(", Segment ") << LABEL_SEGMENT(segmentIdx) << _T(", Girder ") << LABEL_GIRDER(gdrIdx) << _T(" - Interval ") << LABEL_INTERVAL(interval) << _T(": ") << pIntervals->GetDescription(interval) << std::endl;
               }
               else
               {
                  os << _T("Span ") << LABEL_SPAN(groupIdx) << _T(", Girder ") << LABEL_GIRDER(gdrIdx) << _T(" - Interval ") << LABEL_INTERVAL(interval) << _T(": ") << pIntervals->GetDescription(interval) << std::endl;
               }

               rptRcTable* pTable = rptStyleManager::CreateDefaultTable(vPoi.size() + 1,os.str().c_str());
               (*pPara) << pTable << rptNewLine;

               pTable->SetNumberOfHeaderRows(1);

               ColumnIndexType col = 0;
               for (col = 0; col < 3; col++)
               {
                  pTable->SetColumnStyle(col,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
                  pTable->SetStripeRowColumnStyle(col,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
               }

               RowIndexType row = 0;
               col = 0;
               (*pTable)(row++,col) << Bold(_T("Location from") << rptNewLine << _T("Left Support (")) << Bold(pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag()) << Bold(_T(")"));
               (*pTable)(row++,col) << Bold(_T("Station"));
               (*pTable)(row++,col) << Bold(_T("Offset (")) << Bold(pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag()) << Bold(_T(")"));
               (*pTable)(row++,col) << Bold(_T("Design Elev (")) << Bold(pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag()) << Bold(_T(")"));
               (*pTable)(row++,col) << Bold(_T("Finished Elev (")) << Bold(pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag()) << Bold(_T(")"));
               (*pTable)(row++,col) << Bold(_T("Elev Difference (")) << Bold(pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag()) << Bold(_T(")"));
               (*pTable)(row++,col) << Bold(_T("Elev Difference (")) << Bold(pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag()) << Bold(_T(")"));
               (*pTable)(row++,col) << Bold(_T("Elev Top CL Girder (")) << Bold(pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag()) << Bold(_T(")"));
               (*pTable)(row++,col) << Bold(_T("Elev CL Girder Chord (")) << Bold(pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag()) << Bold(_T(")"));
               (*pTable)(row++,col) << Bold(_T("Left Haunch (")) << Bold(pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag()) << Bold(_T(")"));
               (*pTable)(row++,col) << Bold(_T("CL Haunch (")) << Bold(pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag()) << Bold(_T(")"));
               (*pTable)(row++,col) << Bold(_T("Right Haunch (")) << Bold(pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag()) << Bold(_T(")"));
               (*pTable)(row++,col) << Bold(_T("Min Haunch - Fillet (")) << Bold(pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag()) << Bold(_T(")"));

               col = 1;
               for (const auto& poi : vPoi)
               {
                  row = 0;

                  // get parameters for design elevation
                  // CL Girder location
                  CComPtr<IPoint2d> point_on_cl_girder;
                  pBridge->GetPoint(poi,pgsTypes::pcLocal,&point_on_cl_girder);

                  Float64 station,offset;
                  pAlignment->GetStationAndOffset(pgsTypes::pcLocal,point_on_cl_girder,&station,&offset);

                  Float64 design_elevation = pAlignment->GetElevation(station,offset);

                  CSpanKey spanKey;
                  Float64 spanX;
                  pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&spanX);

                  std::_tstring str = poi.get().GetAttributes(POI_SPAN,false);
                  if (!str.empty())
                  {
                     (*pTable)(row++,col) << _T("(") << str << _T(")") << rptNewLine << dist.SetValue(spanX);
                  }
                  else
                  {
                     (*pTable)(row++,col) << rptNewLine << dist.SetValue(spanX);
                  }

                  (*pTable)(row++,col) << rptRcStation(station,&pDisplayUnits->GetStationFormat());
                  (*pTable)(row++,col) << RPT_OFFSET(offset,dist);
                  (*pTable)(row++,col) << dist.SetValue(design_elevation);

                  // get parameters for finished elevation... for no deck, the finished elevation is the top of the girder
                  Float64 lftHaunch,clHaunch,rgtHaunch;
                  Float64 finished_elevation = pDeformedGirderGeometry->GetFinishedElevation(poi,interval,&lftHaunch,&clHaunch,&rgtHaunch);

                  // if finished elevation differs from design_elevation by more than tolerance, use red text for the finished_elevation
                  rptRiStyle::FontColor color = IsEqual(design_elevation,finished_elevation,tolerance) ? rptRiStyle::Black : rptRiStyle::Red;
                  rptRcColor* pColor = new rptRcColor(color);
                  (*pTable)(row++,col) << pColor << dist.SetValue(finished_elevation) << color(Black);

                  pColor = new rptRcColor(color);
                  (*pTable)(row++,col) << pColor << dist.SetValue(finished_elevation - design_elevation) << color(Black);

                  pColor = new rptRcColor(color);
                  (*pTable)(row++,col) << pColor << dimH.SetValue(finished_elevation - design_elevation) << color(Black);

                  Float64 topGdrLeft,topGirderCl,topGirderRght;
                  pDeformedGirderGeometry->GetTopGirderElevationEx(poi,interval,nullptr,&topGdrLeft,&topGirderCl,&topGirderRght);

                  (*pTable)(row++,col) << dist.SetValue(topGirderCl);

                  Float64 girderChord = pGirder->GetTopGirderChordElevation(poi);
                  (*pTable)(row++,col) << dist.SetValue(girderChord);

                  // haunch depths
                  color = lftHaunch + TOLERANCE > fillet ? rptRiStyle::Black : rptRiStyle::Red;
                  pColor = new rptRcColor(color);
                  (*pTable)(row++,col) << pColor << dimH.SetValue(lftHaunch) << color(Black);

                  color = clHaunch + TOLERANCE > fillet ? rptRiStyle::Black : rptRiStyle::Red;
                  pColor = new rptRcColor(color);
                  (*pTable)(row++,col) << pColor << Bold(dimH.SetValue(clHaunch)) << color(Black);

                  color = rgtHaunch + TOLERANCE > fillet ? rptRiStyle::Black : rptRiStyle::Red;
                  pColor = new rptRcColor(color);
                  (*pTable)(row++,col) << pColor << dimH.SetValue(rgtHaunch) << color(Black);

                  // Report min haunch
                  Float64 minHaunch = min(clHaunch,min(lftHaunch,rgtHaunch)) - fillet;
                  color = minHaunch + TOLERANCE > 0 ? rptRiStyle::Black : rptRiStyle::Red;
                  pColor = new rptRcColor(color);
                  (*pTable)(row++,col) << pColor << dimH.SetValue(minHaunch) << color(Black);

                  col++;
               } // next poi
            } // segments
         } // next girder
      } // next group
   } // next interval
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CADimChapterBuilder::Clone() const
{
   return std::make_unique<CADimChapterBuilder>();
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
