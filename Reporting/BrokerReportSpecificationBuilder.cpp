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

#include "stdafx.h"
#include <Reporting\BrokerReportSpecificationBuilder.h>
#include <Reporting\BrokerReportSpecification.h>
#include <Reporting\SpanGirderReportDlg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CBrokerReportSpecificationBuilder::CBrokerReportSpecificationBuilder(IBroker* pBroker)
{
   m_pBroker = pBroker;
}

CBrokerReportSpecificationBuilder::~CBrokerReportSpecificationBuilder(void)
{
}

boost::shared_ptr<CReportSpecification> CBrokerReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pOldRptSpec)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CSpanGirderReportDlg dlg(m_pBroker,rptDesc,ChaptersOnly,pOldRptSpec);

   if ( dlg.DoModal() == IDOK )
   {
      // If possible, copy information from old spec. Otherwise header/footer and other info will be lost
      boost::shared_ptr<CBrokerReportSpecification> pOldGRptSpec = boost::dynamic_pointer_cast<CBrokerReportSpecification>(pOldRptSpec);

      boost::shared_ptr<CReportSpecification> pNewRptSpec;
      if(pOldGRptSpec)
      {
         boost::shared_ptr<CBrokerReportSpecification> pNewGRptSpec = boost::shared_ptr<CBrokerReportSpecification>( new CBrokerReportSpecification(*pOldGRptSpec) );

         pNewRptSpec = boost::static_pointer_cast<CReportSpecification>(pNewGRptSpec);
      }
      else
      {
         pNewRptSpec = boost::shared_ptr<CBrokerReportSpecification>( new CBrokerReportSpecification(rptDesc.GetReportName(),m_pBroker) );
      }

      std::vector<std::_tstring> chList = dlg.m_ChapterList;
      rptDesc.ConfigureReportSpecification(chList,pNewRptSpec);

      return pNewRptSpec;
   }

   return boost::shared_ptr<CReportSpecification>();
}

boost::shared_ptr<CReportSpecification> CBrokerReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   // Use all chapters at the maximum level
   boost::shared_ptr<CReportSpecification> pRptSpec( new CBrokerReportSpecification(rptDesc.GetReportName(),m_pBroker) );

   rptDesc.ConfigureReportSpecification(pRptSpec);

   return pRptSpec;
}