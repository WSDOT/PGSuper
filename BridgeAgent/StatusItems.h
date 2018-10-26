///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
class pgsAlignmentDescriptionStatusItem : public CEAFStatusItem
{
public:
   pgsAlignmentDescriptionStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,long dlgPage,LPCTSTR strDescription);
   bool IsEqual(CEAFStatusItem* pOther);

   long m_DlgPage;
};

///////////////////////////
class pgsAlignmentDescriptionStatusCallback : public iStatusCallback
{
public:
   pgsAlignmentDescriptionStatusCallback(IBroker* pBroker,eafTypes::StatusSeverityType severity);
   virtual eafTypes::StatusSeverityType GetSeverity();
   virtual void Execute(CEAFStatusItem* pStatusItem);

private:
   IBroker* m_pBroker;
   eafTypes::StatusSeverityType m_Severity;
};

// status for Concrete Strength
class pgsConcreteStrengthStatusItem : public pgsSpanGirderRelatedStatusItem
{
public:
   enum ConcreteType { Slab, Girder, RailingSystem };
   enum ElementType { ReleaseStrength, FinalStrength, Density, DensityForWeight, AggSize, Modulus };
   pgsConcreteStrengthStatusItem(ConcreteType concType,ElementType elemType,SpanIndexType span,GirderIndexType gdr,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription);
   bool IsEqual(CEAFStatusItem* pOther);

   ConcreteType m_ConcreteType;
   ElementType m_ElementType;
   SpanIndexType m_Span;
   GirderIndexType m_Girder;
};

///////////////////////////
class pgsConcreteStrengthStatusCallback : public iStatusCallback
{
public:
   pgsConcreteStrengthStatusCallback(IBroker* pBroker,eafTypes::StatusSeverityType statusLevel);
   virtual eafTypes::StatusSeverityType GetSeverity();
   virtual void Execute(CEAFStatusItem* pStatusItem);

private:
   IBroker* m_pBroker;
   eafTypes::StatusSeverityType m_Severity;
};

// status for point loads
class pgsPointLoadStatusItem : public pgsSpanGirderRelatedStatusItem
{
public:
   pgsPointLoadStatusItem(IndexType loadIndex,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription,
                          SpanIndexType span,GirderIndexType gdr);
   bool IsEqual(CEAFStatusItem* pOther);

   IndexType m_LoadIndex;
   SpanIndexType   m_Span;
   GirderIndexType m_Gdr;
};

class pgsPointLoadStatusCallback : public iStatusCallback
{
public:
   pgsPointLoadStatusCallback(IBroker* pBroker,eafTypes::StatusSeverityType severity);
   virtual eafTypes::StatusSeverityType GetSeverity();
   virtual void Execute(CEAFStatusItem* pStatusItem);

private:
   IBroker* m_pBroker;
   eafTypes::StatusSeverityType m_Severity;
};

// status for Distributed loads
class pgsDistributedLoadStatusItem : public pgsSpanGirderRelatedStatusItem
{
public:
   pgsDistributedLoadStatusItem(IndexType loadIndex,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription,
                                SpanIndexType span,GirderIndexType gdr);
   bool IsEqual(CEAFStatusItem* pOther);

   IndexType m_LoadIndex;
   SpanIndexType   m_Span;
   GirderIndexType m_Gdr;
};

///////////////////////////
class pgsDistributedLoadStatusCallback : public iStatusCallback
{
public:
   pgsDistributedLoadStatusCallback(IBroker* pBroker,eafTypes::StatusSeverityType severity);
   virtual eafTypes::StatusSeverityType GetSeverity();
   virtual void Execute(CEAFStatusItem* pStatusItem);

private:
   IBroker* m_pBroker;
   eafTypes::StatusSeverityType m_Severity;
};

// status for moment loads
class pgsMomentLoadStatusItem : public pgsSpanGirderRelatedStatusItem
{
public:
   pgsMomentLoadStatusItem(IndexType loadIndex,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription,
                           SpanIndexType span,GirderIndexType gdr);
   bool IsEqual(CEAFStatusItem* pOther);

   IndexType m_LoadIndex;
   SpanIndexType   m_Span;
   GirderIndexType m_Gdr;
};

///////////////////////////
class pgsMomentLoadStatusCallback : public iStatusCallback
{
public:
   pgsMomentLoadStatusCallback(IBroker* pBroker,eafTypes::StatusSeverityType severity);
   virtual eafTypes::StatusSeverityType GetSeverity();
   virtual void Execute(CEAFStatusItem* pStatusItem);

private:
   IBroker* m_pBroker;
   eafTypes::StatusSeverityType m_Severity;
};
