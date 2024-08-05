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

#include "stdafx.h"
#include <Reporting\GirderSeedDataComparisonParagraph.h>

#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CGirderSeedDataComparisonParagraph
****************************************************************************/

CGirderSeedDataComparisonParagraph::CGirderSeedDataComparisonParagraph()
{
}

CGirderSeedDataComparisonParagraph::~CGirderSeedDataComparisonParagraph()
{
} 

rptParagraph* CGirderSeedDataComparisonParagraph::Build(IBroker* pBroker, const CGirderKey& girderKey) const
{
   rptParagraph* pParagraph = new rptParagraph;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);

   GirderIndexType nGirders = pGroup->GetGirderCount();
   GirderIndexType firstGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex);
   GirderIndexType lastGirderIdx  = (girderKey.girderIndex == ALL_GIRDERS ? nGirders-1: firstGirderIdx);

   bool was_diff = false;
   for (GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++)
   {
      const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
      const GirderLibraryEntry* pGirderLib = pGirder->GetGirderLibraryEntry();

      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         if ( 1 < nSegments )
         {
            *pParagraph << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
         }

         if (pGirderLib->DoWarnForTransReinfEquality())
         {
            const CShearData2& currentShearData = pGirder->GetSegment(segIdx)->ShearData;

            // compare shear data from library
            CShearData2 shearData;
            shearData.CopyGirderEntryData(pGirderLib);
            if (currentShearData != shearData)
            {
               *pParagraph << color(Red) << _T("Trans. Reinforcement data for Girder ") << LABEL_GIRDER(gdrIdx) << _T(" does not match Girder Library entry ") << pGirderLib->GetName() << color(Black) << rptNewLine;
               was_diff = true;
            }
         }

         if (pGirderLib->DoWarnForLongReinfEquality())
         {
            // compare long data from library
            const CLongitudinalRebarData& currentLRD = pGirder->GetSegment(segIdx)->LongitudinalRebarData;

            CLongitudinalRebarData longData;
            longData.CopyGirderEntryData(pGirderLib);
            if (currentLRD != longData)
            {
               *pParagraph << color(Red) << _T("Long. Reinforcement data for Girder ") << LABEL_GIRDER(gdrIdx) << _T(" does not match Girder Library entry ") << pGirderLib->GetName() << color(Black) << rptNewLine;
               was_diff = true;
            }
         }

         if ( 1 < nSegments )
         {
            *pParagraph << rptNewLine;
         }
      } // next segment
   } // next girder

   if (was_diff)
   {
      return pParagraph;
   }
   else
   {
      delete pParagraph;
      return nullptr;
   }
}
