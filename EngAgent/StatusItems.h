///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
   virtual eafTypes::StatusSeverityType GetSeverity();
   virtual void Execute(CEAFStatusItem* pStatusItem);

private:
   IBroker* m_pBroker;
};


// status for Lifting support location
class pgsLiftingSupportLocationStatusItem : public CEAFStatusItem
{
public:
   pgsLiftingSupportLocationStatusItem(SpanIndexType span,GirderIndexType gdr,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription);
   bool IsEqual(CEAFStatusItem* pOther);

   SpanIndexType m_Span;
   GirderIndexType m_Girder;
};

///////////////////////////
class pgsLiftingSupportLocationStatusCallback : public iStatusCallback
{
public:
   pgsLiftingSupportLocationStatusCallback(IBroker* pBroker,eafTypes::StatusSeverityType severity);
   virtual eafTypes::StatusSeverityType GetSeverity();
   virtual void Execute(CEAFStatusItem* pStatusItem);

private:
   IBroker* m_pBroker;
   eafTypes::StatusSeverityType m_Severity;
};

// status for truck stiffness
class pgsTruckStiffnessStatusItem : public CEAFStatusItem
{
public:
   pgsTruckStiffnessStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription);
   bool IsEqual(CEAFStatusItem* pOther);
};

///////////////////////////
class pgsTruckStiffnessStatusCallback : public iStatusCallback
{
public:
   pgsTruckStiffnessStatusCallback(IBroker* pBroker);
   virtual eafTypes::StatusSeverityType GetSeverity();
   virtual void Execute(CEAFStatusItem* pStatusItem);

private:
   IBroker* m_pBroker;
};

// status for bunk point
class pgsBunkPointLocationStatusItem : public CEAFStatusItem
{
public:
   pgsBunkPointLocationStatusItem(SpanIndexType span,GirderIndexType gdr,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription);
   bool IsEqual(CEAFStatusItem* pOther);

   SpanIndexType m_Span;
   GirderIndexType m_Girder;
};

///////////////////////////
class pgsBunkPointLocationStatusCallback : public iStatusCallback
{
public:
   pgsBunkPointLocationStatusCallback(IBroker* pBroker);
   virtual eafTypes::StatusSeverityType GetSeverity();
   virtual void Execute(CEAFStatusItem* pStatusItem);

private:
   IBroker* m_pBroker;
};
