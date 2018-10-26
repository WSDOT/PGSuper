///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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
#include <EAF\EAFTransactions.h>
#include <EAF\EAFUtilities.h>

#include "RefinedAnalysisOptionsDlg.h"
#include "BoundaryConditionDlg.h"

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\EditBridge.h>

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
   {
      return false;
   }

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
   ATLASSERT(pItem!=nullptr);

   CRefinedAnalysisOptionsDlg dlg;
   dlg.m_strDescription = pStatusItem->GetDescription();
   dlg.m_strDescription.Replace(_T("\n"),_T("\r\n")); // this makes the text wrap correctly in the dialog

   if ( dlg.DoModal() == IDOK )
   {
      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

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
         {
         GET_IFACE(ILiveLoads,pLiveLoads);
         method = pBridgeDesc->GetDistributionFactorMethod();
         roaAction = pLiveLoads->GetLldfRangeOfApplicabilityAction();
         }
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
   {
      return false;
   }

   if ( m_Component != other->m_Component)
   {
      return false;
   }

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
   pgsInstallationErrorStatusItem* pItem = dynamic_cast<pgsInstallationErrorStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   CString msg;
   msg.Format(_T("The software was not successfully installed or has become damanged.\n\n%s could not be created.\n\nPlease re-install the software."),pItem->m_Component.c_str());
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
   {
      return false;
   }

   if ( m_File != other->m_File )
   {
      return false;
   }

   if ( m_Line != other->m_Line )
   {
      return false;
   }

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
   ATLASSERT(pItem!=nullptr);

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
   {
      return false;
   }

   if ( CString(GetDescription()) != CString(other->GetDescription()) )
   {
      return false;
   }

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
   EAFShowStatusMessage(pStatusItem,m_Severity,FALSE,AfxGetAppName(),m_HelpID);
}


////////////////

pgsProjectCriteriaStatusItem::pgsProjectCriteriaStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
CEAFStatusItem(statusGroupID,callbackID,strDescription)
{
}

bool pgsProjectCriteriaStatusItem::IsEqual(CEAFStatusItem* pOther)
{
   pgsProjectCriteriaStatusItem* other = dynamic_cast<pgsProjectCriteriaStatusItem*>(pOther);
   if ( !other )
   {
      return false;
   }

   if ( CString(GetDescription()) != CString(other->GetDescription()) )
   {
      return false;
   }

   return true;
}

//////////////////////////////////////////////////////////
pgsProjectCriteriaStatusCallback::pgsProjectCriteriaStatusCallback(IBroker* pBroker):
m_pBroker(pBroker)
{
   m_HelpID = 0;
}

eafTypes::StatusSeverityType pgsProjectCriteriaStatusCallback::GetSeverity()
{
   return eafTypes::statusError;
}

void pgsProjectCriteriaStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   //pgsInformationalStatusItem* pItem = dynamic_cast<pgsInformationalStatusItem*>(pStatusItem);
   //ATLASSERT(pItem!=nullptr);

   std::_tstring msg = pStatusItem->GetDescription();

   GET_IFACE(ISpecification,pSpec);
   std::_tstring strSpec(pSpec->GetSpecification());
   GET_IFACE(ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(strSpec.c_str());
   if ( pSpecEntry->IsEditingEnabled() )
   {
      SpecLibrary* pSpecLibrary = pLib->GetSpecLibrary();
      pSpecLibrary->EditEntry(pSpecEntry->GetName().c_str(),SPEC_PAGE_LOSSES);
   }
   else
   {
      msg += _T("\r\n\r\nWould you like to select a different Project Criteria?");
      if ( AfxMessageBox(msg.c_str(),MB_YESNO | MB_ICONQUESTION) == IDYES )
      {
         GET_IFACE(IEditByUI,pEdit);
         pEdit->SelectProjectCriteria();
      }
   }
}

////////////////

