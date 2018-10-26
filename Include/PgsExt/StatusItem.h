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

#ifndef INCLUDED_STATUSITEM_H_
#define INCLUDED_STATUSITEM_H_

// SYSTEM INCLUDES
//
#include <PgsExt\PgsExtExp.h>


// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

class pgsStatusItem;

interface iStatusCallback
{
   virtual pgsTypes::StatusSeverityType GetSeverity() = 0;
   virtual void Execute(pgsStatusItem* pItem) = 0;
};

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   pgsStatusItem
   
DESCRIPTION
   Base class for Status Items. Derived classes must implement the IsEqual
   method.
   
COPYRIGHT
   Copyright © 1997-2004
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/

class PGSEXTCLASS pgsStatusItem
{
public:
   pgsStatusItem(AgentIDType agentID,StatusCallbackIDType callbackID,const char* strDescription);

   // Called by the framework to assign a unique identifier
   // to the status item. Don't call this method
   void SetID(StatusItemIDType id);

   StatusItemIDType GetID() const;
   AgentIDType GetAgentID() const;

   // framework will remove status item after edit if true
   bool RemoveAfterEdit();
   void RemoveAfterEdit(bool bRemoveAfterEdit);

   const std::string& GetDescription() const;
   StatusCallbackIDType GetCallbackID() const;

   virtual bool IsEqual(pgsStatusItem* pOther) = 0;

private:
   StatusItemIDType m_ID;
   AgentIDType m_AgentID;
   StatusCallbackIDType m_CallbackID;
   bool m_bRemoveAfterEdit;
   std::string m_Description;
};

// status for refined analysis
class PGSEXTCLASS pgsRefinedAnalysisStatusItem : public pgsStatusItem
{
public:
   pgsRefinedAnalysisStatusItem(AgentIDType agentID,StatusCallbackIDType callbackID,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);
};

///////////////////////////
class PGSEXTCLASS pgsRefinedAnalysisStatusCallback : public iStatusCallback
{
public:
   pgsRefinedAnalysisStatusCallback(IBroker* pBroker);
   virtual pgsTypes::StatusSeverityType GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   IBroker* m_pBroker;
};

// status for install error
class PGSEXTCLASS pgsInstallationErrorStatusItem : public pgsStatusItem
{
public:
   pgsInstallationErrorStatusItem(AgentIDType agentID,StatusCallbackIDType callbackID,const char* strComponent,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);
   std::string m_Component;
};

///////////////////////////
class PGSEXTCLASS pgsInstallationErrorStatusCallback : public iStatusCallback
{
public:
   pgsInstallationErrorStatusCallback();
   virtual pgsTypes::StatusSeverityType GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);
};

// status for unknown error
class PGSEXTCLASS pgsUnknownErrorStatusItem : public pgsStatusItem
{
public:
   pgsUnknownErrorStatusItem(AgentIDType agentID,StatusCallbackIDType callbackID,const char* file,long line,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);
   std::string m_File;
   long m_Line;
};

///////////////////////////
class PGSEXTCLASS pgsUnknownErrorStatusCallback : public iStatusCallback
{
public:
   pgsUnknownErrorStatusCallback();
   virtual pgsTypes::StatusSeverityType GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);
};

// status informational message
class PGSEXTCLASS pgsInformationalStatusItem : public pgsStatusItem
{
public:
   pgsInformationalStatusItem(AgentIDType agentID,StatusCallbackIDType callbackID,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);

};

class PGSEXTCLASS pgsInformationalStatusCallback : public iStatusCallback
{
public:
   pgsInformationalStatusCallback(pgsTypes::StatusSeverityType severity,UINT helpID=0);
   virtual pgsTypes::StatusSeverityType GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   pgsTypes::StatusSeverityType m_Severity;
   UINT m_HelpID;
};

// status for girder input
class PGSEXTCLASS pgsGirderDescriptionStatusItem : public pgsStatusItem
{
public:
   pgsGirderDescriptionStatusItem(SpanIndexType span,GirderIndexType gdr,Uint16 page,AgentIDType agentID,StatusCallbackIDType callbackID,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);

   SpanIndexType m_Span;
   GirderIndexType m_Girder;
   Uint16 m_Page; // page of girder input wizard
};


///////////////////////////
class PGSEXTCLASS pgsGirderDescriptionStatusCallback : public iStatusCallback
{
public:
   pgsGirderDescriptionStatusCallback(IBroker* pBroker,pgsTypes::StatusSeverityType severity);
   virtual pgsTypes::StatusSeverityType GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   IBroker* m_pBroker;
   pgsTypes::StatusSeverityType m_Severity;
};

// status for structural analysis type
class PGSEXTCLASS pgsStructuralAnalysisTypeStatusItem : public pgsStatusItem
{
public:
   pgsStructuralAnalysisTypeStatusItem(AgentIDType agentID,StatusCallbackIDType callbackID,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);
};

class PGSEXTCLASS pgsStructuralAnalysisTypeStatusCallback : public iStatusCallback
{
public:
   pgsStructuralAnalysisTypeStatusCallback();
   virtual pgsTypes::StatusSeverityType GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);
};

// status for general bridge description input
class PGSEXTCLASS pgsBridgeDescriptionStatusItem : public pgsStatusItem
{
public:
   pgsBridgeDescriptionStatusItem(AgentIDType agentID,StatusCallbackIDType callbackID,long dlgPage,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);

   long m_DlgPage;
};

///////////////////////////
class PGSEXTCLASS pgsBridgeDescriptionStatusCallback : public iStatusCallback
{
public:
   pgsBridgeDescriptionStatusCallback(IBroker* pBroker,pgsTypes::StatusSeverityType severity);
   virtual pgsTypes::StatusSeverityType GetSeverity();
   virtual void Execute(pgsStatusItem* pStatusItem);

private:
   IBroker* m_pBroker;
   pgsTypes::StatusSeverityType m_Severity;
};

#endif // INCLUDED_STATUSITEM_H_
