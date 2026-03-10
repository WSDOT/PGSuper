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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\StatusItem.h>

#include "resource.h"

#include <IFace/Tools.h>
#include <IFace\EditByUI.h>
#include <IFace/Project.h>
#include <EAF\EAFTransactions.h>

#include "RefinedAnalysisOptionsDlg.h"
#include "BoundaryConditionDlg.h"

#include <PsgLib\BridgeDescription2.h>
#include <psgLib/LibraryManager.h>
#include <psgLib/SpecLibraryEntry.h>

#include <PgsExt\EditBridge.h>

pgsRefinedAnalysisStatusItem::pgsRefinedAnalysisStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
WBFL::EAF::StatusItem(statusGroupID,callbackID,strDescription)
{
}

bool pgsRefinedAnalysisStatusItem::IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const
{
   auto other = std::dynamic_pointer_cast<const pgsRefinedAnalysisStatusItem>(pOther);
   if ( !other )
   {
      return false;
   }

   return true;
}

//////////////////////////////////////////////////////////
pgsRefinedAnalysisStatusCallback::pgsRefinedAnalysisStatusCallback()
{
}

WBFL::EAF::StatusSeverityType pgsRefinedAnalysisStatusCallback::GetSeverity() const
{
   return WBFL::EAF::StatusSeverityType::Error;
}

void pgsRefinedAnalysisStatusCallback::Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   auto pItem = std::dynamic_pointer_cast<pgsRefinedAnalysisStatusItem>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   CRefinedAnalysisOptionsDlg dlg;
   dlg.m_strDescription = pStatusItem->GetDescription();
   dlg.m_strDescription.Replace(_T("\n"),_T("\r\n")); // this makes the text wrap correctly in the dialog

   if ( dlg.DoModal() == IDOK )
   {
      auto broker = EAFGetBroker();
      GET_IFACE2(broker,IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

      pgsTypes::DistributionFactorMethod method;
      WBFL::LRFD::RangeOfApplicabilityAction roaAction;
      switch(dlg.m_Choice)
      {
      case CRefinedAnalysisOptionsDlg::lldfDirectInput:
         method = pgsTypes::DirectlyInput;
         roaAction = WBFL::LRFD::RangeOfApplicabilityAction::Ignore;
         break;

      case CRefinedAnalysisOptionsDlg::lldfIgnore:
         method = pgsTypes::Calculated;
         roaAction = WBFL::LRFD::RangeOfApplicabilityAction::Ignore;
         break;

      case CRefinedAnalysisOptionsDlg::lldfIgnoreLever:
         method = pgsTypes::Calculated;
         roaAction = WBFL::LRFD::RangeOfApplicabilityAction::IgnoreUseLeverRule;
         break;

      case CRefinedAnalysisOptionsDlg::lldfForceLever:
         method = pgsTypes::LeverRule;
         roaAction = WBFL::LRFD::RangeOfApplicabilityAction::Ignore;
         break;

      case CRefinedAnalysisOptionsDlg::lldfDefault:
         {
         GET_IFACE2(broker,ILiveLoads,pLiveLoads);
         method = pBridgeDesc->GetDistributionFactorMethod();
         roaAction = pLiveLoads->GetRangeOfApplicabilityAction();
         }
         break;

      default:
         ATLASSERT(false); // is there a new choice???
      }

      GET_IFACE2(broker,IEditByUI,pEdit);
      pEdit->EditLiveLoadDistributionFactors(method,roaAction);
   }
}

////////////////

pgsInstallationErrorStatusItem::pgsInstallationErrorStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strComponent,LPCTSTR strDescription) :
WBFL::EAF::StatusItem(statusGroupID,callbackID,strDescription), m_Component(strComponent)
{
}

