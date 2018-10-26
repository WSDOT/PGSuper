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
#include <Reporting\StrandEccentricities.h>
#include <Reporting\StrandEccTable.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CStrandEccentricities
****************************************************************************/


CStrandEccentricities::CStrandEccentricities()
{
}

CStrandEccentricities::CStrandEccentricities(const CStrandEccentricities& rOther)
{
   MakeCopy(rOther);
}

CStrandEccentricities::~CStrandEccentricities()
{
}

CStrandEccentricities& CStrandEccentricities::operator= (const CStrandEccentricities& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }
   return *this;
}

void CStrandEccentricities::Build(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,
                                IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(), true );

   // Strand Eccentricity Table
   rptParagraph* p = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   *pChapter << p;

   GET_IFACE2( pBroker, ILossParameters, pLossParams);
   pgsTypes::LossMethod lossMethod = pLossParams->GetLossMethod();

   GET_IFACE2(pBroker,ISectionProperties,pSectProps);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (segmentKey.groupIndex == ALL_GROUPS ? 0 : segmentKey.groupIndex);
   GroupIndexType lastGroupIdx  = (segmentKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx);
   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType firstGirderIdx = (segmentKey.girderIndex == ALL_GIRDERS ? 0 : segmentKey.girderIndex);
      GirderIndexType lastGirderIdx  = (segmentKey.girderIndex == ALL_GIRDERS ? nGirders-1 : firstGirderIdx);
      
      for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++ )
      {
         SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(grpIdx,gdrIdx));
         SegmentIndexType firstSegmentIdx = (segmentKey.segmentIndex == ALL_SEGMENTS ? 0 : segmentKey.segmentIndex);
         SegmentIndexType lastSegmentIdx  = (segmentKey.segmentIndex == ALL_SEGMENTS ? nSegments-1 : firstSegmentIdx);
         for ( SegmentIndexType segIdx = firstSegmentIdx; segIdx <= lastSegmentIdx; segIdx++ )
         {
            CSegmentKey thisSegmentKey(grpIdx,gdrIdx,segIdx);

            IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(thisSegmentKey);
            IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(thisSegmentKey);
            IntervalIndexType nIntervals = pIntervals->GetIntervalCount(thisSegmentKey);

            if ( lossMethod == pgsTypes::TIME_STEP )
            {
               for (IntervalIndexType intervalIdx = releaseIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
               {
                  CStrandEccTable ecc_table;
                  *p << ecc_table.Build(pBroker,thisSegmentKey,intervalIdx,pDisplayUnits) << rptNewLine;
               }
            }
            else
            {
               std::vector<IntervalIndexType> vIntervals;
               if ( pSectProps->GetSectionPropertiesMode() == pgsTypes::spmTransformed )
               {
                  vIntervals.push_back(releaseIntervalIdx);
                  vIntervals.push_back(compositeDeckIntervalIdx);
               }
               else
               {
                  vIntervals.push_back(releaseIntervalIdx);
               }

               BOOST_FOREACH(IntervalIndexType intervalIdx,vIntervals)
               {
                  CStrandEccTable ecc_table;
                  *p << ecc_table.Build(pBroker,thisSegmentKey,intervalIdx,pDisplayUnits) << rptNewLine;
               }
            }

            p = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
            *pChapter << p;
            *p << _T("Eccentricities measured from neutral axis of non-composite section") << rptNewLine;
            *p << _T("Positive values indicate strands are below the centroid") << rptNewLine;
            *p << rptNewLine;

            //p = new rptParagraph;
            //*pChapter << p;

            //*p << _T("Overall Length = ") << length.SetValue( pBridge->GetSegmentLength(thisSegmentKey) ) << rptNewLine;
            //*p << _T("Span Length = ") << length.SetValue( pBridge->GetSegmentSpanLength(thisSegmentKey) )<<_T(" (CL Bearing to CL Bearing)") << rptNewLine;
            //*p << _T("Left End Distance = ") << length.SetValue( pBridge->GetSegmentStartEndDistance(thisSegmentKey) )<<_T(" (Overhang, CL Bearing to End of Girder, Measured Along Girder)") << rptNewLine;
            //*p << _T("Right End Distance = ") << length.SetValue( pBridge->GetSegmentEndEndDistance(thisSegmentKey) )<<_T(" (Overhang, CL Bearing to End of Girder, Measured Along Girder)") << rptNewLine;
            //*p << rptNewLine;
         }
      }
   }
}

void CStrandEccentricities::MakeCopy(const CStrandEccentricities& rOther)
{
   // Add copy code here...
}

void CStrandEccentricities::MakeAssignment(const CStrandEccentricities& rOther)
{
   MakeCopy( rOther );
}
