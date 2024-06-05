///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
#include <IFace\Limits.h>

void CStrandEccentricities::Build(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,
                                IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length, pDisplayUnits->GetSpanLengthUnit(), true );

   // Strand Eccentricity Table
   rptParagraph* p = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << p;

   *p << _T("Strand Eccentricity") << rptNewLine;

   GET_IFACE2( pBroker, ILossParameters, pLossParams);
   PrestressLossCriteria::LossMethodType lossMethod = pLossParams->GetLossMethod();

   GET_IFACE2(pBroker, ISectionProperties, pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker, IStressCheck, pStressCheck);

   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(segmentKey, &vGirderKeys);
   for(const auto& thisGirderKey : vGirderKeys)
   {
      SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);
      SegmentIndexType firstSegmentIdx = (segmentKey.segmentIndex == ALL_SEGMENTS ? 0 : segmentKey.segmentIndex);
      SegmentIndexType lastSegmentIdx  = (segmentKey.segmentIndex == ALL_SEGMENTS ? nSegments-1 : firstSegmentIdx);
      for ( SegmentIndexType segIdx = firstSegmentIdx; segIdx <= lastSegmentIdx; segIdx++ )
      {
         CSegmentKey thisSegmentKey(thisGirderKey,segIdx);

         std::vector<IntervalIndexType> vIntervals = pStressCheck->GetStressCheckIntervals(thisSegmentKey);
         vIntervals.push_back(pIntervals->GetLiveLoadInterval());
         std::sort(vIntervals.begin(), vIntervals.end());
         vIntervals.erase(std::unique(vIntervals.begin(), vIntervals.end()), vIntervals.end());
         for (const auto& intervalIdx : vIntervals)
         {
            CStrandEccTable ecc_table;
            *p << ecc_table.Build(pBroker, thisSegmentKey, intervalIdx, pDisplayUnits) << rptNewLine;
         }

         p = new rptParagraph(rptStyleManager::GetFootnoteStyle());
         *pChapter << p;
         *p << _T("Eccentricities measured from centroid of the section") << rptNewLine;
         *p << _T("Positive ") << Sub2(_T("e"), _T("y")) << _T(" values indicate strands are below the centroid") << rptNewLine;
         *p << rptNewLine;
      } // next segment
   } // next group
}