bool pgsInstallationErrorStatusItem::IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const
{
   auto other = std::dynamic_pointer_cast<const pgsInstallationErrorStatusItem>(pOther);
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

WBFL::EAF::StatusSeverityType pgsInstallationErrorStatusCallback::GetSeverity() const
{
   return WBFL::EAF::StatusSeverityType::Error;
}

void pgsInstallationErrorStatusCallback::Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem)
{
   auto pItem = std::dynamic_pointer_cast<pgsInstallationErrorStatusItem>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   CString msg;
   msg.Format(_T("The software was not successfully installed or has become damaged.\n\n%s could not be created.\n\nPlease re-install the software."),pItem->m_Component.c_str());
   AfxMessageBox(msg,MB_OK | MB_ICONEXCLAMATION);
}


////////////////

pgsUnknownErrorStatusItem::pgsUnknownErrorStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strFile,long line,LPCTSTR strDescription) :
WBFL::EAF::StatusItem(statusGroupID,callbackID,strDescription), m_File(strFile), m_Line(line)
{
}

bool pgsUnknownErrorStatusItem::IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const
{
   auto other = std::dynamic_pointer_cast<const pgsUnknownErrorStatusItem>(pOther);
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

WBFL::EAF::StatusSeverityType pgsUnknownErrorStatusCallback::GetSeverity() const
{
   return WBFL::EAF::StatusSeverityType::Error;
}

void pgsUnknownErrorStatusCallback::Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   auto pItem = std::dynamic_pointer_cast<pgsUnknownErrorStatusItem>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   CString msg;
   msg.Format(_T("An unspecified error occurred at %s, Line %d"),pItem->m_File.c_str(),pItem->m_Line);
   AfxMessageBox(msg,MB_OK | MB_ICONEXCLAMATION);
}

////////////////

pgsInformationalStatusItem::pgsInformationalStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
WBFL::EAF::StatusItem(statusGroupID,callbackID,strDescription)
{
}

pgsInformationalStatusItem::~pgsInformationalStatusItem()
{
}

bool pgsInformationalStatusItem::IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const
{
   auto other = std::dynamic_pointer_cast<const pgsInformationalStatusItem>(pOther);
   if ( !other )
   {
      return false;
   }

   return 0 == CompareDescriptions(pOther);
}

//////////////////////////////////////////////////////////
pgsInformationalStatusCallback::pgsInformationalStatusCallback(WBFL::EAF::StatusSeverityType severity,UINT helpID):
m_Severity(severity), m_HelpID(helpID)
{
}

WBFL::EAF::StatusSeverityType pgsInformationalStatusCallback::GetSeverity() const
{
   return m_Severity;
}

void pgsInformationalStatusCallback::Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   EAFShowStatusMessage(pStatusItem,m_Severity,FALSE,FALSE,AfxGetAppName(),m_HelpID);
}


////////////////

pgsProjectCriteriaStatusItem::pgsProjectCriteriaStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
WBFL::EAF::StatusItem(statusGroupID,callbackID,strDescription)
{
}

bool pgsProjectCriteriaStatusItem::IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const
{
   auto other = std::dynamic_pointer_cast<const pgsProjectCriteriaStatusItem>(pOther);
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
pgsProjectCriteriaStatusCallback::pgsProjectCriteriaStatusCallback()
{
   m_HelpID = 0;
}

WBFL::EAF::StatusSeverityType pgsProjectCriteriaStatusCallback::GetSeverity() const
{
   return WBFL::EAF::StatusSeverityType::Error;
}

void pgsProjectCriteriaStatusCallback::Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   //pgsInformationalStatusItem* pItem = dynamic_cast<pgsInformationalStatusItem*>(pStatusItem);
   //ATLASSERT(pItem!=nullptr);

   std::_tstring msg = pStatusItem->GetDescription();
   auto broker = EAFGetBroker();
   GET_IFACE2(broker,ISpecification,pSpec);
   std::_tstring strSpec(pSpec->GetSpecification());
   GET_IFACE2(broker,ILibrary,pLib);
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
         GET_IFACE2(broker,IEditByUI,pEdit);
         pEdit->SelectProjectCriteria();
      }
   }
}

////////////////

