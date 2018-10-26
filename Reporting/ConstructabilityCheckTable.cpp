///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <Reporting\ConstructabilityCheckTable.h>

#include <IFace\Artifact.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Constructability.h>

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\HoldDownForceArtifact.h>
#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CConstructabilityCheckTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CConstructabilityCheckTable::CConstructabilityCheckTable()
{
}

CConstructabilityCheckTable::CConstructabilityCheckTable(const CConstructabilityCheckTable& rOther)
{
   MakeCopy(rOther);
}

CConstructabilityCheckTable::~CConstructabilityCheckTable()
{
}

//======================== OPERATORS  =======================================
CConstructabilityCheckTable& CConstructabilityCheckTable::operator= (const CConstructabilityCheckTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CConstructabilityCheckTable::BuildSlabOffsetTable(IBroker* pBroker,const std::vector<CGirderKey>& girderList,IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IArtifact,pIArtifact);

#pragma Reminder("UPDATE: assuming precast girder bridge") // this needs to be updated for spliced girders

   // Create table - delete it later if we don't need it
   bool IsSingleGirder = girderList.size()==1;

   ColumnIndexType nCols = IsSingleGirder ? 4 : 6; // put span/girder in table if multi girder
   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(nCols,_T("Slab Offset (\"A\" Dimension)"));

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim2, pDisplayUnits->GetComponentDimUnit(), true );

   pTable->SetColumnStyle(nCols-1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(nCols-1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;
   if (!IsSingleGirder)
   {
      (*pTable)(0,col++) << _T("Span");
      (*pTable)(0,col++) << _T("Girder");
   }

   (*pTable)(0,col++) << COLHDR(_T("Minimum") << rptNewLine << _T("Provided"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Required"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << _T("Status");
   (*pTable)(0,col++) << _T("Notes");

   // First thing - check if we can generate the girder schedule table at all.
   bool areAnyRows(false);
   std::vector<CGirderKey>::const_iterator girderIter(girderList.begin());
   std::vector<CGirderKey>::const_iterator girderIterEnd(girderList.end());
   RowIndexType row=0;
   for ( ; girderIter != girderIterEnd; girderIter++ )
   {
      const CGirderKey& girderKey(*girderIter);

      const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetGirderArtifact(girderKey);

      const pgsConstructabilityArtifact* pArtifact = pGdrArtifact->GetConstructabilityArtifact();
      
      if (pArtifact->SlabOffsetStatus() != pgsConstructabilityArtifact::NA)
      {
         row++;
         col = 0;

         if (!IsSingleGirder)
         {
            SpanIndexType span = girderKey.groupIndex;
            GirderIndexType girder = girderKey.girderIndex;
            (*pTable)(row, col++) << LABEL_SPAN(span);
            (*pTable)(row, col++) << LABEL_GIRDER(girder);
         }

         (*pTable)(row, col++) << dim.SetValue(pArtifact->GetProvidedSlabOffset());
         (*pTable)(row, col++) << dim.SetValue(pArtifact->GetRequiredSlabOffset());

         switch( pArtifact->SlabOffsetStatus() )
         {
            case pgsConstructabilityArtifact::Pass:
               (*pTable)(row, col++) << RPT_PASS;
               break;

            case pgsConstructabilityArtifact::Fail:
               (*pTable)(row, col++) << RPT_FAIL;
               break;

            case pgsConstructabilityArtifact::Excessive:
               (*pTable)(row, col++) << color(Blue) << _T("Excessive") << color(Black);
               break;

            default:
               ATLASSERT(false);
               break;
         }

         if ( pArtifact->CheckStirrupLength() )
         {
            GET_IFACE2(pBroker,IGirderHaunch,pGdrHaunch);
            HAUNCHDETAILS haunch_details;
            pGdrHaunch->GetHaunchDetails(girderKey,&haunch_details);

            if ( 0 < haunch_details.HaunchDiff )
            {
               (*pTable)(row, col++) << color(Red) << _T("The haunch depth in the middle of the span exceeds the depth at the ends by ") << dim2.SetValue(haunch_details.HaunchDiff) << _T(". Check stirrup lengths to ensure they engage the deck in all locations.") << color(Black) << rptNewLine;
            }
            else
            {
               (*pTable)(row, col++) << color(Red) << _T("The haunch depth in the ends of the span exceeds the depth at the middle by ") << dim2.SetValue(-haunch_details.HaunchDiff) << _T(". Check stirrup lengths to ensure they engage the deck in all locations.") << color(Black) << rptNewLine;
            }
         }
         else
         {
            (*pTable)(row, col++) << _T("");
         }
      }
   } // next girder

   // Only return a table if it has content
   if (0 < row)
   {
      return pTable;
   }
   else
   {
      delete pTable;
      return NULL;
   }
}

void CConstructabilityCheckTable::BuildCamberCheck(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,ICamber,pCamber);

   GET_IFACE2( pBroker, ILibrary, pLib );
   GET_IFACE2( pBroker, ISpecification, pSpec );
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );
   Float64 min_days =  ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Min(), unitMeasure::Day);
   Float64 max_days =  ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Max(), unitMeasure::Day);

   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Camber");

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(2,_T(""));

   pTable->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   *pBody << pTable << rptNewLine;

   (*pTable)(0,0) << _T("");
   (*pTable)(0,1) << COLHDR(_T("Camber"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker, IPointOfInterest, pPointOfInterest );

   GET_IFACE2(pBroker, IBridge, pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   SpanIndexType startSpanIdx, endSpanIdx;
   pBridge->GetGirderGroupSpans(girderKey.groupIndex,&startSpanIdx,&endSpanIdx);
   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      CSpanKey spanKey(spanIdx,girderKey.girderIndex);
      std::vector<pgsPointOfInterest> vPoi = pPointOfInterest->GetPointsOfInterest(spanKey,POI_SPAN | POI_5L);
      ATLASSERT(vPoi.size()==1);
      pgsPointOfInterest poiMidSpan(vPoi.front());

      Float64 C = 0;
      if ( deckType != pgsTypes::sdtNone )
      {
         C = pCamber->GetScreedCamber( poiMidSpan ) ;
         (*pTable)(row,  0) << _T("Screed Camber, C");
         (*pTable)(row++,1) << dim.SetValue(C);
      }
   
      // get # of days for creep
      Float64 Dmax_UpperBound, Dmax_Average, Dmax_LowerBound;
      Float64 Dmin_UpperBound, Dmin_Average, Dmin_LowerBound;
      Float64 Cfactor = pCamber->GetLowerBoundCamberVariabilityFactor();
      Dmin_UpperBound = pCamber->GetDCamberForGirderSchedule( poiMidSpan, CREEP_MINTIME);
      Dmax_UpperBound = pCamber->GetDCamberForGirderSchedule( poiMidSpan, CREEP_MAXTIME);
      
      Dmin_LowerBound = Cfactor*Dmin_UpperBound;
      Dmin_Average    = (1+Cfactor)/2*Dmin_UpperBound;
      
      Dmax_LowerBound = Cfactor*Dmax_UpperBound;
      Dmax_Average    = (1+Cfactor)/2*Dmax_UpperBound;
   
   
      if ( IsEqual(min_days,max_days) )
      {
         // Min/Max timing cambers will be the same, only report them once
         ATLASSERT(IsEqual(Dmin_UpperBound,Dmax_UpperBound));
         ATLASSERT(IsEqual(Dmin_Average,   Dmax_Average));
         ATLASSERT(IsEqual(Dmin_LowerBound,Dmax_LowerBound));
         if ( IsZero(1-Cfactor) )
         {
            // Upper,average, and lower bound cambers will all be the same... report them once
            ATLASSERT(IsEqual(Dmin_UpperBound,Dmin_LowerBound));
            ATLASSERT(IsEqual(Dmin_UpperBound,Dmin_Average));
   
            (*pTable)(row,0) << _T("Camber at ") << min_days << _T(" days, D") << Sub(min_days);
            if ( Dmin_UpperBound < 0 )
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmin_UpperBound) << color(Black);
            else
               (*pTable)(row++,1) << dim.SetValue(Dmin_UpperBound);
         }
         else
         {
            (*pTable)(row,0) << _T("Lower Bound Camber at ") << min_days << _T(" days, ")<<Cfactor*100<<_T("% of D") <<Sub(min_days);
            if ( Dmin_LowerBound < 0 )
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmin_LowerBound) << color(Black);
            else
               (*pTable)(row++,1) << dim.SetValue(Dmin_LowerBound);
   
            (*pTable)(row,0) << _T("Average Camber at ") << min_days << _T(" days, ")<<(1+Cfactor)/2*100<<_T("% of D") <<Sub(min_days);
            if ( Dmin_Average < 0 )
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmin_Average) << color(Black);
            else
               (*pTable)(row++,1) << dim.SetValue(Dmin_Average);
   
            (*pTable)(row,0) << _T("Upper Bound Camber at ") << min_days << _T(" days, D") << Sub(min_days);
            if ( Dmin_UpperBound < 0 )
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmin_UpperBound) << color(Black);
            else
               (*pTable)(row++,1) << dim.SetValue(Dmin_UpperBound);
         }
      }
      else
      {
         if ( IsZero(1-Cfactor) )
         {
            (*pTable)(row,0) << _T("Camber at ") << min_days << _T(" days, D") << Sub(min_days);
            if ( Dmin_UpperBound < 0 )
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmin_UpperBound) << color(Black);
            else
               (*pTable)(row++,1) << dim.SetValue(Dmin_UpperBound);
   
            (*pTable)(row,0) << _T("Camber at ") << max_days << _T(" days, D") << Sub(max_days);
            if ( Dmax_UpperBound < 0 )
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmax_UpperBound) << color(Black);
            else
               (*pTable)(row++,1) << dim.SetValue(Dmax_UpperBound);
         }
         else
         {
            (*pTable)(row,0) << _T("Lower bound camber at ")<< min_days<<_T(" days, ")<<Cfactor*100<<_T("% of D") <<Sub(min_days);
            if ( Dmin_LowerBound < 0 )
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmin_LowerBound) << color(Black);
            else
               (*pTable)(row++,1) << dim.SetValue(Dmin_LowerBound);
   
            (*pTable)(row,0) << _T("Average camber at ")<< min_days<<_T(" days, ")<<(1+Cfactor)/2*100<<_T("% of D") <<Sub(min_days);
            if ( Dmin_Average < 0 )
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmin_Average) << color(Black);
            else
               (*pTable)(row++,1) << dim.SetValue(Dmin_Average);
   
            (*pTable)(row,0) << _T("Upper bound camber at ")<< min_days<<_T(" days, D") << Sub(min_days);
            if ( Dmin_UpperBound < 0 )
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmin_UpperBound) << color(Black);
            else
               (*pTable)(row++,1) << dim.SetValue(Dmin_UpperBound);
   
            (*pTable)(row,0) << _T("Lower bound camber at ")<< max_days<<_T(" days, ")<<Cfactor*100<<_T("% of D") <<Sub(max_days);
            if ( Dmax_LowerBound < 0 )
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmax_LowerBound) << color(Black);
            else
               (*pTable)(row++,1) << dim.SetValue(Dmax_LowerBound);
   
            (*pTable)(row,0) << _T("Average camber at ")<< max_days<<_T(" days, ")<<(1+Cfactor)/2*100<<_T("% of D") <<Sub(max_days);
            if ( Dmax_Average < 0 )
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmax_Average) << color(Black);
            else
               (*pTable)(row++,1) << dim.SetValue(Dmax_Average);
   
            (*pTable)(row,0) << _T("Upper bound camber at ")<< max_days<<_T(" days, D") << Sub(max_days);
            if ( Dmax_UpperBound < 0 )
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmax_UpperBound) << color(Black);
            else
               (*pTable)(row++,1) << dim.SetValue(Dmax_UpperBound);
         }
      }
   
      if ( pSpecEntry->CheckGirderSag() && deckType != pgsTypes::sdtNone )
      {
         std::_tstring camberType;
         Float64 D = 0;
   
         switch(pSpecEntry->GetSagCamberType())
         {
         case pgsTypes::LowerBoundCamber:
            D = Dmin_LowerBound;
            camberType = _T("lower bound");
            break;
         case pgsTypes::AverageCamber:
            D = Dmin_Average;
            camberType = _T("average");
            break;
         case pgsTypes::UpperBoundCamber:
            D = Dmin_UpperBound;
            camberType = _T("upper bound");
            break;
         }
   
         if ( D < C )
         {
            rptParagraph* p = new rptParagraph;
            *pChapter << p;
   
            *p << color(Red) << _T("WARNING: Screed Camber, C, is greater than the ") << camberType.c_str() << _T(" camber at time of deck casting, D. The girder may end up with a sag.") << color(Black) << rptNewLine;
         }
         else if ( IsEqual(C,D,::ConvertToSysUnits(0.25,unitMeasure::Inch)) )
         {
            rptParagraph* p = new rptParagraph;
            *pChapter << p;
   
            *p << color(Red) << _T("WARNING: Screed Camber, C, is nearly equal to the ") << camberType.c_str() << _T(" camber at time of deck casting, D. The girder may end up with a sag.") << color(Black) << rptNewLine;
         }
   
         if ( Dmin_LowerBound < C && pSpecEntry->GetSagCamberType() != pgsTypes::LowerBoundCamber )
         {
            rptParagraph* p = new rptParagraph;
            *pChapter << p;
   
            *p << _T("Screed Camber (C) is greater than the lower bound camber at time of deck casting (") << Cfactor*100 << _T("% of D") << Sub(min_days) << _T("). The girder may end up with a sag if the deck is placed at day ") << min_days << _T(" and the actual camber is a lower bound value.") << rptNewLine;
         }
      }
   }
}

