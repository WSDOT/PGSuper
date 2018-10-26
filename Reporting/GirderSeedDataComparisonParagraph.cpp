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

#include "stdafx.h"
#include <Reporting\GirderSeedDataComparisonParagraph.h>
#include <PgsExt\ReportStyleHolder.h>
#include <IFace\Project.h>
#include <PgsExt\BridgeDescription.h>
#include <PgsExt\GirderData.h>

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

rptParagraph* CGirderSeedDataComparisonParagraph::Build(IBroker* pBroker, SpanIndexType span, GirderIndexType gdr) const
{
   rptParagraph* pParagraph = new rptParagraph;

   GET_IFACE2(pBroker,ILibrary,pLibrary);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);

   GirderIndexType start_gdr, end_gdr;
   if (gdr == INVALID_INDEX)
   {
      start_gdr = 0;
      end_gdr = pSpan->GetGirderCount()-1;
   }
   else
   {
      start_gdr = gdr;
      end_gdr   = gdr;
   }

   bool was_diff = false;
   for (GirderIndexType igdr=start_gdr; igdr<=end_gdr; igdr++)
   {
      const CGirderTypes* pGdrTypes = pSpan->GetGirderTypes();
      const GirderLibraryEntry* pGirderLib = pGdrTypes->GetGirderLibraryEntry(igdr);
      const CGirderData& rGdrData = pGdrTypes->GetGirderData(igdr);

      // compare shear data from library
      CShearData shearData;
      shearData.CopyGirderEntryData(*pGirderLib);
      if (rGdrData.ShearData != shearData)
      {
         *pParagraph<<color(Red)<<_T("Trans. Reinforcement data for Span ")<< LABEL_SPAN(span)<<_T(", Girder ")<< LABEL_GIRDER(igdr) << _T(" does not match Girder Library entry ")<<pGirderLib->GetName()<<color(Black)<<rptNewLine;
         was_diff = true;
      }

      // compare long data from library
      CLongitudinalRebarData longData;
      longData.CopyGirderEntryData(*pGirderLib);
      if (rGdrData.LongitudinalRebarData != longData)
      {
         *pParagraph<<color(Red)<<_T("Long. Reinforcement data for Span ")<< LABEL_SPAN(span)<<_T(", Girder ")<< LABEL_GIRDER(igdr) << _T(" does not match Girder Library entry ")<<pGirderLib->GetName()<<color(Black)<<rptNewLine;
         was_diff = true;
      }
   }

   if (was_diff)
   {
      return pParagraph;
   }
   else
   {
      delete pParagraph;
      return NULL;
   }
}
