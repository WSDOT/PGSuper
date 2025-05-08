///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#pragma once

#include <PgsExt\PgsExtExp.h>
#include <PsgLib\Keys.h>
#include <EAF\StatusItem.h>

// Special WBFL::EAF::StatusItem that is directly related to a given span(s)/girders(s)
// This class allows reporting and messaging routines to filter WBFL::EAF::StatusItem's
//
class PGSEXTCLASS pgsSegmentRelatedStatusItem : public WBFL::EAF::StatusItem
{
public:
   pgsSegmentRelatedStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription,const CSegmentKey& segmentKey);

   bool IsRelatedTo(const CSegmentKey& segmentKey);

   void AddRelationshipTo(const CSegmentKey& segmentKey);

private:
   // status items can depend using ALL_SPANS and ALL_GIRDERS
   bool m_EntireBridge;
   std::vector<SpanIndexType> m_EntireSpans;
   std::vector<GirderIndexType> m_EntireGirderLines;
   std::set<SpanGirderHashType> m_SpanGirders;
};
