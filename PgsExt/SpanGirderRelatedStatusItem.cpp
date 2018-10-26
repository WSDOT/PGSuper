///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\SpanGirderRelatedStatusItem.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

pgsSpanGirderRelatedStatusItem::pgsSpanGirderRelatedStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription,
                                                               SpanIndexType span,GirderIndexType gdr):
CEAFStatusItem(statusGroupID,callbackID,strDescription),
m_EntireBridge(false)
{
   AddRelationshipTo(span,gdr);
}

bool pgsSpanGirderRelatedStatusItem::IsRelatedTo(SpanIndexType span,GirderIndexType gdr)
{
   if (m_EntireBridge)
   {
      return true;
   }

   // ALL_SPANS
   if( !m_EntireSpans.empty() )
   {
      if(m_EntireSpans.end() != std::find(m_EntireSpans.begin(), m_EntireSpans.end(), span) )
      {
         return true;
      }
   }

   // ALL_SPANS
   if( !m_EntireGirderLines.empty() )
   {
      if( m_EntireGirderLines.end() != std::find(m_EntireGirderLines.begin(), m_EntireGirderLines.end(), gdr) )
      {
         return true;
      }
   }

   // Individual girders
   SpanGirderHashType hash = HashSpanGirder(span,gdr);
   std::set<SpanGirderHashType>::iterator it = m_SpanGirders.find(hash);
   return it != m_SpanGirders.end();
}

void pgsSpanGirderRelatedStatusItem::AddRelationshipTo(SpanIndexType span,GirderIndexType gdr)
{
   if(span==ALL_SPANS && gdr==ALL_GIRDERS)
   {
      m_EntireBridge = true;
   }
   else if(span==ALL_SPANS)
   {
      m_EntireSpans.push_back(span);
   }
   else if(gdr==ALL_GIRDERS)
   {
      m_EntireGirderLines.push_back(gdr);
   }
   else
   {
      SpanGirderHashType hash = HashSpanGirder(span,gdr);
      m_SpanGirders.insert(hash);
   }
}

