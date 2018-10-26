///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 2004  Washington State Department of Transportation
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

#include "stdafx.h"
#include "resource.h"
#include "PGSuper.h"
#include "MainFrm.h"

#include "StatusCallbacks.h"
#include <pgsExt\StatusItem.h>

#include "DealWithLoadDlg.h"
#include "EditPointLoadDlg.h"
#include "EditDistributedLoadDlg.h"
#include "EditMomentLoadDlg.h"
#include "StatusMessageDialog.h"

#include <IFace\Project.h>
#include <PgsExt\PointLoadData.h>

#include "RefinedAnalysisOptionsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPointLoadStatusCallback::CPointLoadStatusCallback(CPGSuperDoc* pDoc, long statusLevel):
m_pDoc(pDoc),
m_StatusLevel(statusLevel)
{
}

long CPointLoadStatusCallback::GetSeverity()
{
   return m_StatusLevel;
}

void CPointLoadStatusCallback::Execute(pgsStatusItem* pStatusItem)
{

   pgsPointLoadStatusItem* pItem = dynamic_cast<pgsPointLoadStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   CDealWithLoadDlg dlg;
   dlg.m_Message = pItem->GetDescription().c_str();

   int st = dlg.DoModal();

   if (st!=IDCANCEL)
   {
      CComPtr<IBroker> pBroker;
      m_pDoc->GetBroker(&pBroker);
      GET_IFACE2(pBroker, IUserDefinedLoadData, pUdl);

      // delete
      if ( st==CDealWithLoadDlg::IDDELETELOAD )
      {
         pUdl->DeletePointLoad(pItem->m_LoadIndex);

         long id = pItem->GetID();
         m_pDoc->GetStatusCenter().RemoveByID(id);
      }
      // edit
      else if (st==CDealWithLoadDlg::IDEDITLOAD)
      {
         CPointLoadData rld = pUdl->GetPointLoad(pItem->m_LoadIndex);

   	   CEditPointLoadDlg dlgl(rld,pBroker);
         if (dlgl.DoModal() == IDOK)
         {
            // only update if changed
            if (rld!=dlgl.m_Load)
            {
               pUdl->UpdatePointLoad(pItem->m_LoadIndex, dlgl.m_Load);

               long id = pItem->GetID();
               m_pDoc->GetStatusCenter().RemoveByID(id);
            }
         }
      }
      else
      {
         ATLASSERT(0);
      }
   }
}

//////////////////////////////////////////////////////////
CDistributedLoadStatusCallback::CDistributedLoadStatusCallback(CPGSuperDoc* pDoc, long statusLevel):
m_pDoc(pDoc),
m_StatusLevel(statusLevel)
{
}

long CDistributedLoadStatusCallback::GetSeverity()
{
   return m_StatusLevel;
}

void CDistributedLoadStatusCallback::Execute(pgsStatusItem* pStatusItem)
{

   pgsDistributedLoadStatusItem* pItem = dynamic_cast<pgsDistributedLoadStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   CDealWithLoadDlg dlg;
   dlg.m_Message = pItem->GetDescription().c_str();

   int st = dlg.DoModal();

   if (st!=IDCANCEL)
   {
      CComPtr<IBroker> pBroker;
      m_pDoc->GetBroker(&pBroker);
      GET_IFACE2(pBroker, IUserDefinedLoadData, pUdl);

      // delete
      if ( st==CDealWithLoadDlg::IDDELETELOAD )
      {
         pUdl->DeleteDistributedLoad(pItem->m_LoadIndex);

         long id = pItem->GetID();
         m_pDoc->GetStatusCenter().RemoveByID(id);
      }
      // edit
      else if (st==CDealWithLoadDlg::IDEDITLOAD)
      {
         CDistributedLoadData rld = pUdl->GetDistributedLoad(pItem->m_LoadIndex);

   	   CEditDistributedLoadDlg dlgl(rld,pBroker);
         if (dlgl.DoModal() == IDOK)
         {
            // only update if changed
            if (rld!=dlgl.m_Load)
            {
               pUdl->UpdateDistributedLoad(pItem->m_LoadIndex, dlgl.m_Load);

               long id = pItem->GetID();
               m_pDoc->GetStatusCenter().RemoveByID(id);
            }
         }
      }
      else
      {
         ATLASSERT(0);
      }
   }
}

///////////////////////////
CMomentLoadStatusCallback::CMomentLoadStatusCallback(CPGSuperDoc* pDoc, long statusLevel):
m_pDoc(pDoc),
m_StatusLevel(statusLevel)
{
}

long CMomentLoadStatusCallback::GetSeverity()
{
   return m_StatusLevel;
}

