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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\StatusItem.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

pgsStatusItem::pgsStatusItem(Uint32 agentID,Uint32 callbackID,const char* strDescription) :
m_Description(strDescription), m_bRemoveAfterEdit(false)
{
   m_AgentID = agentID;
   m_CallbackID = callbackID;
}

void pgsStatusItem::SetID(Uint32 id)
{
   m_ID = id;
}

Uint32 pgsStatusItem::GetID() const
{
   return m_ID;
}

Uint32 pgsStatusItem::GetAgentID() const
{
   return m_AgentID;
}

const std::string& pgsStatusItem::GetDescription() const
{
   return m_Description;
}

Uint32 pgsStatusItem::GetCallbackID() const
{
   return m_CallbackID;
}

bool pgsStatusItem::RemoveAfterEdit()
{
   return m_bRemoveAfterEdit;
}

void pgsStatusItem::RemoveAfterEdit(bool bRemoveAfterEdit)
{
   m_bRemoveAfterEdit = bRemoveAfterEdit;
}


pgsPointLoadStatusItem::pgsPointLoadStatusItem(Uint32 value,Uint32 agentID,Uint32 callbackID,const char* strDescription) :
pgsStatusItem(agentID,callbackID,strDescription), m_LoadIndex(value)
{
}

bool pgsPointLoadStatusItem::IsEqual(pgsStatusItem* pOther)
{
   pgsPointLoadStatusItem* other = dynamic_cast<pgsPointLoadStatusItem*>(pOther);
   if ( !other )
      return false;

   return (other->m_LoadIndex == m_LoadIndex);
}


pgsMomentLoadStatusItem::pgsMomentLoadStatusItem(Uint32 value,Uint32 agentID,Uint32 callbackID,const char* strDescription) :
pgsStatusItem(agentID,callbackID,strDescription), m_LoadIndex(value)
{
}

bool pgsMomentLoadStatusItem::IsEqual(pgsStatusItem* pOther)
{
   pgsMomentLoadStatusItem* other = dynamic_cast<pgsMomentLoadStatusItem*>(pOther);
   if ( !other )
      return false;

   return (other->m_LoadIndex == m_LoadIndex);
}

pgsDistributedLoadStatusItem::pgsDistributedLoadStatusItem(Uint32 value,Uint32 agentID,Uint32 callbackID,const char* strDescription) :
pgsStatusItem(agentID,callbackID,strDescription), m_LoadIndex(value)
{
}

bool pgsDistributedLoadStatusItem::IsEqual(pgsStatusItem* pOther)
{
   pgsDistributedLoadStatusItem* other = dynamic_cast<pgsDistributedLoadStatusItem*>(pOther);
   if ( !other )
      return false;

   return (other->m_LoadIndex == m_LoadIndex);
}

////////////////

pgsConcreteStrengthStatusItem::pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::ConcreteType concType,pgsConcreteStrengthStatusItem::ElementType elemType,SpanIndexType span,GirderIndexType gdr,long agentID,long callbackID,const char* strDescription) :
pgsStatusItem(agentID,callbackID,strDescription), m_ConcreteType(concType),m_ElementType(elemType),m_Span(span),m_Girder(gdr)
{
}

bool pgsConcreteStrengthStatusItem::IsEqual(pgsStatusItem* pOther)
{
   pgsConcreteStrengthStatusItem* other = dynamic_cast<pgsConcreteStrengthStatusItem*>(pOther);
   if ( !other )
      return false;

   return (other->m_ConcreteType == m_ConcreteType && other->m_ElementType == m_ElementType && other->m_Span == m_Span && other->m_Girder == m_Girder);
}

////////////////

pgsVSRatioStatusItem::pgsVSRatioStatusItem(SpanIndexType span,GirderIndexType gdr,Uint32 agentID,Uint32 callbackID,const char* strDescription) :
pgsStatusItem(agentID,callbackID,strDescription), m_Span(span),m_Girder(gdr)
{
}

bool pgsVSRatioStatusItem::IsEqual(pgsStatusItem* pOther)
{
   pgsVSRatioStatusItem* other = dynamic_cast<pgsVSRatioStatusItem*>(pOther);
   if ( !other )
      return false;

   return (other->m_Span == m_Span && other->m_Girder == m_Girder);
}

////////////////

pgsLiftingSupportLocationStatusItem::pgsLiftingSupportLocationStatusItem(SpanIndexType span,GirderIndexType gdr,Uint32 agentID,Uint32 callbackID,const char* strDescription) :
pgsStatusItem(agentID,callbackID,strDescription), m_Span(span),m_Girder(gdr)
{
}

bool pgsLiftingSupportLocationStatusItem::IsEqual(pgsStatusItem* pOther)
{
   pgsLiftingSupportLocationStatusItem* other = dynamic_cast<pgsLiftingSupportLocationStatusItem*>(pOther);
   if ( !other )
      return false;

   return (other->m_Span == m_Span && other->m_Girder == m_Girder);
}

////////////////

pgsTruckStiffnessStatusItem::pgsTruckStiffnessStatusItem(Uint32 agentID,Uint32 callbackID,const char* strDescription) :
pgsStatusItem(agentID,callbackID,strDescription)
{
}

bool pgsTruckStiffnessStatusItem::IsEqual(pgsStatusItem* pOther)
{
   pgsTruckStiffnessStatusItem* other = dynamic_cast<pgsTruckStiffnessStatusItem*>(pOther);
   if ( !other )
      return false;

   return true;
}

////////////////

pgsBridgeDescriptionStatusItem::pgsBridgeDescriptionStatusItem(Uint32 agentID,Uint32 callbackID,long dlgPage,const char* strDescription) :
pgsStatusItem(agentID,callbackID,strDescription), m_DlgPage(dlgPage)
{
}

