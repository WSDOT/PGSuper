///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <Reporting\SpanGirderReportSpecificationBuilder.h>
#include <Reporting\SpanGirderReportSpecification.h>
#include <Reporting\SpanGirderReportDlg.h>
#include <Reporting\MultiGirderReportDlg.h>
#include "MultiViewReportDlg.h"

#include <IFace\Selection.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>

#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CSpanReportSpecificationBuilder::CSpanReportSpecificationBuilder(IBroker* pBroker) :
CBrokerReportSpecificationBuilder(pBroker)
{
}

CSpanReportSpecificationBuilder::~CSpanReportSpecificationBuilder(void)
{
}

boost::shared_ptr<CReportSpecification> CSpanReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for span and chapter list
   GET_IFACE(ISelection,pSelection);
   SpanIndexType spanIdx = pSelection->GetSpanIdx();
   spanIdx = (spanIdx == INVALID_INDEX ? 0 : spanIdx );

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   GroupIndexType grpIdx = pGroup->GetIndex();

   CSpanGirderReportDlg dlg(m_pBroker,rptDesc,SpanAndChapters,pRptSpec); // span only mode
   dlg.m_Group = grpIdx;

   if ( dlg.DoModal() == IDOK )
   {
      boost::shared_ptr<CReportSpecification> pRptSpec( new CSpanReportSpecification(rptDesc.GetReportName(),m_pBroker,dlg.m_Group) );

      std::vector<std::_tstring> chList = dlg.m_ChapterList;
      AddChapters(rptDesc,chList,pRptSpec);

      return pRptSpec;
   }

   return boost::shared_ptr<CReportSpecification>();
}

boost::shared_ptr<CReportSpecification> CSpanReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   // Get the selected span and girder
   GET_IFACE(ISelection,pSelection);
   SpanIndexType spanIdx = pSelection->GetSpanIdx();

   ATLASSERT( spanIdx != INVALID_INDEX );

   spanIdx = (spanIdx == INVALID_INDEX ? 0 : spanIdx );
   boost::shared_ptr<CReportSpecification> pRptSpec( new CSpanReportSpecification(rptDesc.GetReportName(),m_pBroker,spanIdx) );

   AddChapters(rptDesc,pRptSpec);

   return pRptSpec;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

CGirderLineReportSpecificationBuilder::CGirderLineReportSpecificationBuilder(IBroker* pBroker) :
CBrokerReportSpecificationBuilder(pBroker)
{
}

CGirderLineReportSpecificationBuilder::~CGirderLineReportSpecificationBuilder(void)
{
}

boost::shared_ptr<CReportSpecification> CGirderLineReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for span and chapter list
   GET_IFACE(ISelection,pSelection);
   GirderIndexType gdrIdx = pSelection->GetGirderIdx();

   gdrIdx = (gdrIdx == INVALID_INDEX ? 0 : gdrIdx );

   CSpanGirderReportDlg dlg(m_pBroker,rptDesc,SpanGirderAndChapters,pRptSpec);
   dlg.m_Group = 0;
   dlg.m_Girder = gdrIdx;

   if ( dlg.DoModal() == IDOK )
   {
      boost::shared_ptr<CReportSpecification> pRptSpec( new CGirderReportSpecification(rptDesc.GetReportName(),m_pBroker,CGirderKey(dlg.m_Group,dlg.m_Girder)) );

      std::vector<std::_tstring> chList = dlg.m_ChapterList;
      AddChapters(rptDesc,chList,pRptSpec);

      return pRptSpec;
   }

   return boost::shared_ptr<CReportSpecification>();
}

boost::shared_ptr<CReportSpecification> CGirderLineReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   // Get the selected span and girder
   GET_IFACE(ISelection,pSelection);
   GirderIndexType gdrIdx = pSelection->GetGirderIdx();

   ATLASSERT( gdrIdx != INVALID_INDEX );

   gdrIdx = (gdrIdx == INVALID_INDEX ? 0 : gdrIdx );

   boost::shared_ptr<CReportSpecification> pRptSpec( new CGirderLineReportSpecification(rptDesc.GetReportName(),m_pBroker,gdrIdx) );

   AddChapters(rptDesc,pRptSpec);

   return pRptSpec;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