pgsGirderDescriptionStatusItem::pgsGirderDescriptionStatusItem(const CSegmentKey& segmentKey,Uint16 page,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
pgsSegmentRelatedStatusItem(statusGroupID,callbackID,strDescription,segmentKey), m_SegmentKey(segmentKey), m_Page(page)
{
}

bool pgsGirderDescriptionStatusItem::IsEqual(CEAFStatusItem* pOther)
{
   pgsGirderDescriptionStatusItem* other = dynamic_cast<pgsGirderDescriptionStatusItem*>(pOther);
   if ( !other )
   {
      return false;
   }

   if ( CString(this->GetDescription()) != CString(other->GetDescription()) )
   {
      return false;
   }

   if ( m_SegmentKey != other->m_SegmentKey )
   {
      return false;
   }

   if ( m_Page != other->m_Page )
   {
      return false;
   }

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
   ATLASSERT(pItem!=nullptr);

   CString strMessage;
   strMessage.Format(_T("%s\n\r%s"),pItem->GetDescription(),_T("Would you like to edit the girder?"));
   int result = AfxMessageBox(strMessage,MB_YESNO);

   if ( result == IDYES )
   {
      GET_IFACE(IEditByUI,pEdit);

      if (pEdit->EditSegmentDescription(pItem->m_SegmentKey,pItem->m_Page))
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
   {
      return false;
   }

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
   AfxMessageBox(pStatusItem->GetDescription(),MB_OK);
}

pgsBridgeDescriptionStatusItem::pgsBridgeDescriptionStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,pgsBridgeDescriptionStatusItem::IssueType issueType,LPCTSTR strDescription) :
CEAFStatusItem(statusGroupID,callbackID,strDescription), m_IssueType(issueType)
{
}

bool pgsBridgeDescriptionStatusItem::IsEqual(CEAFStatusItem* pOther)
{
   pgsBridgeDescriptionStatusItem* other = dynamic_cast<pgsBridgeDescriptionStatusItem*>(pOther);
   if ( !other )
   {
      return false;
   }

   if ( CString(this->GetDescription()) != CString(other->GetDescription()) )
   {
      return false;
   }

   if ( m_IssueType != other->m_IssueType )
   {
      return false;
   }

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
   ATLASSERT(pItem!=nullptr);

   long dlgPage;
   switch(pItem->m_IssueType)
   {
   case pgsBridgeDescriptionStatusItem::General:
      dlgPage = 0;
      break;

   case pgsBridgeDescriptionStatusItem::Framing:
      dlgPage = 1;
      break;

   case pgsBridgeDescriptionStatusItem::Railing:
      dlgPage = 2;
      break;

   case pgsBridgeDescriptionStatusItem::Deck:
      dlgPage = 3;
      break;

   default:
      dlgPage = -1;
   }

   if (0 <= dlgPage)
   {
      GET_IFACE(IEditByUI,pEdit);
      pEdit->EditBridgeDescription(dlgPage);
   }
   else
   {
      if ( pItem->m_IssueType == pgsBridgeDescriptionStatusItem::BoundaryConditions )
      {
         CBoundaryConditionDlg dlg;
         dlg.m_PierIdx = 1;
         if ( dlg.DoModal() == IDOK )
         {
            GET_IFACE(IBridgeDescription,pBridgeDesc);
            CBridgeDescription2 bridge = *pBridgeDesc->GetBridgeDescription();
            CPierData2* pPier = bridge.GetPier(dlg.m_PierIdx);
#pragma Reminder("REVIEW: is this only for boundary piers?") // should InteriorPiers be included too?
            // When the concept of Boundary and Interior Piers was added, this wasn't given any thought...
            // it was just made to compile
            ATLASSERT(pPier->IsBoundaryPier()); // this assumes we have a boundary pier
            CSpanData2* pNextSpan = pPier->GetNextSpan();
            while ( pNextSpan )
            {
               pPier->SetBoundaryConditionType(dlg.m_BoundaryCondition);
               pPier = pNextSpan->GetNextPier();
               pNextSpan = pPier->GetNextSpan();
            }

            txnEditBridge* pTxn = new txnEditBridge(*pBridgeDesc->GetBridgeDescription(), bridge);

            GET_IFACE(IEAFTransactions,pTransactions);
            pTransactions->Execute(pTxn);
         }
      }
   }
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
   {
      return false;
   }

   return true;
}

//////////////////////////////////////////////////////////
pgsLldfWarningStatusCallback::pgsLldfWarningStatusCallback(IBroker* pBroker):
m_pBroker(pBroker)
{
}

eafTypes::StatusSeverityType pgsLldfWarningStatusCallback::GetSeverity()
{
   return eafTypes::statusInformation;
}

void pgsLldfWarningStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   pgsLldfWarningStatusItem* pItem = dynamic_cast<pgsLldfWarningStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   // Just go straight to main lldf  editing dialog
   GET_IFACE(ILiveLoads,pLiveLoads);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   pgsTypes::DistributionFactorMethod method = pBridgeDesc->GetDistributionFactorMethod();
   LldfRangeOfApplicabilityAction roaAction = pLiveLoads->GetLldfRangeOfApplicabilityAction();

   GET_IFACE(IEditByUI,pEdit);
   pEdit->EditLiveLoadDistributionFactors(method,roaAction);
}

