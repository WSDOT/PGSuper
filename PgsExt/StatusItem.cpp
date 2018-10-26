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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\StatusItem.h>

#include <IFace\Project.h>
#include <IFace\EditByUI.h>
#include <IFace\StatusCenter.h>

#include "StatusMessageDialog.h"
#include "RefinedAnalysisOptionsDlg.h"

#include <PgsExt\BridgeDescription.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

pgsStatusItem::pgsStatusItem(AgentIDType agentID,StatusCallbackIDType callbackID,const char* strDescription) :
m_Description(strDescription), m_bRemoveAfterEdit(false)
{
   m_AgentID = agentID;
   m_CallbackID = callbackID;
}

void pgsStatusItem::SetID(StatusItemIDType id)
{
   m_ID = id;
}

StatusItemIDType pgsStatusItem::GetID() const
{
   return m_ID;
}

AgentIDType pgsStatusItem::GetAgentID() const
{
   return m_AgentID;
}

const std::string& pgsStatusItem::GetDescription() const
{
   return m_Description;
}

StatusCallbackIDType pgsStatusItem::GetCallbackID() const
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


////////////////

pgsRefinedAnalysisStatusItem::pgsRefinedAnalysisStatusItem(AgentIDType agentID,StatusCallbackIDType callbackID,const char* strDescription) :
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

//////////////////////////////////////////////////////////
pgsRefinedAnalysisStatusCallback::pgsRefinedAnalysisStatusCallback(IBroker* pBroker):
m_pBroker(pBroker)
{
}

pgsTypes::StatusSeverityType pgsRefinedAnalysisStatusCallback::GetSeverity()
{
   return pgsTypes::statusError;
}

void pgsRefinedAnalysisStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   pgsRefinedAnalysisStatusItem* pItem = dynamic_cast<pgsRefinedAnalysisStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   CRefinedAnalysisOptionsDlg dlg;
   dlg.m_strDescription = pStatusItem->GetDescription().c_str();
   dlg.m_strDescription.Replace("\n","\r\n"); // this makes the text wrap correctly in the dialog

   if ( dlg.DoModal() == IDOK )
   {
      GET_IFACE(ILiveLoads,pLiveLoads);
      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

      pgsTypes::DistributionFactorMethod method;
      LldfRangeOfApplicabilityAction roaAction;
      switch(dlg.m_Choice)
      {
      case CRefinedAnalysisOptionsDlg::lldfDirectInput:
         method = pgsTypes::DirectlyInput;
         roaAction = roaIgnore;
         break;

      case CRefinedAnalysisOptionsDlg::lldfIgnore:
         method = pgsTypes::Calculated;
         roaAction = roaIgnore;
         break;

      case CRefinedAnalysisOptionsDlg::lldfIgnoreLever:
         method = pgsTypes::Calculated;
         roaAction =roaIgnoreUseLeverRule;
         break;

      case CRefinedAnalysisOptionsDlg::lldfForceLever:
         method = pgsTypes::LeverRule;
         roaAction = roaIgnore;
         break;

      case CRefinedAnalysisOptionsDlg::lldfDefault:
         method = pBridgeDesc->GetDistributionFactorMethod();
         roaAction = pLiveLoads->GetLldfRangeOfApplicabilityAction();
         break;

      default:
         ATLASSERT(false); // is there a new choice???
      }

      GET_IFACE(IEditByUI,pEdit);
      pEdit->EditLiveLoadDistributionFactors(method,roaAction);
   }
}

////////////////

pgsInstallationErrorStatusItem::pgsInstallationErrorStatusItem(AgentIDType agentID,StatusCallbackIDType callbackID,const char* strComponent,const char* strDescription) :
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

//////////////////////////////////////////////////////////
pgsInstallationErrorStatusCallback::pgsInstallationErrorStatusCallback()
{
}

pgsTypes::StatusSeverityType pgsInstallationErrorStatusCallback::GetSeverity()
{
   return pgsTypes::statusError;
}

void pgsInstallationErrorStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   pgsInstallationErrorStatusItem* pItem = dynamic_cast<pgsInstallationErrorStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   CString msg;
   msg.Format("PGSuper was not successfully installed or has become damanged.\n\n%s could not be created.\n\nPlease re-install the software.",pItem->m_Component.c_str());
   AfxMessageBox(msg,MB_OK | MB_ICONEXCLAMATION);
}


////////////////

pgsUnknownErrorStatusItem::pgsUnknownErrorStatusItem(AgentIDType agentID,StatusCallbackIDType callbackID,const char* strFile,long line,const char* strDescription) :
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

//////////////////////////////////////////////////////////
pgsUnknownErrorStatusCallback::pgsUnknownErrorStatusCallback()
{
}

pgsTypes::StatusSeverityType pgsUnknownErrorStatusCallback::GetSeverity()
{
   return pgsTypes::statusError;
}

void pgsUnknownErrorStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   pgsUnknownErrorStatusItem* pItem = dynamic_cast<pgsUnknownErrorStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   CString msg;
   msg.Format("An unspecified error occured at %s, Line %d",pItem->m_File.c_str(),pItem->m_Line);
   AfxMessageBox(msg,MB_OK | MB_ICONEXCLAMATION);
}

////////////////

pgsInformationalStatusItem::pgsInformationalStatusItem(AgentIDType agentID,StatusCallbackIDType callbackID,const char* strDescription) :
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

//////////////////////////////////////////////////////////
pgsInformationalStatusCallback::pgsInformationalStatusCallback(pgsTypes::StatusSeverityType severity,UINT helpID):
m_Severity(severity), m_HelpID(helpID)
{
}

pgsTypes::StatusSeverityType pgsInformationalStatusCallback::GetSeverity()
{
   return m_Severity;
}

void pgsInformationalStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   std::string msg = pStatusItem->GetDescription();

   if (m_HelpID!=0)
   {
      msg += std::string("\r\n\r\nClick on Help button for more details.");
   }

   bool is_severe = m_Severity==pgsTypes::statusError;
   if (!is_severe)
   {
      msg += std::string("\r\n\r\nClick OK to remove this message.");
   }

   pgsInformationalStatusItem* pItem = dynamic_cast<pgsInformationalStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   CStatusMessageDialog dlg;
   dlg.m_Message = msg.c_str();
   dlg.m_IsSevere = is_severe;
   dlg.m_HelpID = m_HelpID;

   int st = dlg.DoModal();
   if (!is_severe && st==IDOK) // allow non-severe messages to be removed by user
   {
      pStatusItem->RemoveAfterEdit(true);
   }
}


////////////////

pgsGirderDescriptionStatusItem::pgsGirderDescriptionStatusItem(SpanIndexType span,GirderIndexType gdr,Uint16 page,AgentIDType agentID,StatusCallbackIDType callbackID,const char* strDescription) :
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

//////////////////////////////////////////////////////////
pgsGirderDescriptionStatusCallback::pgsGirderDescriptionStatusCallback(IBroker* pBroker,pgsTypes::StatusSeverityType severity):
m_pBroker(pBroker), m_Severity(severity)
{
}

pgsTypes::StatusSeverityType pgsGirderDescriptionStatusCallback::GetSeverity()
{
   return m_Severity;
}

void pgsGirderDescriptionStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   pgsGirderDescriptionStatusItem* pItem = dynamic_cast<pgsGirderDescriptionStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   CString strMessage;
   strMessage.Format("%s\n\r%s",pItem->GetDescription().c_str(),"Would you like to edit the girder?");
   int result = AfxMessageBox(strMessage,MB_YESNO);

   if ( result == IDYES )
   {
      GET_IFACE(IEditByUI,pEdit);

      if (pEdit->EditGirderDescription(pItem->m_Span,pItem->m_Girder,pItem->m_Page))
      {
         // assume that edit took care of status
         StatusItemIDType id = pItem->GetID();
         GET_IFACE(IStatusCenter,pStatusCenter);
         pStatusCenter->RemoveByID(id);
      }
   }
}

////////////////

pgsStructuralAnalysisTypeStatusItem::pgsStructuralAnalysisTypeStatusItem(AgentIDType agentID,StatusCallbackIDType callbackID,const char* strDescription) :
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

////////////////

pgsStructuralAnalysisTypeStatusCallback::pgsStructuralAnalysisTypeStatusCallback()
{
}

pgsTypes::StatusSeverityType pgsStructuralAnalysisTypeStatusCallback::GetSeverity()
{
   return pgsTypes::statusWarning;
}

void pgsStructuralAnalysisTypeStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   AfxMessageBox(pStatusItem->GetDescription().c_str(),MB_OK);
}

pgsBridgeDescriptionStatusItem::pgsBridgeDescriptionStatusItem(AgentIDType agentID,StatusCallbackIDType callbackID,long dlgPage,const char* strDescription) :
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

//////////////////////////////////////////////////////////
pgsBridgeDescriptionStatusCallback::pgsBridgeDescriptionStatusCallback(IBroker* pBroker,pgsTypes::StatusSeverityType severity):
m_pBroker(pBroker), m_Severity(severity)
{
}

pgsTypes::StatusSeverityType pgsBridgeDescriptionStatusCallback::GetSeverity()
{
   return m_Severity;
}

void pgsBridgeDescriptionStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   pgsBridgeDescriptionStatusItem* pItem = dynamic_cast<pgsBridgeDescriptionStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   GET_IFACE(IEditByUI,pEdit);
   pEdit->EditBridgeDescription(pItem->m_DlgPage);
}