void CConstructabilityCheckTable::BuildGlobalGirderStabilityCheck(rptChapter* pChapter,IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bIsApplicable = false;
   SegmentIndexType nSegments = pBridge->GetSegmentCount(pGirderArtifact->GetGirderKey());
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsSegmentStabilityArtifact* pArtifact = pSegmentArtifact->GetSegmentStabilityArtifact();
      
      if ( pArtifact->IsGlobalGirderStabilityApplicable() )
      {
         bIsApplicable = true;
         break;
      }
   }

   if ( !bIsApplicable )
      return;

   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Global Stability of Girder");

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   *pBody << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("GlobalGirderStability.gif"));

   rptRcScalar slope;
   slope.SetFormat(pDisplayUnits->GetScalarFormat().Format);
   slope.SetWidth(pDisplayUnits->GetScalarFormat().Width);
   slope.SetPrecision(pDisplayUnits->GetScalarFormat().Precision);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );

   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsSegmentStabilityArtifact* pArtifact = pSegmentArtifact->GetSegmentStabilityArtifact();

      CString strTitle;
      if ( 1 < nSegments )
         strTitle.Format(_T("Segment %d"), LABEL_SEGMENT(segIdx));
      else
         strTitle = _T("");

      rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(5,strTitle);
      std::_tstring strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

      (*pTable)(0,0) << COLHDR(Sub2(_T("W"),_T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*pTable)(0,1) << COLHDR(Sub2(_T("Y"),_T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*pTable)(0,2) << _T("Incline from Vertical (") << Sub2(symbol(theta),_T("max")) << _T(")") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
      (*pTable)(0,3) << _T("Max Incline") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
      (*pTable)(0,4) << _T("Status");

      Float64 Wb, Yb, Orientation;
      pArtifact->GetGlobalGirderStabilityParameters(&Wb,&Yb,&Orientation);
      Float64 maxIncline = pArtifact->GetMaxGirderIncline();

      (*pTable)(1,0) << dim.SetValue(Wb);
      (*pTable)(1,1) << dim.SetValue(Yb);
      (*pTable)(1,2) << slope.SetValue(Orientation);
      (*pTable)(1,3) << slope.SetValue(maxIncline);

      if ( pArtifact->Passed() )
         (*pTable)(1,4) << RPT_PASS;
      else
         (*pTable)(1,4) << RPT_FAIL << rptNewLine << _T("Reaction falls outside of middle third of bottom width of girder");
      
      *pBody << pTable;
   } // next segment
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CConstructabilityCheckTable::MakeCopy(const CConstructabilityCheckTable& rOther)
{
   // Add copy code here...
}

void CConstructabilityCheckTable::MakeAssignment(const CConstructabilityCheckTable& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool CConstructabilityCheckTable::AssertValid() const
{
   return true;
}

void CConstructabilityCheckTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CConstructabilityCheckTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CConstructabilityCheckTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CConstructabilityCheckTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CConstructabilityCheckTable");

   TESTME_EPILOG("CConstructabilityCheckTable");
}
#endif // _UNITTEST