//////////////////////////////////////////////////////////
pgsEffectiveFlangeWidthStatusItem::pgsEffectiveFlangeWidthStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
CEAFStatusItem(statusGroupID,callbackID,strDescription)
{
}

bool pgsEffectiveFlangeWidthStatusItem::IsEqual(CEAFStatusItem* pOther)
{
   // we only want one of these in the status center
   pgsEffectiveFlangeWidthStatusItem* other = dynamic_cast<pgsEffectiveFlangeWidthStatusItem*>(pOther);
   if ( !other )
   {
      return false;
   }

   return true;
}

//////////////////////////////////////////////////////////
pgsEffectiveFlangeWidthStatusCallback::pgsEffectiveFlangeWidthStatusCallback(IBroker* pBroker,eafTypes::StatusSeverityType severity):
m_pBroker(pBroker),
m_Severity(severity)
{
}

eafTypes::StatusSeverityType pgsEffectiveFlangeWidthStatusCallback::GetSeverity()
{
   return m_Severity;
}

void pgsEffectiveFlangeWidthStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   pgsEffectiveFlangeWidthStatusItem* pItem = dynamic_cast<pgsEffectiveFlangeWidthStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   // Just go straight to main editing dialog
   GET_IFACE(IEditByUI,pEdit);
   pEdit->EditEffectiveFlangeWidth();
}


pgsTimelineStatusItem::pgsTimelineStatusItem(StatusGroupIDType statusGroupID, StatusCallbackIDType callbackID, LPCTSTR strDescription) :
   CEAFStatusItem(statusGroupID, callbackID, strDescription)
{
}

bool pgsTimelineStatusItem::IsEqual(CEAFStatusItem* pOther)
{
   pgsTimelineStatusItem* other = dynamic_cast<pgsTimelineStatusItem*>(pOther);
   if (!other)
   {
      return false;
   }

   if (CString(this->GetDescription()) != CString(other->GetDescription()))
   {
      return false;
   }

   return true;
}

//////////////////////////////////////////////////////////
pgsTimelineStatusCallback::pgsTimelineStatusCallback(IBroker* pBroker, eafTypes::StatusSeverityType severity) :
   m_pBroker(pBroker), m_Severity(severity)
{
}

eafTypes::StatusSeverityType pgsTimelineStatusCallback::GetSeverity()
{
   return m_Severity;
}

void pgsTimelineStatusCallback::Execute(CEAFStatusItem* pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   pgsTimelineStatusItem* pItem = dynamic_cast<pgsTimelineStatusItem*>(pStatusItem);
   ATLASSERT(pItem != nullptr);

   GET_IFACE(IEditByUI, pEdit);

   if (pEdit->EditTimeline())
   {
      // assume that edit took care of status
      StatusItemIDType id = pItem->GetID();
      GET_IFACE(IEAFStatusCenter, pStatusCenter);
      pStatusCenter->RemoveByID(id);
   }
}
