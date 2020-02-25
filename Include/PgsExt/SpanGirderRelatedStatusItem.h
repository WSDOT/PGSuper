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

#ifndef INCLUDED_SPANGIRDERRELATEDSTATUSITEM_H_
#define INCLUDED_SPANGIRDERRELATEDSTATUSITEM_H_

// SYSTEM INCLUDES
//
#include <PgsExt\PgsExtExp.h>
#include <PgsExt\Keys.h>
#include <EAF\EAFStatusItem.h>

// Special CEAFStatusItem that is directly related to a given span(s)/girders(s)
// This class allows reporting and messaging routines to filter CEAFStatusItem's
//
class PGSEXTCLASS pgsSpanGirderRelatedStatusItem : public CEAFStatusItem
{
public:
   pgsSpanGirderRelatedStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription,
                                  const CSpanKey& spanKey);

   bool IsRelatedTo(SpanIndexType span,GirderIndexType gdr);

   void AddRelationshipTo(SpanIndexType span,GirderIndexType gdr);

private:
   // status items can depend using ALL_SPANS and ALL_GIRDERS
   bool m_EntireBridge;
   std::vector<SpanIndexType> m_EntireSpans;
   std::vector<GirderIndexType> m_EntireGirderLines;
   std::set<CSpanKey> m_SpanGirders;
};


#endif // INCLUDED_SPANGIRDERRELATEDSTATUSITEM_H_