pgsGirderDescriptionStatusItem::pgsGirderDescriptionStatusItem(const CSegmentKey& segmentKey,Uint16 page,StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
pgsSegmentRelatedStatusItem(statusGroupID,callbackID,strDescription,segmentKey), m_SegmentKey(segmentKey), m_Page(page)
{
}

bool pgsGirderDescriptionStatusItem::IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const
{
   auto other = std::dynamic_pointer_cast<const pgsGirderDescriptionStatusItem>(pOther);
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
pgsGirderDescriptionStatusCallback::pgsGirderDescriptionStatusCallback(WBFL::EAF::StatusSeverityType severity):
m_Severity(severity)
{
}

WBFL::EAF::StatusSeverityType pgsGirderDescriptionStatusCallback::GetSeverity() const
{
   return m_Severity;
}

void pgsGirderDescriptionStatusCallback::Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem)
{
   auto pItem = std::dynamic_pointer_cast<pgsGirderDescriptionStatusItem>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   CString strMessage;
   strMessage.Format(_T("%s\n\n%s"),pItem->GetDescription(),_T("Would you like to edit the girder?"));
   int result = AfxMessageBox(strMessage,MB_YESNO);

   if ( result == IDYES )
   {
      auto broker = EAFGetBroker();
      GET_IFACE2(broker,IEditByUI,pEdit);

      if (pItem->m_SegmentKey.segmentIndex == INVALID_INDEX ? pEdit->EditGirderDescription(pItem->m_SegmentKey, pItem->m_Page) : pEdit->EditSegmentDescription(pItem->m_SegmentKey, pItem->m_Page))
      {
         // assume that edit took care of status
         StatusItemIDType id = pItem->GetID();
         GET_IFACE2(broker,IEAFStatusCenter, pStatusCenter);
         pStatusCenter->RemoveByID(id);
      }
   }
}

////////////////

pgsStructuralAnalysisTypeStatusItem::pgsStructuralAnalysisTypeStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
WBFL::EAF::StatusItem(statusGroupID,callbackID,strDescription)
{
}

bool pgsStructuralAnalysisTypeStatusItem::IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const
{
   auto other = std::dynamic_pointer_cast<const pgsStructuralAnalysisTypeStatusItem>(pOther);
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

WBFL::EAF::StatusSeverityType pgsStructuralAnalysisTypeStatusCallback::GetSeverity() const
{
   return WBFL::EAF::StatusSeverityType::Warning;
}

void pgsStructuralAnalysisTypeStatusCallback::Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem)
{
   AfxMessageBox(pStatusItem->GetDescription(),MB_OK);
}

pgsBridgeDescriptionStatusItem::pgsBridgeDescriptionStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,pgsBridgeDescriptionStatusItem::IssueType issueType,LPCTSTR strDescription) :
WBFL::EAF::StatusItem(statusGroupID,callbackID,strDescription), m_IssueType(issueType)
{
}

