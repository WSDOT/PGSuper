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


// status for general alignment description input
class pgsAlignmentDescriptionStatusItem : public pgsStatusItem
{
public:
   pgsAlignmentDescriptionStatusItem(AgentIDType agentID,StatusCallbackIDType callbackID,long dlgPage,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);

   long m_DlgPage;
};

///////////////////////////
class pgsAlignmentDescriptionStatusCallback : public iStatusCallback
{
public:
   pgsAlignmentDescriptionStatusCallback(IBroker* pBroker,pgsTypes::StatusSeverityType severity);
   virtual pgsTypes::StatusSeverityType GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   IBroker* m_pBroker;
   pgsTypes::StatusSeverityType m_Severity;
};

// status for Concrete Strength
class pgsConcreteStrengthStatusItem : public pgsStatusItem
{
public:
   enum ConcreteType { Slab, Girder, RailingSystem };
   enum ElementType { ReleaseStrength, FinalStrength, Density, DensityForWeight, AggSize };
   pgsConcreteStrengthStatusItem(ConcreteType concType,ElementType elemType,SpanIndexType span,GirderIndexType gdr,AgentIDType agentID,StatusCallbackIDType callbackID,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);

   ConcreteType m_ConcreteType;
   ElementType m_ElementType;
   SpanIndexType m_Span;
   GirderIndexType m_Girder;
};

///////////////////////////
class pgsConcreteStrengthStatusCallback : public iStatusCallback
{
public:
   pgsConcreteStrengthStatusCallback(IBroker* pBroker,pgsTypes::StatusSeverityType statusLevel);
   virtual pgsTypes::StatusSeverityType GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   IBroker* m_pBroker;
   pgsTypes::StatusSeverityType m_Severity;
};

// status for point loads
class pgsPointLoadStatusItem : public pgsStatusItem
{
public:
   pgsPointLoadStatusItem(Uint32 loadIndex,AgentIDType agentID,StatusCallbackIDType callbackID,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);

   Uint32 m_LoadIndex;
};

class pgsPointLoadStatusCallback : public iStatusCallback
{
public:
   pgsPointLoadStatusCallback(IBroker* pBroker,pgsTypes::StatusSeverityType severity);
   virtual pgsTypes::StatusSeverityType GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   IBroker* m_pBroker;
   pgsTypes::StatusSeverityType m_Severity;
};

// status for Distributed loads
class pgsDistributedLoadStatusItem : public pgsStatusItem
{
public:
   pgsDistributedLoadStatusItem(Uint32 loadIndex,AgentIDType agentID,StatusCallbackIDType callbackID,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);

   Uint32 m_LoadIndex;
};

///////////////////////////
class pgsDistributedLoadStatusCallback : public iStatusCallback
{
public:
   pgsDistributedLoadStatusCallback(IBroker* pBroker,pgsTypes::StatusSeverityType severity);
   virtual pgsTypes::StatusSeverityType GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   IBroker* m_pBroker;
   pgsTypes::StatusSeverityType m_Severity;
};

// status for moment loads
class pgsMomentLoadStatusItem : public pgsStatusItem
{
public:
   pgsMomentLoadStatusItem(Uint32 loadIndex,AgentIDType agentID,StatusCallbackIDType callbackID,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);

   Uint32 m_LoadIndex;
};

///////////////////////////
class pgsMomentLoadStatusCallback : public iStatusCallback
{
public:
   pgsMomentLoadStatusCallback(IBroker* pBroker,pgsTypes::StatusSeverityType severity);
   virtual pgsTypes::StatusSeverityType GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   IBroker* m_pBroker;
   pgsTypes::StatusSeverityType m_Severity;
};