bool pgsBridgeDescriptionStatusItem::IsEqual(pgsStatusItem* pOther)
{
   pgsBridgeDescriptionStatusItem* other = dynamic_cast<pgsBridgeDescriptionStatusItem*>(pOther);
   if ( !other )
      return false;

   if ( this->GetDescription() != other->GetDescription() )
      return false;

   if ( m_DlgPage != other->m_DlgPage )
      return false;

   return true;
}

////////////////

pgsAlignmentDescriptionStatusItem::pgsAlignmentDescriptionStatusItem(Uint32 agentID,Uint32 callbackID,long dlgPage,const char* strDescription) :
pgsStatusItem(agentID,callbackID,strDescription), m_DlgPage(dlgPage)
{
}

bool pgsAlignmentDescriptionStatusItem::IsEqual(pgsStatusItem* pOther)
{
   pgsAlignmentDescriptionStatusItem* other = dynamic_cast<pgsAlignmentDescriptionStatusItem*>(pOther);
   if ( !other )
      return false;

   if ( this->GetDescription() != other->GetDescription() )
      return false;

   if ( m_DlgPage != other->m_DlgPage )
      return false;

   return true;
}

////////////////

pgsRefinedAnalysisStatusItem::pgsRefinedAnalysisStatusItem(Uint32 agentID,Uint32 callbackID,const char* strDescription) :
pgsStatusItem(agentID,callbackID,strDescription)
{
}

bool pgsRefinedAnalysisStatusItem::IsEqual(pgsStatusItem* pOther)
{
   pgsRefinedAnalysisStatusItem* other = dynamic_cast<pgsRefinedAnalysisStatusItem*>(pOther);
   if ( !other )
      return false;

   return true;
}

////////////////

pgsInstallationErrorStatusItem::pgsInstallationErrorStatusItem(Uint32 agentID,Uint32 callbackID,const char* strComponent,const char* strDescription) :
pgsStatusItem(agentID,callbackID,strDescription), m_Component(strComponent)
{
}

bool pgsInstallationErrorStatusItem::IsEqual(pgsStatusItem* pOther)
{
   pgsInstallationErrorStatusItem* other = dynamic_cast<pgsInstallationErrorStatusItem*>(pOther);
   if ( !other )
      return false;

   if ( m_Component != other->m_Component)
      return false;

   return true;
}

////////////////

pgsUnknownErrorStatusItem::pgsUnknownErrorStatusItem(Uint32 agentID,Uint32 callbackID,const char* strFile,long line,const char* strDescription) :
pgsStatusItem(agentID,callbackID,strDescription), m_File(strFile), m_Line(line)
{
}

bool pgsUnknownErrorStatusItem::IsEqual(pgsStatusItem* pOther)
{
   pgsUnknownErrorStatusItem* other = dynamic_cast<pgsUnknownErrorStatusItem*>(pOther);
   if ( !other )
      return false;

   if ( m_File != other->m_File )
      return false;

   if ( m_Line != other->m_Line )
      return false;

   return true;
}
////////////////

pgsInformationalStatusItem::pgsInformationalStatusItem(Uint32 agentID,Uint32 callbackID,const char* strDescription) :
pgsStatusItem(agentID,callbackID,strDescription)
{
}

bool pgsInformationalStatusItem::IsEqual(pgsStatusItem* pOther)
{
   pgsInformationalStatusItem* other = dynamic_cast<pgsInformationalStatusItem*>(pOther);
   if ( !other )
      return false;

   if ( GetDescription() != other->GetDescription())
      return false;

   return true;
}


////////////////

pgsGirderDescriptionStatusItem::pgsGirderDescriptionStatusItem(SpanIndexType span,GirderIndexType gdr,Uint16 page,Uint32 agentID,Uint32 callbackID,const char* strDescription) :
pgsStatusItem(agentID,callbackID,strDescription), m_Span(span), m_Girder(gdr), m_Page(page)
{
}

bool pgsGirderDescriptionStatusItem::IsEqual(pgsStatusItem* pOther)
{
   pgsGirderDescriptionStatusItem* other = dynamic_cast<pgsGirderDescriptionStatusItem*>(pOther);
   if ( !other )
      return false;

   if ( this->GetDescription() != other->GetDescription() )
      return false;

   if ( m_Span != other->m_Span )
      return false;

   if ( m_Girder != other->m_Girder )
      return false;

   if ( m_Page != other->m_Page )
      return false;

   return true;
}

////////////////

pgsLiveLoadStatusItem::pgsLiveLoadStatusItem(Uint32 agentID,Uint32 callbackID,const char* strDescription) :
pgsStatusItem(agentID,callbackID,strDescription)
{
}

bool pgsLiveLoadStatusItem::IsEqual(pgsStatusItem* pOther)
{
   pgsLiveLoadStatusItem* other = dynamic_cast<pgsLiveLoadStatusItem*>(pOther);
   if ( !other )
      return false;

   return true;
}

////////////////

pgsStructuralAnalysisTypeStatusItem::pgsStructuralAnalysisTypeStatusItem(Uint32 agentID,Uint32 callbackID,const char* strDescription) :
pgsStatusItem(agentID,callbackID,strDescription)
{
}

bool pgsStructuralAnalysisTypeStatusItem::IsEqual(pgsStatusItem* pOther)
{
   pgsStructuralAnalysisTypeStatusItem* other = dynamic_cast<pgsStructuralAnalysisTypeStatusItem*>(pOther);
   if ( !other )
      return false;

   return true;
}
