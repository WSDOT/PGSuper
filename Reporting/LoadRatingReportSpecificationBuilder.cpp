///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#include <IFace\RatingSpecification.h>
#include <IFace\Selection.h>
#include <IFace\Bridge.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CLoadRatingReportSpecificationBuilder::CLoadRatingReportSpecificationBuilder(IBroker* pBroker) :
   CGirderLineReportSpecificationBuilder(pBroker)
{
}

CLoadRatingReportSpecificationBuilder::~CLoadRatingReportSpecificationBuilder(void)
{
}

std::shared_ptr<CReportSpecification> CLoadRatingReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,std::shared_ptr<CReportSpecification>& pOldRptSpec)
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

      CLoadRatingReportDlg dlg(m_pBroker, rptDesc, pOldRptSpec);
      dlg.m_Girder = girderKey.girderIndex;

      if (dlg.DoModal() == IDOK)
      {
         // If possible, copy information from old spec. Otherwise header/footer and other info will be lost
         std::shared_ptr<CLoadRatingReportSpecification> pOldGRptSpec(std::dynamic_pointer_cast<CLoadRatingReportSpecification>(pOldRptSpec));

         std::shared_ptr<CReportSpecification> pNewRptSpec;
         if (pOldGRptSpec)
         {
            std::shared_ptr<CLoadRatingReportSpecification> pNewGRptSpec(std::make_shared<CLoadRatingReportSpecification>(*pOldGRptSpec));

            pNewGRptSpec->SetGirderIndex(dlg.m_Girder);
            pNewGRptSpec->ReportAtAllPointsOfInterest(dlg.m_bReportAtAllPoi);

            pNewRptSpec = std::static_pointer_cast<CReportSpecification>(pNewGRptSpec);
         }
         else
         {
            pNewRptSpec = std::make_shared<CLoadRatingReportSpecification>(rptDesc.GetReportName(), m_pBroker, dlg.m_Girder, dlg.m_bReportAtAllPoi);
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

std::shared_ptr<CReportSpecification> CLoadRatingReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
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
      // Get the selected span and girder
      GET_IFACE(ISelection, pSelection);
      CGirderKey girderKey = pSelection->GetSelectedGirder();
      girderKey.groupIndex = (girderKey.groupIndex == INVALID_INDEX ? 0 : girderKey.groupIndex);
      girderKey.girderIndex = (girderKey.girderIndex == INVALID_INDEX ? 0 : girderKey.girderIndex);

      std::shared_ptr<CReportSpecification> pRptSpec(std::make_shared<CLoadRatingReportSpecification>(rptDesc.GetReportName(), m_pBroker, girderKey.girderIndex,false/*quick reports... dont report at all poi*/));

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
//////////////////////////////////////////////////

CLoadRatingReportSpecification::CLoadRatingReportSpecification(LPCTSTR strReportName, IBroker* pBroker, GirderIndexType gdrIdx, bool bReportForAllPoi) :
   CGirderLineReportSpecification(strReportName, pBroker, gdrIdx)
{
   m_bReportAtAllPoi = bReportForAllPoi;
}

CLoadRatingReportSpecification::CLoadRatingReportSpecification(const CLoadRatingReportSpecification& other) :
   CGirderLineReportSpecification(other)
{
   m_bReportAtAllPoi = other.m_bReportAtAllPoi;
}

CLoadRatingReportSpecification::~CLoadRatingReportSpecification(void)
{
}

void CLoadRatingReportSpecification::ReportAtAllPointsOfInterest(bool bReportAtAllPoi)
{
   m_bReportAtAllPoi = bReportAtAllPoi;
}

bool CLoadRatingReportSpecification::ReportAtAllPointsOfInterest() const
{
   return m_bReportAtAllPoi;
}
