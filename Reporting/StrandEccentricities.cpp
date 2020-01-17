///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
   rptParagraph* p = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << p;

   p->SetName(_T("Strand Eccentricity"));
   *p << p->GetName() << rptNewLine;

   //GET_IFACE2( pBroker, ILossParameters, pLossParams);
   //pgsTypes::LossMethod lossMethod = pLossParams->GetLossMethod();

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

            //if ( lossMethod == pgsTypes::TIME_STEP )
            //{
            //   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(thisSegmentKey);
            //   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
            //   for (IntervalIndexType intervalIdx = releaseIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
            //   {
            //      CStrandEccTable ecc_table;
            //      *p << ecc_table.Build(pBroker,thisSegmentKey,intervalIdx,pDisplayUnits) << rptNewLine;
            //   }
            //}
            //else
            //{
               IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(thisSegmentKey);
               CStrandEccTable ecc_table;
               *p << ecc_table.Build(pBroker,thisSegmentKey,releaseIntervalIdx,pDisplayUnits) << rptNewLine;
            //}

            p = new rptParagraph(rptStyleManager::GetFootnoteStyle());
            *pChapter << p;
            GET_IFACE2(pBroker,ISectionProperties,pSectProp);
            if ( pSectProp->GetSectionPropertiesMode() == pgsTypes::spmGross )
            {
               *p << _T("Eccentricities are based on the gross non-composite girder section") << rptNewLine;
            }
            else
            {
               *p << _T("Eccentricities are based on the net non-composite girder section") << rptNewLine;
            }
            *p << _T("Eccentricities measured from neutral axis of non-composite section based on material properties at time of prestress release") << rptNewLine;

            if (pBridge->HasAsymmetricGirders() || pBridge->HasAsymmetricPrestressing())
            {
               *p << _T("Positive ") << Sub2(_T("e"), _T("x")) << _T(" values indicate strands are to the left of the centroid") << rptNewLine;
            }
            *p << _T("Positive ") << Sub2(_T("e"), _T("y")) << _T(" values indicate strands are below the centroid") << rptNewLine;
            *p << rptNewLine;
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
