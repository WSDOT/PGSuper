///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
#include <IFace\Constructability.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>

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
   return TEXT("Haunch Details");
}

rptChapter* CADimChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,ILibrary, pLib );
   GET_IFACE2(pBroker,ISpecification, pSpec );
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   GET_IFACE2_NOCHECK(pBroker,IBridge,pBridge);
   if ( !pSpecEntry->IsSlabOffsetCheckEnabled() )
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;

      *pPara << _T("Slab Offset check disabled in Project Criteria. No analysis performed.") << rptNewLine;
      return pChapter;
   }
   else if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;

      *pPara << _T("This bridge does not have a deck. No analysis performed.") << rptNewLine;
      return pChapter;
   }

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, comp, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, defl, pDisplayUnits->GetDeflectionUnit(), false );

   GET_IFACE2(pBroker,IGirderHaunch,pGdrHaunch);
   GET_IFACE2(pBroker, IProductLoads, pProductLoads);
   GET_IFACE2(pBroker, IGirder, pGdr);

   Float64 haunch_tolerance;
   if (IS_SI_UNITS(pDisplayUnits))
   {
      haunch_tolerance = HAUNCH_TOLERANCE_SI;
   }
   else
   {
      haunch_tolerance = HAUNCH_TOLERANCE_US;
   }

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CSegmentKey segmentKey(girderKey, segIdx);

      if (1 < nSegments)
      {
         rptParagraph* pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         *pChapter << pPara;
         (*pPara) << _T("Segment ") << LABEL_SEGMENT(segmentKey.segmentIndex) << rptNewLine;
      }

      const auto& haunch_details = pGdrHaunch->GetHaunchDetails(segmentKey);

      bool bTopFlangeShapeEffect = false;
      pgsTypes::TopFlangeThickeningType tftType = pGdr->GetTopFlangeThickeningType(segmentKey);
      Float64 tft = pGdr->GetTopFlangeThickening(segmentKey);
      if (tftType != pgsTypes::tftNone && !IsZero(tft))
      {
         bTopFlangeShapeEffect = true;
      }

      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      
      rptRcTable* pTable1 = rptStyleManager::CreateDefaultTable(bTopFlangeShapeEffect ? 12 : 11,_T("Haunch Details - Part 1"));
      *pPara << pTable1;

      pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
      *pChapter << pPara;
      *pPara << _T("Finished Grade Elevation = Elevation of the roadway surface directly above the centerline of the girder.") << rptNewLine;
      *pPara << _T("Profile Chord Elevation = Elevation of an imaginary chord that intersects the roadway surface above the point of bearing at the both ends of the girder.") << rptNewLine;
      *pPara << _T("Profile Effect = Finished Grade Elevation - Profile Chord Elevation") << rptNewLine;

      rptRcTable* pTable2 = rptStyleManager::CreateDefaultTable(10,_T("Haunch Details - Part 2"));
      *pPara << pTable2;

      std::_tstring strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

      ColumnIndexType col = 0;
      (*pTable1)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*pTable1)(0, col++) << _T("Station");
      (*pTable1)(0, col++) << COLHDR(_T("Offset"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*pTable1)(0, col++) << COLHDR(_T("Finished") << rptNewLine << _T("Grade") << rptNewLine << _T("Elevation"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*pTable1)(0, col++) << COLHDR(_T("Profile") << rptNewLine << _T("Chord") << rptNewLine << _T("Elevation"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*pTable1)(0, col++) << COLHDR(_T("Profile") << rptNewLine << _T("Effect"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable1)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftSlab) << rptNewLine << _T("Thickness"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable1)(0, col++) << COLHDR(_T("Fillet"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      if (bTopFlangeShapeEffect)
      {
         (*pTable1)(0, col++) << COLHDR(_T("Top Flange") << rptNewLine << _T("Shape Effect"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      }

      Float64 days = pSpecEntry->GetCreepDuration2Max(); // haunch is always computined using max time for construction
      days = ::ConvertFromSysUnits(days, unitMeasure::Day);
      std::_tostringstream os;
      os << days;
      (*pTable1)(0, col++) << COLHDR(Sub2(_T("D"), os.str().c_str()),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

      (*pTable1)(0, col++) << COLHDR(_T("C"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable1)(0, col++) << COLHDR(_T("Computed") << rptNewLine << _T("Excess") << rptNewLine << _T("Camber"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

      col = 0;
      (*pTable2)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*pTable2)(0, col++) << _T("Deck") << rptNewLine << _T("Slope") << rptNewLine << _T("(") << Sub2(_T("m"),_T("d")) << _T(")") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
      (*pTable2)(0, col++) << _T("Girder Top") << rptNewLine << _T("Slope") << rptNewLine << _T("(") << Sub2(_T("m"),_T("g")) << _T(")") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
      (*pTable2)(0, col++)<< COLHDR(_T("Top") << rptNewLine << _T("Width"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0, col++)<< COLHDR(_T("Girder") << rptNewLine << _T("Orientation") << rptNewLine << _T("Effect"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0, col++)<< COLHDR(_T("Required") << rptNewLine << _T("Slab") << rptNewLine << _T("Offset") << (_T("*")),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0, col++)<< COLHDR(_T("Top") << rptNewLine << _T("Girder") << rptNewLine << _T("Elevation") << Super(_T("**")),rptLengthUnitTag,pDisplayUnits->GetSpanLengthUnit());
      (*pTable2)(0, col++)<< COLHDR(_T("Actual") << rptNewLine << _T("Depth") << Super(_T("***")),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0, col++)<< COLHDR(_T("CL") << rptNewLine << _T("Haunch") << rptNewLine << _T("Depth") << Super(_T("&")),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable2)(0, col++)<< COLHDR(_T("Least") << rptNewLine << _T("Haunch") << rptNewLine << _T("Depth") << Super(_T("&&")),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

      RowIndexType row1 = pTable1->GetNumberOfHeaderRows();
      RowIndexType row2 = pTable2->GetNumberOfHeaderRows();
      for ( const auto& haunch : haunch_details.Haunch)
      {
         col = 0;

         (*pTable1)(row1,col++) << location.SetValue(POI_ERECTED_SEGMENT, haunch.PointOfInterest );
         (*pTable1)(row1,col++) << rptRcStation(haunch.Station, &pDisplayUnits->GetStationFormat() );
         (*pTable1)(row2,col++) << RPT_OFFSET(haunch.Offset,dim);

         (*pTable1)(row1,col++) << dim.SetValue( haunch.ElevAlignment );
         (*pTable1)(row1,col++) << dim.SetValue( haunch.ElevGirder );
         (*pTable1)(row1,col++) << comp.SetValue(haunch.ProfileEffect);
         (*pTable1)(row1,col++) << comp.SetValue( haunch.tSlab );
         (*pTable1)(row1,col++) << comp.SetValue( haunch.Fillet );

         if (bTopFlangeShapeEffect)
         {
            (*pTable1)(row1, col++) << defl.SetValue(haunch.TopFlangeShapeEffect);
         }

         (*pTable1)(row1,col++) << defl.SetValue( haunch.D );
         (*pTable1)(row1,col++) << defl.SetValue( haunch.C );
         (*pTable1)(row1, col++) << defl.SetValue(haunch.CamberEffect);

         row1++;

         col = 0;
         (*pTable2)(row2,col++) << location.SetValue(POI_ERECTED_SEGMENT, haunch.PointOfInterest );
         (*pTable2)(row2,col++) << haunch.CrownSlope;
         (*pTable2)(row2,col++) << haunch.GirderTopSlope;
         (*pTable2)(row2,col++) << comp.SetValue( haunch.Wtop );
         (*pTable2)(row2,col++) << comp.SetValue( haunch.GirderOrientationEffect );
         (*pTable2)(row2,col++) << comp.SetValue( haunch.RequiredHaunchDepth );
         (*pTable2)(row2,col++) << dim.SetValue( haunch.ElevTopGirder);
         (*pTable2)(row2,col++) << comp.SetValue( haunch.TopSlabToTopGirder );
         Float64 dHaunch = haunch.TopSlabToTopGirder - haunch.tSlab;
         if ( dHaunch < -haunch_tolerance )
         {
            (*pTable2)(row2,col++) << color(Red) << comp.SetValue( dHaunch ) << color(Black);
         }
         else
         {
            (*pTable2)(row2,col++) << comp.SetValue( dHaunch );
         }

         Float64 dHaunchMin;
         if ( haunch.GirderOrientationEffect < 0.0 )
         {
            dHaunchMin = haunch.TopSlabToTopGirder; // cl girder in a valley
         }
         else
         {
            dHaunchMin = haunch.TopSlabToTopGirder - haunch.tSlab - haunch.GirderOrientationEffect;
         }

         if ( dHaunchMin < -haunch_tolerance )
         {
            (*pTable2)(row2,col++) << color(Red) << comp.SetValue( dHaunchMin ) << color(Black);
         }
         else
         {
            (*pTable2)(row2,col++) << comp.SetValue( dHaunchMin );
         }

         row2++;
      }

      comp.ShowUnitTag(true);

      // table footnotes
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      pgsTypes::SlabOffsetType slabOffsetType = pBridgeDesc->GetSlabOffsetType();

      pPara = new rptParagraph(rptStyleManager::GetFootnoteStyle());
      *pChapter << pPara;

      *pPara << _T("Deck Slope (") << Sub2(_T("m"),_T("d")) << _T(") and Girder Top Slope (") << Sub2(_T("m"),_T("g")) << _T(") are positive when the slope is upwards towards the right") << rptNewLine;
      *pPara << Super(_T("*")) << _T(" required slab offset (equal at both ends) from top of girder to top of deck at centerline bearing for geometric effects at this point. (Slab Thickness + Fillet");
      if (bTopFlangeShapeEffect)
      {
         *pPara << _T(" + Top Flange Shape Effect");
      }
      *pPara << _T(" + Excess Camber + Profile Effect + Girder Orientation Effect)") << rptNewLine;

      if ( slabOffsetType == pgsTypes::sotBridge )
      {
         // one _T("A") dimension for whole bridge
         Float64 A = pBridgeDesc->GetSlabOffset();
         *pPara << Super(_T("**")) << _T(" includes the effects of camber and based on Slab Offset of ") << comp.SetValue(A) << _T(".") << rptNewLine;
         *pPara << Super(_T("***")) << _T(" top of girder to top of deck based on Slab Offset of ") << comp.SetValue(A) << _T(". (Profile Grade Elevation - Top Girder Elevation)") << rptNewLine;
      }
      else
      {
         Float64 Astart, Aend;
         pBridge->GetSlabOffset(segmentKey, &Astart, &Aend);
         *pPara << Super(_T("**")) << _T(" includes the effects of camber and based on Slab Offset at the start of the girder of ") << comp.SetValue(Astart);
         *pPara << _T(" and a Slab Offset at the end of the girder of ") << comp.SetValue(Aend) << _T(".") << rptNewLine;

         *pPara << Super(_T("***")) << _T(" actual depth from top C.L. of girder to top of deck based on Slab Offset at the start of the girder of ") << comp.SetValue(Astart);
         *pPara << _T(" and a Slab Offset at the end of the girder of ") << comp.SetValue(Aend) << _T(". (Profile Grade Elevation - Top Girder Elevation)") << rptNewLine;
      }

      *pPara << Super(_T("&")) << _T(" CL Haunch Depth = Haunch Depth along the centerline of girder")  << rptNewLine;
      *pPara << Super(_T("&&")) << _T(" Least Haunch Depth = Haunch Depth at Edge of Top Flange = CL Haunch Depth - Girder Orientation Effect")  << rptNewLine;

      // this is not footnote text.... need a new paragraph with the regular style
      pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << rptNewLine;

      comp.ShowUnitTag(true);
      *pPara << _T("Required Slab Offset at intersection of centerline bearing and centerline girder: ") << comp.SetValue(haunch_details.RequiredSlabOffset);
      *pPara << _T(" (") << comp.SetValue(RoundOff(haunch_details.RequiredSlabOffset, haunch_tolerance)) << _T(", rounded)") << rptNewLine;
      *pPara << _T("Maximum Change in CL Haunch Depth along girder: ") << comp.SetValue(haunch_details.HaunchDiff) << rptNewLine;
   } // next span

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << rptNewLine;

   *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("ProfileEffect.gif")) << rptNewLine;
   *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("GirderOrientationEffect.png"));
   *pPara << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("GirderOrientationEffectEquation.png"))  << rptNewLine;

   return pChapter;
}

CChapterBuilder* CADimChapterBuilder::Clone() const
{
   return new CADimChapterBuilder;
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
