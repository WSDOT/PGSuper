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

#pragma once

#include <PgsExt\SegmentRelatedStatusItem.h>
#include <PGSuperTypes.h>

// status for haul truck
class pgsHaulTruckStatusItem : public pgsSegmentRelatedStatusItem
{
public:
   pgsHaulTruckStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription,const CSegmentKey& segmentKey);
   bool IsEqual(CEAFStatusItem* pOther);

   CSegmentKey m_SegmentKey;
};

///////////////////////////
class pgsHaulTruckStatusCallback : public iStatusCallback
{
public:
   pgsHaulTruckStatusCallback(IBroker* pBroker,eafTypes::StatusSeverityType statusLevel);
   virtual eafTypes::StatusSeverityType GetSeverity() const override;
   virtual void Execute(CEAFStatusItem* pStatusItem) override;

private:
   IBroker* m_pBroker;
   eafTypes::StatusSeverityType m_Severity;
};
