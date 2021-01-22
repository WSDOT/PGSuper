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

#pragma once

#include <PgsExt\StatusItem.h>
#include <PGSuperTypes.h>

class pgsLiveLoadStatusItem : public CEAFStatusItem
{
public:
   pgsLiveLoadStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription);
   bool IsEqual(CEAFStatusItem* pOther);
};

class pgsLiveLoadStatusCallback : public iStatusCallback
{
public:
   pgsLiveLoadStatusCallback(IBroker* pBroker);
   virtual eafTypes::StatusSeverityType GetSeverity() const override;
   virtual void Execute(CEAFStatusItem* pStatusItem) override;

private:
   IBroker* m_pBroker;
};


// status for Lifting support location
class pgsLiftingSupportLocationStatusItem : public CEAFStatusItem
{
public:
   pgsLiftingSupportLocationStatusItem(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription);
   bool IsEqual(CEAFStatusItem* pOther);

   CSegmentKey m_SegmentKey;
   pgsTypes::MemberEndType m_End;
};

///////////////////////////
class pgsLiftingSupportLocationStatusCallback : public iStatusCallback
{
public:
   pgsLiftingSupportLocationStatusCallback(IBroker* pBroker,eafTypes::StatusSeverityType severity);
   virtual eafTypes::StatusSeverityType GetSeverity() const override;
   virtual void Execute(CEAFStatusItem* pStatusItem) override;

private:
   IBroker* m_pBroker;
   eafTypes::StatusSeverityType m_Severity;
};

// status for truck stiffness
class pgsHaulTruckStatusItem : public CEAFStatusItem
{
public:
   pgsHaulTruckStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription);
   bool IsEqual(CEAFStatusItem* pOther);
};

///////////////////////////
class pgsHaulTruckStatusCallback : public iStatusCallback
{
public:
   pgsHaulTruckStatusCallback(IBroker* pBroker);
   virtual eafTypes::StatusSeverityType GetSeverity() const override;
   virtual void Execute(CEAFStatusItem* pStatusItem) override;

private:
   IBroker* m_pBroker;
};

// status for bunk point
class pgsBunkPointLocationStatusItem : public CEAFStatusItem
{
public:
   pgsBunkPointLocationStatusItem(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription);
   bool IsEqual(CEAFStatusItem* pOther);

   CSegmentKey m_SegmentKey;
   pgsTypes::MemberEndType m_End;
};

///////////////////////////
class pgsBunkPointLocationStatusCallback : public iStatusCallback
{
public:
   pgsBunkPointLocationStatusCallback(IBroker* pBroker);
   virtual eafTypes::StatusSeverityType GetSeverity() const override;
   virtual void Execute(CEAFStatusItem* pStatusItem) override;

private:
   IBroker* m_pBroker;
};
