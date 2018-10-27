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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\SegmentRelatedStatusItem.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

pgsSegmentRelatedStatusItem::pgsSegmentRelatedStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription,const CSegmentKey& segmentKey):
CEAFStatusItem(statusGroupID,callbackID,strDescription),
m_EntireBridge(false)
{
   AddRelationshipTo(segmentKey);
}

bool pgsSegmentRelatedStatusItem::IsRelatedTo(const CSegmentKey& segmentKey)
{
   if (m_EntireBridge)
   {
      return true;
   }

#pragma Reminder("UPDATE: assuming precast girder bridge")
   SpanIndexType spanIdx = segmentKey.groupIndex;
   GirderIndexType gdrIdx = segmentKey.girderIndex;
   ATLASSERT(segmentKey.segmentIndex == 0);

   // ALL_SPANS
   if( !m_EntireSpans.empty() )
   {
      if(m_EntireSpans.end() != std::find(m_EntireSpans.begin(), m_EntireSpans.end(), spanIdx) )
      {
         return true;
      }
   }

   // ALL_SPANS
   if( !m_EntireGirderLines.empty() )
   {
      if( m_EntireGirderLines.end() != std::find(m_EntireGirderLines.begin(), m_EntireGirderLines.end(), gdrIdx) )
      {
         return true;
      }
   }

   // Individual girders
#pragma Reminder("UPDATE: assuming precast girder bridge")
   std::set<SpanGirderHashType>::iterator it = m_SpanGirders.find( ::HashSpanGirder(segmentKey.groupIndex,segmentKey.girderIndex) );
   return it != m_SpanGirders.end();
}

void pgsSegmentRelatedStatusItem::AddRelationshipTo(const CSegmentKey& segmentKey)
{
#pragma Reminder("UPDATE: assuming precast girder bridge")
   SpanIndexType spanIdx = segmentKey.groupIndex;
   GirderIndexType gdrIdx = segmentKey.girderIndex;

   if(spanIdx == ALL_SPANS && gdrIdx == ALL_GIRDERS)
   {
      m_EntireBridge = true;
   }
   else if(spanIdx == ALL_SPANS)
   {
      m_EntireSpans.push_back(spanIdx);
   }
   else if(gdrIdx == ALL_GIRDERS)
   {
      m_EntireGirderLines.push_back(gdrIdx);
   }
   else
   {
      m_SpanGirders.insert( ::HashSpanGirder(segmentKey.groupIndex,segmentKey.girderIndex) );
   }
}