bool pgsBridgeDescriptionStatusItem::IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const
{
   auto other = std::dynamic_pointer_cast<const pgsBridgeDescriptionStatusItem>(pOther);
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
pgsBridgeDescriptionStatusCallback::pgsBridgeDescriptionStatusCallback(WBFL::EAF::StatusSeverityType severity):
m_Severity(severity)
{
}

WBFL::EAF::StatusSeverityType pgsBridgeDescriptionStatusCallback::GetSeverity() const
{
   return m_Severity;
}

void pgsBridgeDescriptionStatusCallback::Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem)
{
   auto pItem = std::dynamic_pointer_cast<pgsBridgeDescriptionStatusItem>(pStatusItem);
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

   auto broker = EAFGetBroker();

   if (0 <= dlgPage)
   {
      GET_IFACE2(broker,IEditByUI,pEdit);
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
            GET_IFACE2(broker,IBridgeDescription,pBridgeDesc);
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

            std::unique_ptr<txnEditBridge> pTxn(std::make_unique<txnEditBridge>(*pBridgeDesc->GetBridgeDescription(), bridge));

            GET_IFACE2(broker,IEAFTransactions,pTransactions);
            pTransactions->Execute(std::move(pTxn));
         }
      }
      else if (pItem->m_IssueType == pgsBridgeDescriptionStatusItem::Bearings)
      {
         // Show status item in larger area so we can see it.
         AFX_MANAGE_STATE(AfxGetStaticModuleState());
         auto retval = EAFShowStatusMessage(pItem,m_Severity,FALSE,TRUE,AfxGetAppName(),NULL);

         bool didEdit(false);
         if (retval == WBFL::EAF::StatusItemDisplayReturn::Edit)
         {
            GET_IFACE2(broker,IEditByUI, pEdit);
            didEdit = pEdit->EditBearings();
         }

         if (retval == WBFL::EAF::StatusItemDisplayReturn::Remove || didEdit)
         {
            // assume that edit took care of status
            StatusItemIDType id = pItem->GetID();
            GET_IFACE2(broker,IEAFStatusCenter,pStatusCenter);
            pStatusCenter->RemoveByID(id);
         }
      }
      else if (pItem->m_IssueType == pgsBridgeDescriptionStatusItem::DeckCasting)
      {
         AFX_MANAGE_STATE(AfxGetStaticModuleState());
         auto retval = EAFShowStatusMessage(pItem, m_Severity, FALSE, TRUE, AfxGetAppName(), NULL);

         bool didEdit(false);
         if (retval == WBFL::EAF::StatusItemDisplayReturn::Edit)
         {
            GET_IFACE2(broker,IEditByUI, pEdit);
            didEdit = pEdit->EditCastDeckActivity();
         }

         if (retval == WBFL::EAF::StatusItemDisplayReturn::Remove || didEdit)
         {
            // assume that edit took care of status
            StatusItemIDType id = pItem->GetID();
            GET_IFACE2(broker,IEAFStatusCenter, pStatusCenter);
            pStatusCenter->RemoveByID(id);
         }
      }
      else
      {
         ATLASSERT(false); // is there a new issue type?
      }
   }
}

//////////////////////////////////////////////////////////
pgsLldfWarningStatusItem::pgsLldfWarningStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
WBFL::EAF::StatusItem(statusGroupID,callbackID,strDescription)
{
}

bool pgsLldfWarningStatusItem::IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const
{
   auto other = std::dynamic_pointer_cast<const pgsLldfWarningStatusItem>(pOther);
   if ( !other )
   {
      return false;
   }

   return true;
}

//////////////////////////////////////////////////////////
pgsLldfWarningStatusCallback::pgsLldfWarningStatusCallback()
{
}

WBFL::EAF::StatusSeverityType pgsLldfWarningStatusCallback::GetSeverity() const
{
   return WBFL::EAF::StatusSeverityType::Information;
}

void pgsLldfWarningStatusCallback::Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem)
{
   auto pItem = std::dynamic_pointer_cast<pgsLldfWarningStatusItem>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   // Just go straight to main LLDF editing dialog
   auto broker = EAFGetBroker();
   GET_IFACE2(broker,ILiveLoads,pLiveLoads);
   GET_IFACE2(broker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   pgsTypes::DistributionFactorMethod method = pBridgeDesc->GetDistributionFactorMethod();
   auto roaAction = pLiveLoads->GetRangeOfApplicabilityAction();

   GET_IFACE2(broker,IEditByUI,pEdit);
   pEdit->EditLiveLoadDistributionFactors(method,roaAction);
}

//////////////////////////////////////////////////////////
pgsEffectiveFlangeWidthStatusItem::pgsEffectiveFlangeWidthStatusItem(StatusGroupIDType statusGroupID,StatusCallbackIDType callbackID,LPCTSTR strDescription) :
WBFL::EAF::StatusItem(statusGroupID,callbackID,strDescription)
{
}

bool pgsEffectiveFlangeWidthStatusItem::IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const
{
   // we only want one of these in the status center
   auto other = std::dynamic_pointer_cast<const pgsEffectiveFlangeWidthStatusItem>(pOther);
   if ( !other )
   {
      return false;
   }

   return true;
}

//////////////////////////////////////////////////////////
pgsEffectiveFlangeWidthStatusCallback::pgsEffectiveFlangeWidthStatusCallback(WBFL::EAF::StatusSeverityType severity):
m_Severity(severity)
{
}

WBFL::EAF::StatusSeverityType pgsEffectiveFlangeWidthStatusCallback::GetSeverity() const
{
   return m_Severity;
}

void pgsEffectiveFlangeWidthStatusCallback::Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem)
{
   auto pItem = std::dynamic_pointer_cast<pgsEffectiveFlangeWidthStatusItem>(pStatusItem);
   ATLASSERT(pItem!=nullptr);

   // Just go straight to main editing dialog
   auto broker = EAFGetBroker();
   GET_IFACE2(broker,IEditByUI,pEdit);
   pEdit->EditEffectiveFlangeWidth();
}


