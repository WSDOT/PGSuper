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

boost::shared_ptr<CReportSpecification> CBrokerReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CSpanGirderReportDlg dlg(m_pBroker,rptDesc,ChaptersOnly,pRptSpec);

   if ( dlg.DoModal() == IDOK )
   {
      boost::shared_ptr<CReportSpecification> pRptSpec( new CBrokerReportSpecification(rptDesc.GetReportName(),m_pBroker) );

      std::vector<std::_tstring> chList = dlg.m_ChapterList;
      AddChapters(rptDesc,chList,pRptSpec);

      return pRptSpec;
   }

   return boost::shared_ptr<CReportSpecification>();
}

boost::shared_ptr<CReportSpecification> CBrokerReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   // Use all chapters at the maximum level
   boost::shared_ptr<CReportSpecification> pRptSpec( new CBrokerReportSpecification(rptDesc.GetReportName(),m_pBroker) );

   AddChapters(rptDesc,pRptSpec);

   return pRptSpec;
}

void CBrokerReportSpecificationBuilder::AddChapters(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec)
{
   std::vector<CChapterInfo> vChInfo = rptDesc.GetChapterInfo();
   std::vector<CChapterInfo>::const_iterator iter;
   for ( iter = vChInfo.begin(); iter != vChInfo.end(); iter++ )
   {
      CChapterInfo chInfo = *iter;
      if (chInfo.Select)
         pRptSpec->AddChapter(chInfo.Name.c_str(),chInfo.Key.c_str(),chInfo.MaxLevel);
   }
}


void CBrokerReportSpecificationBuilder::AddChapters(const CReportDescription& rptDesc,const std::vector<std::_tstring>& chList,boost::shared_ptr<CReportSpecification>& pRptSpec)
{
   std::vector<CChapterInfo> vChInfo = rptDesc.GetChapterInfo();

   std::vector<std::_tstring>::const_iterator iter;
   for ( iter = chList.begin(); iter != chList.end(); iter++ )
   {
      CChapterInfo search;
      search.Name = *iter;

      std::vector<CChapterInfo>::iterator found = std::find(vChInfo.begin(),vChInfo.end(),search);
      ATLASSERT( found != vChInfo.end() ); // if this fires, the supplied chapter list isn't consistent with the report description
      CChapterInfo chInfo = *found;
      ATLASSERT( chInfo.Name == *iter);
      pRptSpec->AddChapter(chInfo.Name.c_str(),chInfo.Key.c_str(),chInfo.MaxLevel);
   }
}
