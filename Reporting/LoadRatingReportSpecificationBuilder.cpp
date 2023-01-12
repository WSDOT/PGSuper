///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#include "stdafx.h"
#include <Reporting\LoadRatingReportSpecificationBuilder.h>
#include <Reporting\LoadRatingReportDlg.h>
#include <Reporting\LoadRatingSummaryReportDlg.h>

#include <IFace\RatingSpecification.h>
#include <IFace\Selection.h>
#include <IFace\Bridge.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CLoadRatingReportSpecificationBuilder::CLoadRatingReportSpecificationBuilder(IBroker* pBroker) :
   CBrokerReportSpecificationBuilder(pBroker)
{
}

CLoadRatingReportSpecificationBuilder::~CLoadRatingReportSpecificationBuilder(void)
{
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CLoadRatingReportSpecificationBuilder::CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification>& pOldRptSpec) const
{
   GET_IFACE(IRatingSpecification,pRatingSpec);
   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special)
      )
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());

      // Prompt for gider and chapter list
      GET_IFACE(ISelection, pSelection);
      CGirderKey girderKey = pSelection->GetSelectedGirder();
      girderKey.groupIndex = (girderKey.groupIndex == INVALID_INDEX ? ALL_GROUPS : girderKey.groupIndex);
      girderKey.girderIndex = (girderKey.girderIndex == INVALID_INDEX ? 0 : girderKey.girderIndex);

      CLoadRatingReportDlg dlg(m_pBroker, rptDesc, pOldRptSpec);
      dlg.SetGirderKey(girderKey);

      if (dlg.DoModal() == IDOK)
      {
         girderKey = dlg.GetGirderKey();

         std::shared_ptr<WBFL::Reporting::ReportSpecification> pNewRptSpec;
         if (dlg.IsSingleGirderLineSelected())
         {
            // girderline
            pNewRptSpec = std::make_shared<CGirderLineLoadRatingReportSpecification>(rptDesc.GetReportName(), m_pBroker, girderKey.girderIndex, dlg.m_bReportAtAllPoi);
         }
         else
         {
            // single girder
            pNewRptSpec = std::make_shared<CGirderLoadRatingReportSpecification>(rptDesc.GetReportName(), m_pBroker, girderKey, dlg.m_bReportAtAllPoi);
         }

         // If possible, copy information from old spec. Otherwise header/footer and other info will be lost
         if (pOldRptSpec)
         {
            *pNewRptSpec = *pOldRptSpec;
         }

         std::vector<std::_tstring> chList = dlg.m_ChapterList;
         rptDesc.ConfigureReportSpecification(chList, pNewRptSpec);

         return pNewRptSpec;
      }

      return nullptr;
   }
   else
   {
      AfxMessageBox(_T("No rating types defined. Select Project | Load Rating Options to select rating types"));
      return nullptr;
   }
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CLoadRatingReportSpecificationBuilder::CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const
{
   GET_IFACE(IRatingSpecification,pRatingSpec);
   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special)
      )
   {
      // Get the selected span and girder. By default we rate a entire selected girderline
      GET_IFACE(ISelection, pSelection);
      CGirderKey girderKey = pSelection->GetSelectedGirder();
      GirderIndexType girderIndex = (girderKey.girderIndex == INVALID_INDEX ? 0 : girderKey.girderIndex);

      std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec(std::make_shared<CGirderLineLoadRatingReportSpecification>(rptDesc.GetReportName(), m_pBroker, girderIndex, false/*quick reports... dont report at all poi*/));

      rptDesc.ConfigureReportSpecification(pRptSpec);

      return pRptSpec;
   }
   else
   {
      AfxMessageBox(_T("No rating types defined. Select Project | Load Rating Options to select rating types"));
      return nullptr;
   }
}

//////////////////////////////// CLoadRatingSummaryReportSpecificationBuilder ////////////////////

CLoadRatingSummaryReportSpecificationBuilder::CLoadRatingSummaryReportSpecificationBuilder(IBroker* pBroker) :
   CBrokerReportSpecificationBuilder(pBroker)
{
}

CLoadRatingSummaryReportSpecificationBuilder::~CLoadRatingSummaryReportSpecificationBuilder(void)
{
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CLoadRatingSummaryReportSpecificationBuilder::CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification>& pOldRptSpec) const
{
   GET_IFACE(IRatingSpecification,pRatingSpec);
   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special)
      )
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());

      // Prompt for span and chapter list
      GET_IFACE(ISelection, pSelection);
      CGirderKey girderKey = pSelection->GetSelectedGirder();
      girderKey.groupIndex = (girderKey.groupIndex == INVALID_INDEX ? 0 : girderKey.groupIndex);
      girderKey.girderIndex = (girderKey.girderIndex == INVALID_INDEX ? 0 : girderKey.girderIndex);

      CLoadRatingSummaryReportDlg dlg(m_pBroker, rptDesc, pOldRptSpec);
      dlg.m_GirderKeys.push_back(girderKey);

      if (dlg.DoModal() == IDOK)
      {
         std::shared_ptr<WBFL::Reporting::ReportSpecification> pNewRptSpec;
         if (dlg.m_bIsSingleGirderLineSelected)
         {
            pNewRptSpec = std::make_shared<CGirderLineLoadRatingReportSpecification>(rptDesc.GetReportName(), m_pBroker, dlg.m_GirderKeys.front().girderIndex, dlg.m_bReportAtAllPoi);
         }
         else
         {
            pNewRptSpec = std::make_shared<CMultiGirderLoadRatingReportSpecification>(rptDesc.GetReportName(), m_pBroker, dlg.m_GirderKeys, dlg.m_bReportAtAllPoi);
         }

         // If possible, copy information from old spec. Otherwise header/footer and other info will be lost
         if (pOldRptSpec)
         {
            *pNewRptSpec = *pOldRptSpec;
         }

         std::vector<std::_tstring> chList = dlg.m_ChapterList;
         rptDesc.ConfigureReportSpecification(chList, pNewRptSpec);

         return pNewRptSpec;
      }

      return nullptr;
   }
   else
   {
      AfxMessageBox(_T("No rating types defined. Select Project | Load Rating Options to select rating types"));
      return nullptr;
   }
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CLoadRatingSummaryReportSpecificationBuilder::CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const
{
   GET_IFACE(IRatingSpecification,pRatingSpec);
   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) ||
      pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special)
      )
   {
      // Get the selected span and girder. By default we rate a entire selected girderline
      GET_IFACE(ISelection, pSelection);
      CGirderKey girderKey = pSelection->GetSelectedGirder();
      GirderIndexType girderIndex = (girderKey.girderIndex == INVALID_INDEX ? 0 : girderKey.girderIndex);

      std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec(std::make_shared<CGirderLineLoadRatingReportSpecification>(rptDesc.GetReportName(), m_pBroker, girderIndex, false/*quick reports... dont report at all poi*/));

      rptDesc.ConfigureReportSpecification(pRptSpec);

      return pRptSpec;
   }
   else
   {
      AfxMessageBox(_T("No rating types defined. Select Project | Load Rating Options to select rating types"));
      return nullptr;
   }
}

