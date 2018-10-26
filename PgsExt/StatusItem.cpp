///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

pgsRefinedAnalysisStatusItem::pgsRefinedAnalysisStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
CEAFStatusItem(statusGroupID,callbackID,strDescription)
{
}

bool pgsRefinedAnalysisStatusItem::IsEqual(CEAFStatusItem* pOther)
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

eafTypes::StatusSeverityType pgsRefinedAnalysisStatusCallback::GetSeverity()
{
   return eafTypes::statusError;
}

void pgsRefinedAnalysisStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   pgsRefinedAnalysisStatusItem* pItem = dynamic_cast<pgsRefinedAnalysisStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   CRefinedAnalysisOptionsDlg dlg;
   dlg.m_strDescription = pStatusItem->GetDescription().c_str();
   dlg.m_strDescription.Replace(_T("\n"),_T("\r\n")); // this makes the text wrap correctly in the dialog

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

pgsInstallationErrorStatusItem::pgsInstallationErrorStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strComponent,LPCTSTR strDescription) :
CEAFStatusItem(statusGroupID,callbackID,strDescription), m_Component(strComponent)
{
}

bool pgsInstallationErrorStatusItem::IsEqual(CEAFStatusItem* pOther)
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

eafTypes::StatusSeverityType pgsInstallationErrorStatusCallback::GetSeverity()
{
   return eafTypes::statusError;
}

void pgsInstallationErrorStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   pgsInstallationErrorStatusItem* pItem = dynamic_cast<pgsInstallationErrorStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   CString msg;
   msg.Format(_T("PGSuper was not successfully installed or has become damaged.\n\n%s could not be created.\n\nPlease re-install the software."),pItem->m_Component.c_str());
   AfxMessageBox(msg,MB_OK | MB_ICONEXCLAMATION);
}


////////////////

pgsUnknownErrorStatusItem::pgsUnknownErrorStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strFile,long line,LPCTSTR strDescription) :
CEAFStatusItem(statusGroupID,callbackID,strDescription), m_File(strFile), m_Line(line)
{
}

bool pgsUnknownErrorStatusItem::IsEqual(CEAFStatusItem* pOther)
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

eafTypes::StatusSeverityType pgsUnknownErrorStatusCallback::GetSeverity()
{
   return eafTypes::statusError;
}

void pgsUnknownErrorStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   pgsUnknownErrorStatusItem* pItem = dynamic_cast<pgsUnknownErrorStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   CString msg;
   msg.Format(_T("An unspecified error occured at %s, Line %d"),pItem->m_File.c_str(),pItem->m_Line);
   AfxMessageBox(msg,MB_OK | MB_ICONEXCLAMATION);
}

////////////////

pgsInformationalStatusItem::pgsInformationalStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
CEAFStatusItem(statusGroupID,callbackID,strDescription)
{
}

bool pgsInformationalStatusItem::IsEqual(CEAFStatusItem* pOther)
{
   pgsInformationalStatusItem* other = dynamic_cast<pgsInformationalStatusItem*>(pOther);
   if ( !other )
      return false;

   if ( GetDescription() != other->GetDescription())
      return false;

   return true;
}

//////////////////////////////////////////////////////////
pgsInformationalStatusCallback::pgsInformationalStatusCallback(eafTypes::StatusSeverityType severity,UINT helpID):
m_Severity(severity), m_HelpID(helpID)
{
}

eafTypes::StatusSeverityType pgsInformationalStatusCallback::GetSeverity()
{
   return m_Severity;
}

void pgsInformationalStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   std::_tstring msg = pStatusItem->GetDescription();

   if (m_HelpID!=0)
   {
      msg += std::_tstring(_T("\r\n\r\nClick on Help button for more details."));
   }

   bool is_severe = m_Severity==eafTypes::statusError;
   if (!is_severe)
   {
      msg += std::_tstring(_T("\r\n\r\nClick OK to remove this message."));
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

pgsGirderDescriptionStatusItem::pgsGirderDescriptionStatusItem(SpanIndexType span,GirderIndexType gdr,Uint16 page,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
pgsSpanGirderRelatedStatusItem(statusGroupID,callbackID,strDescription,span,gdr), m_Span(span), m_Girder(gdr), m_Page(page)
{
}

bool pgsGirderDescriptionStatusItem::IsEqual(CEAFStatusItem* pOther)
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
pgsGirderDescriptionStatusCallback::pgsGirderDescriptionStatusCallback(IBroker* pBroker,eafTypes::StatusSeverityType severity):
m_pBroker(pBroker), m_Severity(severity)
{
}

eafTypes::StatusSeverityType pgsGirderDescriptionStatusCallback::GetSeverity()
{
   return m_Severity;
}

void pgsGirderDescriptionStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   pgsGirderDescriptionStatusItem* pItem = dynamic_cast<pgsGirderDescriptionStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   CString strMessage;
   strMessage.Format(_T("%s\n\r%s"),pItem->GetDescription().c_str(),_T("Would you like to edit the girder?"));
   int result = AfxMessageBox(strMessage,MB_YESNO);

   if ( result == IDYES )
   {
      GET_IFACE(IEditByUI,pEdit);

      if (pEdit->EditGirderDescription(pItem->m_Span,pItem->m_Girder,pItem->m_Page))
      {
         // assume that edit took care of status
         StatusItemIDType id = pItem->GetID();
         GET_IFACE(IEAFStatusCenter,pStatusCenter);
         pStatusCenter->RemoveByID(id);
      }
   }
}

////////////////

pgsStructuralAnalysisTypeStatusItem::pgsStructuralAnalysisTypeStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
CEAFStatusItem(statusGroupID,callbackID,strDescription)
{
}

bool pgsStructuralAnalysisTypeStatusItem::IsEqual(CEAFStatusItem* pOther)
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

eafTypes::StatusSeverityType pgsStructuralAnalysisTypeStatusCallback::GetSeverity()
{
   return eafTypes::statusWarning;
}

void pgsStructuralAnalysisTypeStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   AfxMessageBox(pStatusItem->GetDescription().c_str(),MB_OK);
}

pgsBridgeDescriptionStatusItem::pgsBridgeDescriptionStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,long dlgPage,LPCTSTR strDescription) :
CEAFStatusItem(statusGroupID,callbackID,strDescription), m_DlgPage(dlgPage)
{
}

bool pgsBridgeDescriptionStatusItem::IsEqual(CEAFStatusItem* pOther)
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
pgsBridgeDescriptionStatusCallback::pgsBridgeDescriptionStatusCallback(IBroker* pBroker,eafTypes::StatusSeverityType severity):
m_pBroker(pBroker), m_Severity(severity)
{
}

eafTypes::StatusSeverityType pgsBridgeDescriptionStatusCallback::GetSeverity()
{
   return m_Severity;
}

void pgsBridgeDescriptionStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   pgsBridgeDescriptionStatusItem* pItem = dynamic_cast<pgsBridgeDescriptionStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   GET_IFACE(IEditByUI,pEdit);
   pEdit->EditBridgeDescription(pItem->m_DlgPage);
}

//////////////////////////////////////////////////////////
pgsLldfWarningStatusItem::pgsLldfWarningStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
CEAFStatusItem(statusGroupID,callbackID,strDescription)
{
}

bool pgsLldfWarningStatusItem::IsEqual(CEAFStatusItem* pOther)
{
   pgsLldfWarningStatusItem* other = dynamic_cast<pgsLldfWarningStatusItem*>(pOther);
   if ( !other )
      return false;

   return true;
}

//////////////////////////////////////////////////////////
pgsLldfWarningStatusCallback::pgsLldfWarningStatusCallback(IBroker* pBroker):
m_pBroker(pBroker)
{
}

eafTypes::StatusSeverityType pgsLldfWarningStatusCallback::GetSeverity()
{
   return eafTypes::statusOK;
}

void pgsLldfWarningStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   pgsLldfWarningStatusItem* pItem = dynamic_cast<pgsLldfWarningStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   // Just go straight to main lldf  editing dialog
   GET_IFACE(ILiveLoads,pLiveLoads);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   pgsTypes::DistributionFactorMethod method = pBridgeDesc->GetDistributionFactorMethod();
   LldfRangeOfApplicabilityAction roaAction = pLiveLoads->GetLldfRangeOfApplicabilityAction();

   GET_IFACE(IEditByUI,pEdit);
   pEdit->EditLiveLoadDistributionFactors(method,roaAction);
}
