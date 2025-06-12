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

#include <PgsExt\StatusItem.h>
#include <PGSuperTypes.h>

class pgsLiveLoadStatusItem : public WBFL::EAF::StatusItem
{
public:
   pgsLiveLoadStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription);
   bool IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const override;
};

class pgsLiveLoadStatusCallback : public WBFL::EAF::StatusCallback
{
public:
   pgsLiveLoadStatusCallback();
   WBFL::EAF::StatusSeverityType GetSeverity() const override;
   void Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem) override;
};


// status for Lifting support location
class pgsLiftingSupportLocationStatusItem : public WBFL::EAF::StatusItem
{
public:
   pgsLiftingSupportLocationStatusItem(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription);
   bool IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const override;

   CSegmentKey m_SegmentKey;
   pgsTypes::MemberEndType m_End;
};

///////////////////////////
class pgsLiftingSupportLocationStatusCallback : public WBFL::EAF::StatusCallback
{
public:
   pgsLiftingSupportLocationStatusCallback(WBFL::EAF::StatusSeverityType severity);
   WBFL::EAF::StatusSeverityType GetSeverity() const override;
   void Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem) override;

private:
   WBFL::EAF::StatusSeverityType m_Severity;
};

// status for truck stiffness
class pgsHaulTruckStatusItem : public WBFL::EAF::StatusItem
{
public:
   pgsHaulTruckStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription);
   bool IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const override;
};

///////////////////////////
class pgsHaulTruckStatusCallback : public WBFL::EAF::StatusCallback
{
public:
   pgsHaulTruckStatusCallback();
   WBFL::EAF::StatusSeverityType GetSeverity() const override;
   void Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem) override;
};

// status for bunk point
class pgsBunkPointLocationStatusItem : public WBFL::EAF::StatusItem
{
public:
   pgsBunkPointLocationStatusItem(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription);
   bool IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const override;

   CSegmentKey m_SegmentKey;
   pgsTypes::MemberEndType m_End;
};

///////////////////////////
class pgsBunkPointLocationStatusCallback : public WBFL::EAF::StatusCallback
{
public:
   pgsBunkPointLocationStatusCallback();
   WBFL::EAF::StatusSeverityType GetSeverity() const override;
   void Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem) override;
};
