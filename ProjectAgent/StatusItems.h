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

#pragma once

#include <PgsExt\StatusItem.h>
#include <PGSuperTypes.h>


// status for Rebar Strength
class pgsRebarStrengthStatusItem : public pgsSegmentRelatedStatusItem
{
public:
   enum Type { Longitudinal, Transverse, Deck };
   pgsRebarStrengthStatusItem(const CSegmentKey& segmentKey,Type type,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription);
   bool IsEqual(CEAFStatusItem* pOther);

   CSegmentKey m_SegmentKey;
   Type m_Type;
};

class pgsRebarStrengthStatusCallback : public iStatusCallback
{
public:
   pgsRebarStrengthStatusCallback();
   virtual eafTypes::StatusSeverityType GetSeverity() override;
   virtual void Execute(CEAFStatusItem* pStatusItem) override;
};