void CMomentLoadStatusCallback::Execute(pgsStatusItem* pStatusItem)
{

   pgsMomentLoadStatusItem* pItem = dynamic_cast<pgsMomentLoadStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   CDealWithLoadDlg dlg;
   dlg.m_Message = pItem->GetDescription().c_str();

   int st = dlg.DoModal();

   if (st!=IDCANCEL)
   {
      CComPtr<IBroker> pBroker;
      m_pDoc->GetBroker(&pBroker);
      GET_IFACE2(pBroker, IUserDefinedLoadData, pUdl);

      // delete
      if ( st==CDealWithLoadDlg::IDDELETELOAD )
      {
         pUdl->DeleteMomentLoad(pItem->m_LoadIndex);

         long id = pItem->GetID();
         m_pDoc->GetStatusCenter().RemoveByID(id);
      }
      // edit
      else if (st==CDealWithLoadDlg::IDEDITLOAD)
      {
         CMomentLoadData rld = pUdl->GetMomentLoad(pItem->m_LoadIndex);

   	   CEditMomentLoadDlg dlgl(rld,pBroker);
         if (dlgl.DoModal() == IDOK)
         {
            // only update if changed
            if (rld!=dlgl.m_Load)
            {
               pUdl->UpdateMomentLoad(pItem->m_LoadIndex, dlgl.m_Load);

               long id = pItem->GetID();
               m_pDoc->GetStatusCenter().RemoveByID(id);
            }
         }
      }
      else
      {
         ATLASSERT(0);
      }
   }
}

//////////////////////////////////////////////////////////
CConcreteStrengthStatusCallback::CConcreteStrengthStatusCallback(CPGSuperDoc* pDoc, long statusLevel):
m_pDoc(pDoc),
m_StatusLevel(statusLevel)
{
}

long CConcreteStrengthStatusCallback::GetSeverity()
{
   return m_StatusLevel;
}

void CConcreteStrengthStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   pgsConcreteStrengthStatusItem* pItem = dynamic_cast<pgsConcreteStrengthStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   if ( pItem->m_ConcreteType == pgsConcreteStrengthStatusItem::Slab )
   {
      m_pDoc->EditBridgeDescription(EBD_DECK);
   }
   else
   {
      m_pDoc->EditGirderDescription(pItem->m_Span,pItem->m_Girder,EGD_CONCRETE);
   }
}

//////////////////////////////////////////////////////////
CVSRatioStatusCallback::CVSRatioStatusCallback(CPGSuperDoc* pDoc):
m_pDoc(pDoc)
{
}

long CVSRatioStatusCallback::GetSeverity()
{
   return STATUS_ERROR;
}

void CVSRatioStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   pgsVSRatioStatusItem* pItem = dynamic_cast<pgsVSRatioStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   CString msg;
   msg.Format("Span %d Girder %s: %s",LABEL_SPAN(pItem->m_Span),LABEL_GIRDER(pItem->m_Girder),pItem->GetDescription().c_str());
   AfxMessageBox(msg);
}

//////////////////////////////////////////////////////////
CLiftingSupportLocationStatusCallback::CLiftingSupportLocationStatusCallback(CPGSuperDoc* pDoc):
m_pDoc(pDoc)
{
}

long CLiftingSupportLocationStatusCallback::GetSeverity()
{
   return STATUS_ERROR;
}

void CLiftingSupportLocationStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   pgsLiftingSupportLocationStatusItem* pItem = dynamic_cast<pgsLiftingSupportLocationStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   m_pDoc->EditGirderDescription(pItem->m_Span,pItem->m_Girder,EGD_TRANSPORTATION);
}

//////////////////////////////////////////////////////////
CTruckStiffnessStatusCallback::CTruckStiffnessStatusCallback(CPGSuperDoc* pDoc):
m_pDoc(pDoc)
{
}

long CTruckStiffnessStatusCallback::GetSeverity()
{
   return STATUS_ERROR;
}

void CTruckStiffnessStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   pgsTruckStiffnessStatusItem* pItem = dynamic_cast<pgsTruckStiffnessStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   CString msg;
   msg.Format("%s\n\nThe truck roll stiffness is specified in the Hauling Parameters of the Design Criteria\nDesign Criteria may be viewed in the Library Editor",pStatusItem->GetDescription().c_str());
   AfxMessageBox(msg);
}

//////////////////////////////////////////////////////////
CBridgeDescriptionStatusCallback::CBridgeDescriptionStatusCallback(CPGSuperDoc* pDoc,long severity):
m_pDoc(pDoc), m_Severity(severity)
{
}

long CBridgeDescriptionStatusCallback::GetSeverity()
{
   return m_Severity;
}

void CBridgeDescriptionStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   pgsBridgeDescriptionStatusItem* pItem = dynamic_cast<pgsBridgeDescriptionStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   m_pDoc->EditBridgeDescription(pItem->m_DlgPage);
}

//////////////////////////////////////////////////////////
CRefinedAnalysisStatusCallback::CRefinedAnalysisStatusCallback(CPGSuperDoc* pDoc):
m_pDoc(pDoc)
{
}

long CRefinedAnalysisStatusCallback::GetSeverity()
{
   return STATUS_ERROR;
}

void CRefinedAnalysisStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   pgsRefinedAnalysisStatusItem* pItem = dynamic_cast<pgsRefinedAnalysisStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   CRefinedAnalysisOptionsDlg dlg;
   dlg.m_strDescription = pStatusItem->GetDescription().c_str();
   dlg.m_strDescription.Replace("\n","\r\n"); // this makes the text wrap correctly in the dialog

   if ( dlg.DoModal() == IDOK )
   {
      // doc deals with dialog setup
      m_pDoc->OnLoadsLldf(dlg.m_Choice);
   }
}

//////////////////////////////////////////////////////////
CInstallationErrorStatusCallback::CInstallationErrorStatusCallback(CPGSuperDoc* pDoc):
m_pDoc(pDoc)
{
}

long CInstallationErrorStatusCallback::GetSeverity()
{
   return STATUS_ERROR;
}

void CInstallationErrorStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   pgsInstallationErrorStatusItem* pItem = dynamic_cast<pgsInstallationErrorStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   CString msg;
   msg.Format("PGSuper was not successfully installed or has become damanged.\n\n%s could not be created.\n\nPlease re-install the software.",pItem->m_Component.c_str());
   AfxMessageBox(msg,MB_OK | MB_ICONEXCLAMATION);
}

//////////////////////////////////////////////////////////
CUnknownErrorStatusCallback::CUnknownErrorStatusCallback(CPGSuperDoc* pDoc):
m_pDoc(pDoc)
{
}

long CUnknownErrorStatusCallback::GetSeverity()
{
   return STATUS_ERROR;
}

void CUnknownErrorStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   pgsUnknownErrorStatusItem* pItem = dynamic_cast<pgsUnknownErrorStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   CString msg;
   msg.Format("An unspecified error occured at %s, Line %d",pItem->m_File.c_str(),pItem->m_Line);
   AfxMessageBox(msg,MB_OK | MB_ICONEXCLAMATION);
}

//////////////////////////////////////////////////////////
CInformationalStatusCallback::CInformationalStatusCallback(CPGSuperDoc* pDoc,long severity,UINT helpID):
m_pDoc(pDoc), m_Severity(severity), m_HelpID(helpID)
{
}

long CInformationalStatusCallback::GetSeverity()
{
   return m_Severity;
}

void CInformationalStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   std::string msg = pStatusItem->GetDescription();

   if (m_HelpID!=0)
   {
      msg += std::string("\r\n\r\nClick on Help button for more details.");
   }

   bool is_severe = m_Severity==STATUS_ERROR;
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

//////////////////////////////////////////////////////////
CGirderDescriptionStatusCallback::CGirderDescriptionStatusCallback(CPGSuperDoc* pDoc,long severity):
m_pDoc(pDoc), m_Severity(severity)
{
}

long CGirderDescriptionStatusCallback::GetSeverity()
{
   return m_Severity;
}

void CGirderDescriptionStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   pgsGirderDescriptionStatusItem* pItem = dynamic_cast<pgsGirderDescriptionStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   CString strMessage;
   strMessage.Format("%s\n\r%s",pItem->GetDescription().c_str(),"Would you like to edit the girder?");
   int result = AfxMessageBox(strMessage,MB_YESNO);

   if ( result == IDYES )
   {
      if (m_pDoc->EditGirderDescription(pItem->m_Span,pItem->m_Girder,pItem->m_Page))
      {
         // assume that edit took care of status
         long id = pItem->GetID();
         m_pDoc->GetStatusCenter().RemoveByID(id);
      }
   }
}

//////////////////////////////////////////////////////////
CAlignmentDescriptionStatusCallback::CAlignmentDescriptionStatusCallback(CPGSuperDoc* pDoc,long severity):
m_pDoc(pDoc), m_Severity(severity)
{
}

long CAlignmentDescriptionStatusCallback::GetSeverity()
{
   return m_Severity;
}

void CAlignmentDescriptionStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   pgsAlignmentDescriptionStatusItem* pItem = dynamic_cast<pgsAlignmentDescriptionStatusItem*>(pStatusItem);
   ATLASSERT(pItem!=NULL);

   if ( AfxMessageBox( pStatusItem->GetDescription().c_str(), MB_OK ) == IDOK )
   {
      m_pDoc->EditAlignmentDescription(pItem->m_DlgPage);
   }
}

//////////////////////////////////////////////////////////
CLiveLoadStatusCallback::CLiveLoadStatusCallback(CPGSuperDoc* pDoc):
m_pDoc(pDoc)
{
}

long CLiveLoadStatusCallback::GetSeverity()
{
   return STATUS_WARNING;
}

void CLiveLoadStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   m_pDoc->OnLiveLoads();
}

//////////////////////////////////////////////////////////
CStructuralAnalysisTypeStatusCallback::CStructuralAnalysisTypeStatusCallback(CPGSuperDoc* pDoc):
m_pDoc(pDoc)
{
}

long CStructuralAnalysisTypeStatusCallback::GetSeverity()
{
   return STATUS_WARNING;
}

void CStructuralAnalysisTypeStatusCallback::Execute(pgsStatusItem* pStatusItem)
{
   AfxMessageBox(pStatusItem->GetDescription().c_str(),MB_OK);
}
