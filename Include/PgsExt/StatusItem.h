///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

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
   pgsStatusItem(Uint32 agentID,Uint32 callbackID,const char* strDescription);

   // Called by the framework to assign a unique identifier
   // to the status item. Don't call this method
   void SetID(Uint32 id);

   Uint32 GetID() const;
   Uint32 GetAgentID() const;

   // framework will remove status item after edit if true
   bool RemoveAfterEdit();
   void RemoveAfterEdit(bool bRemoveAfterEdit);

   const std::string& GetDescription() const;
   Uint32 GetCallbackID() const;

   virtual bool IsEqual(pgsStatusItem* pOther) = 0;

private:
   Uint32 m_ID;
   Uint32 m_AgentID;
   Uint32 m_CallbackID;
   bool m_bRemoveAfterEdit;
   std::string m_Description;
};

// status for point loads
class PGSEXTCLASS pgsPointLoadStatusItem : public pgsStatusItem
{
public:
   pgsPointLoadStatusItem(Uint32 loadIndex,Uint32 agentID,Uint32 callbackID,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);

   Uint32 m_LoadIndex;
};

// status for moment loads
class PGSEXTCLASS pgsMomentLoadStatusItem : public pgsStatusItem
{
public:
   pgsMomentLoadStatusItem(Uint32 loadIndex,Uint32 agentID,Uint32 callbackID,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);

   Uint32 m_LoadIndex;
};

// status for Distributed loads
class PGSEXTCLASS pgsDistributedLoadStatusItem : public pgsStatusItem
{
public:
   pgsDistributedLoadStatusItem(Uint32 loadIndex,Uint32 agentID,Uint32 callbackID,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);

   Uint32 m_LoadIndex;
};

// status for Concrete Strength
class PGSEXTCLASS pgsConcreteStrengthStatusItem : public pgsStatusItem
{
public:
   enum ConcreteType { Slab, Girder };
   enum ElementType { ReleaseStrength, FinalStrength, Density, DensityForWeight, AggSize };
   pgsConcreteStrengthStatusItem(ConcreteType concType,ElementType elemType,SpanIndexType span,GirderIndexType gdr,long agentID,long callbackID,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);

   ConcreteType m_ConcreteType;
   ElementType m_ElementType;
   SpanIndexType m_Span;
   GirderIndexType m_Girder;
};

// status for Volume to Surface Ratio
class PGSEXTCLASS pgsVSRatioStatusItem : public pgsStatusItem
{
public:
   pgsVSRatioStatusItem(SpanIndexType span,GirderIndexType gdr,Uint32 agentID,Uint32 callbackID,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);

   SpanIndexType m_Span;
   GirderIndexType m_Girder;
};

// status for Lifting support location
class PGSEXTCLASS pgsLiftingSupportLocationStatusItem : public pgsStatusItem
{
public:
   pgsLiftingSupportLocationStatusItem(SpanIndexType span,GirderIndexType gdr,Uint32 agentID,Uint32 callbackID,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);

   SpanIndexType m_Span;
   GirderIndexType m_Girder;
};

// status for truck stiffness
class PGSEXTCLASS pgsTruckStiffnessStatusItem : public pgsStatusItem
{
public:
   pgsTruckStiffnessStatusItem(Uint32 agentID,Uint32 callbackID,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);
};

// status for general bridge description input
class PGSEXTCLASS pgsBridgeDescriptionStatusItem : public pgsStatusItem
{
public:
   pgsBridgeDescriptionStatusItem(Uint32 agentID,Uint32 callbackID,long dlgPage,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);

   long m_DlgPage;
};

// status for general alignment description input
class PGSEXTCLASS pgsAlignmentDescriptionStatusItem : public pgsStatusItem
{
public:
   pgsAlignmentDescriptionStatusItem(Uint32 agentID,Uint32 callbackID,long dlgPage,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);

   long m_DlgPage;
};

// status for refined analysis
class PGSEXTCLASS pgsRefinedAnalysisStatusItem : public pgsStatusItem
{
public:
   pgsRefinedAnalysisStatusItem(Uint32 agentID,Uint32 callbackID,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);
};

// status for install error
class PGSEXTCLASS pgsInstallationErrorStatusItem : public pgsStatusItem
{
public:
   pgsInstallationErrorStatusItem(Uint32 agentID,Uint32 callbackID,const char* strComponent,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);
   std::string m_Component;
};

// status for unknown error
class PGSEXTCLASS pgsUnknownErrorStatusItem : public pgsStatusItem
{
public:
   pgsUnknownErrorStatusItem(Uint32 agentID,Uint32 callbackID,const char* file,long line,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);
   std::string m_File;
   long m_Line;
};

// status informational message
class PGSEXTCLASS pgsInformationalStatusItem : public pgsStatusItem
{
public:
   pgsInformationalStatusItem(Uint32 agentID,Uint32 callbackID,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);

};

// status for girder input
class PGSEXTCLASS pgsGirderDescriptionStatusItem : public pgsStatusItem
{
public:
   pgsGirderDescriptionStatusItem(SpanIndexType span,GirderIndexType gdr,Uint16 page,Uint32 agentID,Uint32 callbackID,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);

   SpanIndexType m_Span;
   GirderIndexType m_Girder;
   Uint16 m_Page; // page of girder input wizard
};

// status for live load
class PGSEXTCLASS pgsLiveLoadStatusItem : public pgsStatusItem
{
public:
   pgsLiveLoadStatusItem(Uint32 agentID,Uint32 callbackID,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);
};

// status for structural analysis type
class PGSEXTCLASS pgsStructuralAnalysisTypeStatusItem : public pgsStatusItem
{
public:
   pgsStructuralAnalysisTypeStatusItem(Uint32 agentID,Uint32 callbackID,const char* strDescription);
   bool IsEqual(pgsStatusItem* pOther);
};

#endif // INCLUDED_STATUSITEM_H_