pgsTimelineStatusItem::pgsTimelineStatusItem(StatusGroupIDType statusGroupID, StatusCallbackIDType callbackID, LPCTSTR strDescription) :
   WBFL::EAF::StatusItem(statusGroupID, callbackID, strDescription)
{
}

bool pgsTimelineStatusItem::IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const
{
   auto other = std::dynamic_pointer_cast<const pgsTimelineStatusItem>(pOther);
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
pgsTimelineStatusCallback::pgsTimelineStatusCallback(WBFL::EAF::StatusSeverityType severity) :
   m_Severity(severity)
{
}

WBFL::EAF::StatusSeverityType pgsTimelineStatusCallback::GetSeverity() const
{
   return m_Severity;
}

void pgsTimelineStatusCallback::Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem)
{
   auto pItem = std::dynamic_pointer_cast<pgsTimelineStatusItem>(pStatusItem);
   ATLASSERT(pItem != nullptr);

   CString str(pItem->GetDescription());
   AfxMessageBox(str, MB_ICONEXCLAMATION);

   auto broker = EAFGetBroker();
   GET_IFACE2(broker,IEditByUI, pEdit);

   if (pEdit->EditTimeline())
   {
      // assume that edit took care of status
      StatusItemIDType id = pItem->GetID();
      GET_IFACE2(broker, IEAFStatusCenter, pStatusCenter);
      pStatusCenter->RemoveByID(id);
   }
}


//////////////////////////////////////////////////////////
pgsConnectionGeometryStatusItem::pgsConnectionGeometryStatusItem(StatusGroupIDType statusGroupID, StatusCallbackIDType callbackID, PierIndexType pierIdx, LPCTSTR strDescription) :
   WBFL::EAF::StatusItem(statusGroupID, callbackID, strDescription),
   m_PierIdx(pierIdx)
{
}

bool pgsConnectionGeometryStatusItem::IsEqual(std::shared_ptr<const WBFL::EAF::StatusItem> pOther) const
{
   auto other = std::dynamic_pointer_cast<const pgsConnectionGeometryStatusItem>(pOther);
   if (!other)
   {
      return false;
   }


   if (CString(this->GetDescription()) != CString(other->GetDescription()))
   {
      return false;
   }

   if (m_PierIdx != other->m_PierIdx)
   {
      return false;
   }

   return true;
}


pgsConnectionGeometryStatusCallback::pgsConnectionGeometryStatusCallback(WBFL::EAF::StatusSeverityType severity) :
   m_Severity(severity)
{
}

WBFL::EAF::StatusSeverityType pgsConnectionGeometryStatusCallback::GetSeverity() const
{
   return m_Severity;
}

void pgsConnectionGeometryStatusCallback::Execute(std::shared_ptr<WBFL::EAF::StatusItem> pStatusItem)
{
   auto pItem = std::dynamic_pointer_cast<pgsConnectionGeometryStatusItem>(pStatusItem);
   ATLASSERT(pItem != nullptr);

   CString str;
   str.Format(_T("%s"),pItem->GetDescription());
   AfxMessageBox(str, MB_ICONEXCLAMATION);

   auto broker = EAFGetBroker();
   GET_IFACE2(broker,IEditByUI, pEdit);

   if (pEdit->EditPierDescription(pItem->m_PierIdx,EPD_CONNECTION))
   {
      // assume that edit took care of status
      StatusItemIDType id = pItem->GetID();
      GET_IFACE2(broker, IEAFStatusCenter, pStatusCenter);
      pStatusCenter->RemoveByID(id);
   }
}