CGirderReportSpecificationBuilder::CGirderReportSpecificationBuilder(IBroker* pBroker,const CGirderKey& girderKey) :
CBrokerReportSpecificationBuilder(pBroker)
{
   m_GirderKey = girderKey;
}

CGirderReportSpecificationBuilder::~CGirderReportSpecificationBuilder(void)
{
}

boost::shared_ptr<CReportSpecification> CGirderReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for group, girder, and chapter list
   GET_IFACE(ISelectionEx,pSelection);
   CSelection selection = pSelection->GetSelection();
   CGirderKey girderKey(m_GirderKey);
   if ( selection.Type == CSelection::Segment )
   {
      girderKey.groupIndex   = selection.GroupIdx;
      girderKey.girderIndex  = selection.GirderIdx;
   }

   CSpanGirderReportDlg dlg(m_pBroker,rptDesc,SpanGirderAndChapters,pRptSpec);
   dlg.m_Group  = girderKey.groupIndex;
   dlg.m_Girder = girderKey.girderIndex;

   if ( dlg.DoModal() == IDOK )
   {
      girderKey.groupIndex  = dlg.m_Group;
      girderKey.girderIndex = dlg.m_Girder;
   
      boost::shared_ptr<CReportSpecification> pRptSpec( new CGirderReportSpecification(rptDesc.GetReportName(),m_pBroker,girderKey) );

      std::vector<std::_tstring> chList = dlg.m_ChapterList;
      AddChapters(rptDesc,chList,pRptSpec);

      return pRptSpec;
   }

   return boost::shared_ptr<CReportSpecification>();
}

boost::shared_ptr<CReportSpecification> CGirderReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   // Get the selected group and girder
   GET_IFACE(ISelectionEx,pSelection);
   CSelection selection = pSelection->GetSelection();
   CGirderKey girderKey(m_GirderKey);
   if ( selection.Type == CSelection::Segment )
   {
      girderKey.groupIndex   = selection.GroupIdx;
      girderKey.girderIndex  = selection.GirderIdx;
   }
   boost::shared_ptr<CReportSpecification> pRptSpec( new CGirderReportSpecification(rptDesc.GetReportName(),m_pBroker,girderKey) );

   AddChapters(rptDesc,pRptSpec);

   return pRptSpec;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

CMultiGirderReportSpecificationBuilder::CMultiGirderReportSpecificationBuilder(IBroker* pBroker) :
CBrokerReportSpecificationBuilder(pBroker)
{
}

CMultiGirderReportSpecificationBuilder::~CMultiGirderReportSpecificationBuilder(void)
{
}

boost::shared_ptr<CReportSpecification> CMultiGirderReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for span, girder, and chapter list
   GET_IFACE(ISelection,pSelection);
   SpanIndexType spanIdx = pSelection->GetSpanIdx();
   GirderIndexType gdrIdx = pSelection->GetGirderIdx();

   spanIdx = (spanIdx == INVALID_INDEX ? 0 : spanIdx );
   gdrIdx  = (gdrIdx  == INVALID_INDEX ? 0 : gdrIdx  );

   CMultiGirderReportDlg dlg(m_pBroker,rptDesc,pRptSpec);

   GET_IFACE(IBridge,pBridge);
   GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanIdx);

   CGirderKey girderKey(grpIdx,gdrIdx);
   std::vector<CGirderKey> girderKeys;
   girderKeys.push_back(girderKey);

   dlg.m_GirderKeys = girderKeys;

   if ( dlg.DoModal() == IDOK )
   {
      boost::shared_ptr<CReportSpecification> pRptSpec( new CMultiGirderReportSpecification(rptDesc.GetReportName(),m_pBroker, dlg.m_GirderKeys) );

      std::vector<std::_tstring> chList = dlg.m_ChapterList;
      AddChapters(rptDesc,chList,pRptSpec);

      return pRptSpec;
   }

   return boost::shared_ptr<CReportSpecification>();
}