//////////////////////////////////////////////////
//////////////////////////////////////////////////
CLoadRatingReportSpecificationBase::CLoadRatingReportSpecificationBase(bool bReportForAllPoi)
{
   m_bReportAtAllPoi = bReportForAllPoi;
}

void CLoadRatingReportSpecificationBase::ReportAtAllPointsOfInterest(bool bReportAtAllPoi)
{
   m_bReportAtAllPoi = bReportAtAllPoi;
}

bool CLoadRatingReportSpecificationBase::ReportAtAllPointsOfInterest() const
{
   return m_bReportAtAllPoi;
}

////////////////////  CGirderLoadRatingReportSpecification //////////////////////////////

CGirderLoadRatingReportSpecification::CGirderLoadRatingReportSpecification(const std::_tstring& strReportName, IBroker* pBroker, const CGirderKey& gdrKey, bool bReportForAllPoi) :
   CGirderReportSpecification(strReportName, pBroker, gdrKey), CLoadRatingReportSpecificationBase(bReportForAllPoi)
{
}

CGirderLoadRatingReportSpecification::~CGirderLoadRatingReportSpecification(void)
{
}

std::vector<CGirderKey> CGirderLoadRatingReportSpecification::GetGirderKeys() const
{
   return std::vector<CGirderKey> { m_GirderKey };
}

////////////////////  CGirderLineLoadRatingReportSpecification //////////////////////////////

CGirderLineLoadRatingReportSpecification::CGirderLineLoadRatingReportSpecification(const std::_tstring& strReportName, IBroker* pBroker, GirderIndexType gdrIdx, bool bReportForAllPoi) :
   CGirderLineReportSpecification(strReportName, pBroker, gdrIdx), CLoadRatingReportSpecificationBase(bReportForAllPoi)
{
}

CGirderLineLoadRatingReportSpecification::~CGirderLineLoadRatingReportSpecification(void)
{
}

std::vector<CGirderKey> CGirderLineLoadRatingReportSpecification::GetGirderKeys() const
{
   return std::vector<CGirderKey> { CGirderKey(ALL_GROUPS, m_GirderIdx) };
}

//////////////////////////   CMultiGirderLoadRatingReportSpecification  ////////////////////////

CMultiGirderLoadRatingReportSpecification::CMultiGirderLoadRatingReportSpecification(const std::_tstring& strReportName, IBroker* pBroker, const std::vector<CGirderKey>& gdrKeys, bool bReportForAllPoi) :
   CMultiGirderReportSpecification(strReportName, pBroker, gdrKeys), CLoadRatingReportSpecificationBase(bReportForAllPoi)
{
}

CMultiGirderLoadRatingReportSpecification::~CMultiGirderLoadRatingReportSpecification(void)
{
}

bool CMultiGirderLoadRatingReportSpecification::IsSingleGirderLineReport() const
{
   return m_GirderKeys.size() == 1 && m_GirderKeys.front().groupIndex == ALL_GROUPS;
}

std::vector<CGirderKey> CMultiGirderLoadRatingReportSpecification::GetGirderKeys() const
{
   return CMultiGirderReportSpecification::GetGirderKeys();
}

bool CMultiGirderLoadRatingReportSpecification::IsValid() const
{
   if (IsSingleGirderLineReport())
   {
      // we are ok. this is a girderline description
   }
   else
   {
      GET_IFACE(IBridge,pBridge);
      GroupIndexType nGroups = pBridge->GetGirderGroupCount();
      for (const auto& girderKey : m_GirderKeys)
      {
         if (nGroups <= girderKey.groupIndex)
         {
            return false;
         }

         GirderIndexType nGirders = pBridge->GetGirderCount(girderKey.groupIndex);
         if (nGirders <= girderKey.girderIndex)
         {
            return false;
         }
      }
   }

   return CBrokerReportSpecification::IsValid();
}

std::_tstring CMultiGirderLoadRatingReportSpecification::GetReportContextString() const
{
   if (IsSingleGirderLineReport())
   {
      CString msg;
      msg.Format(_T("GirderLine %s"), LABEL_GIRDER(m_GirderKeys.front().girderIndex));
      return std::_tstring(msg);
   }
   else
   {
      return CMultiGirderReportSpecification::GetReportContextString();
   }
}
