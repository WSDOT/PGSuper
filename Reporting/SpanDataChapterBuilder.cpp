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
#include <Reporting\SpanDataChapterBuilder.h>
#include <Reporting\StrandEccTable.h>

#include <IFace\Bridge.h>
#include <IFace\Intervals.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CSpanDataChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CSpanDataChapterBuilder::CSpanDataChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CSpanDataChapterBuilder::GetName() const
{
#pragma Reminder("UPDATE: this chapter doesn't make sense")
   // The name of this chapter doesn't make sense... neither does the data
   // though it is important (eccentricities and component dimensions)
   // Find a better way to report this
   return TEXT("Span Data");
}

rptChapter* CSpanDataChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   pGdrRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGdrRptSpec->GetGirderKey());

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(), true );

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);


   // Strand Eccentricity Table
   rptParagraph* p = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   *pChapter << p;

   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   GET_IFACE2(pBroker,IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx  = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx);
   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType firstGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex);
      GirderIndexType lastGirderIdx  = (girderKey.girderIndex == ALL_GIRDERS ? nGirders-1 : firstGirderIdx);
      
      for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++ )
      {
         SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(grpIdx,gdrIdx));
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey thisSegmentKey(grpIdx,gdrIdx,segIdx);

            IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(thisSegmentKey);

            if ( spMode == pgsTypes::spmGross )
            {
               CStrandEccTable ecc_table;
               *p << ecc_table.Build(pBroker,thisSegmentKey,releaseIntervalIdx,pDisplayUnits) << rptNewLine;
            }
            else
            {
               for (IntervalIndexType intervalIdx = releaseIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
               {
                  *p << _T("Interval ") << LABEL_INTERVAL(intervalIdx) << _T(" ") << pIntervals->GetDescription(intervalIdx) << rptNewLine;
                  CStrandEccTable ecc_table;
                  *p << ecc_table.Build(pBroker,thisSegmentKey,intervalIdx,pDisplayUnits) << rptNewLine;
               }
            }

            p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
            *pChapter << p;
            *p << _T("Eccentricities measured from neutral axis of non-composite section") << rptNewLine;
            *p << _T("Positive values indicate strands are below the centroid") << rptNewLine;
            *p << rptNewLine;

            p = new rptParagraph;
            *pChapter << p;

            *p << _T("Overall Length = ") << length.SetValue( pBridge->GetSegmentLength(thisSegmentKey) ) << rptNewLine;
            *p << _T("Span Length = ") << length.SetValue( pBridge->GetSegmentSpanLength(thisSegmentKey) )<<_T(" (CL Bearing to CL Bearing)") << rptNewLine;
            *p << _T("Left End Distance = ") << length.SetValue( pBridge->GetSegmentStartEndDistance(thisSegmentKey) )<<_T(" (Overhang, CL Bearing to End of Girder, Measured Along Girder)") << rptNewLine;
            *p << _T("Right End Distance = ") << length.SetValue( pBridge->GetSegmentEndEndDistance(thisSegmentKey) )<<_T(" (Overhang, CL Bearing to End of Girder, Measured Along Girder)") << rptNewLine;
            *p << rptNewLine;
         }
      }
   }

   return pChapter;
}

CChapterBuilder* CSpanDataChapterBuilder::Clone() const
{
   return new CSpanDataChapterBuilder;
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