boost::shared_ptr<CReportSpecification> CMultiGirderReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   // Get the selected span and girder
   GET_IFACE(ISelection,pSelection);
   SpanIndexType spanIdx = pSelection->GetSpanIdx();
   GirderIndexType gdrIdx = pSelection->GetGirderIdx();

   spanIdx = (spanIdx == INVALID_INDEX ? 0 : spanIdx );
   gdrIdx  = (gdrIdx  == INVALID_INDEX ? 0 : gdrIdx  );

   GET_IFACE(IBridge,pBridge);
   GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanIdx);

   CGirderKey girderKey(grpIdx,gdrIdx);
   std::vector<CGirderKey> girderKeys;
   girderKeys.push_back(girderKey);

   boost::shared_ptr<CReportSpecification> pRptSpec( new CMultiGirderReportSpecification(rptDesc.GetReportName(),m_pBroker, girderKeys) );

   AddChapters(rptDesc,pRptSpec);

   return pRptSpec;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

CMultiViewSpanGirderReportSpecificationBuilder::CMultiViewSpanGirderReportSpecificationBuilder(IBroker* pBroker) :
CSpanReportSpecificationBuilder(pBroker)
{
}

CMultiViewSpanGirderReportSpecificationBuilder::~CMultiViewSpanGirderReportSpecificationBuilder(void)
{
}

boost::shared_ptr<CReportSpecification> CMultiViewSpanGirderReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec)
{
   // First check if we are getting a CGirderReportSpecification. If so, use our bro to take care of this
   boost::shared_ptr<CGirderReportSpecification> pGirderRptSpec = boost::dynamic_pointer_cast<CGirderReportSpecification,CReportSpecification>(pRptSpec);
   if ( pGirderRptSpec != NULL )
   {
      boost::shared_ptr<CReportSpecificationBuilder> pGirderRptSpecBuilder( new CGirderReportSpecificationBuilder(m_pBroker,pGirderRptSpec->GetGirderKey()) );
      return pGirderRptSpecBuilder->CreateReportSpec(rptDesc, pRptSpec);
   }
   else
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      // Prompt for span, girder, and chapter list
      GET_IFACE(ISelection,pSelection);
      SpanIndexType spanIdx = pSelection->GetSpanIdx();
      GirderIndexType gdrIdx = pSelection->GetGirderIdx();

      spanIdx = (spanIdx == INVALID_INDEX ? 0 : spanIdx );
      gdrIdx  = (gdrIdx  == INVALID_INDEX ? 0 : gdrIdx  );

      GET_IFACE(IBridge,pBridge);
      GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanIdx);
      CGirderKey girderKey(grpIdx,gdrIdx);

      CMultiViewReportDlg dlg(m_pBroker,rptDesc,pRptSpec,girderKey);

      if ( dlg.DoModal() == IDOK )
      {
         std::vector<CGirderKey> girderKeys = dlg.GetGirderKeys();

         boost::shared_ptr<CReportSpecification> pRptSpec( new CMultiViewSpanGirderReportSpecification(rptDesc.GetReportName(),m_pBroker,girderKeys) );

         std::vector<std::_tstring> chList = dlg.m_ChapterList;
         AddChapters(rptDesc,chList,pRptSpec);

         return pRptSpec;
      }

      return boost::shared_ptr<CReportSpecification>();
   }
}

boost::shared_ptr<CReportSpecification> CMultiViewSpanGirderReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   // Get the selected span and girder
   GET_IFACE(ISelection,pSelection);
   SpanIndexType spanIdx = pSelection->GetSpanIdx();
   GirderIndexType gdrIdx = pSelection->GetGirderIdx();

   spanIdx = (spanIdx == INVALID_INDEX ? 0 : spanIdx );
   gdrIdx  = (gdrIdx  == INVALID_INDEX ? 0 : gdrIdx  );

   GET_IFACE(IBridge,pBridge);
   GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanIdx);

   CGirderKey girderKey(grpIdx,gdrIdx);
   boost::shared_ptr<CReportSpecification> pRptSpec( new CGirderReportSpecification(rptDesc.GetReportName(),m_pBroker,girderKey) );

   AddChapters(rptDesc,pRptSpec);

   return pRptSpec;
}

