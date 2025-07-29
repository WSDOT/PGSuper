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

#include "stdafx.h"
#include <Reporting\SpanGirderReportSpecificationBuilder.h>
#include <Reporting\SpanGirderReportSpecification.h>
#include <Reporting\SpanGirderReportDlg.h>
#include <Reporting\MultiGirderReportDlg.h>
#include "GirderMultiViewReportDlg.h"
#include "SelectPointOfInterestDlg.h"

#include <IFace/Tools.h>
#include <EAF/EAFDisplayUnits.h>
#include <IFace\Selection.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace/PointOfInterest.h>

#include <PsgLib\BridgeDescription2.h>


CSpanReportSpecificationBuilder::CSpanReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker) :
CBrokerReportSpecificationBuilder(pBroker)
{
}

CSpanReportSpecificationBuilder::~CSpanReportSpecificationBuilder(void)
{
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CSpanReportSpecificationBuilder::CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification> pOldRptSpec) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for span and chapter list
   GET_IFACE2(GetBroker(),ISelection,pSelection);
   SpanIndexType spanIdx = pSelection->GetSelectedSpan();
   spanIdx = (spanIdx == INVALID_INDEX ? 0 : spanIdx );

   GET_IFACE2(GetBroker(),IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   GroupIndexType grpIdx = pGroup->GetIndex();

   CSpanGirderReportDlg dlg(GetBroker(), rptDesc, CSpanGirderReportDlg::Mode::GroupAndChapters, pOldRptSpec); // span only mode
   dlg.m_SegmentKey.groupIndex = grpIdx;

   if ( dlg.DoModal() == IDOK )
   {
      // If possible, copy information from old spec. Otherwise header/footer and other info will be lost
      std::shared_ptr<CSpanReportSpecification> pOldGRptSpec( std::dynamic_pointer_cast<CSpanReportSpecification>(pOldRptSpec) );

      std::shared_ptr<WBFL::Reporting::ReportSpecification> pNewRptSpec;
      if(pOldGRptSpec)
      {
         std::shared_ptr<CSpanReportSpecification> pNewGRptSpec(std::make_shared<CSpanReportSpecification>(*pOldGRptSpec) );

         pNewGRptSpec->SetSpan(dlg.m_SegmentKey.groupIndex);

         pNewRptSpec = std::static_pointer_cast<WBFL::Reporting::ReportSpecification>(pNewGRptSpec);
      }
      else
      {
         pNewRptSpec = std::make_shared<CSpanReportSpecification>(rptDesc.GetReportName(),m_pBroker,dlg.m_SegmentKey.groupIndex);
      }

      std::vector<std::_tstring> chList = dlg.m_ChapterList;
      rptDesc.ConfigureReportSpecification(chList,pNewRptSpec);

      return pNewRptSpec;
   }

   return nullptr;
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CSpanReportSpecificationBuilder::CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const
{
   // Get the selected span and girder
   GET_IFACE2(GetBroker(),ISelection,pSelection);
   SpanIndexType spanIdx = pSelection->GetSelectedSpan();

   spanIdx = (spanIdx == INVALID_INDEX ? 0 : spanIdx );
   std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec( std::make_shared<CSpanReportSpecification>(rptDesc.GetReportName(),m_pBroker,spanIdx) );

   rptDesc.ConfigureReportSpecification(pRptSpec);

   return pRptSpec;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

CGirderLineReportSpecificationBuilder::CGirderLineReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker) :
CBrokerReportSpecificationBuilder(pBroker)
{
}

CGirderLineReportSpecificationBuilder::~CGirderLineReportSpecificationBuilder(void)
{
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CGirderLineReportSpecificationBuilder::CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification> pOldRptSpec) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for span and chapter list
   GET_IFACE2(GetBroker(),ISelection,pSelection);
   CGirderKey girderKey = pSelection->GetSelectedGirder();
   girderKey.groupIndex  = (girderKey.groupIndex  == INVALID_INDEX ? 0 : girderKey.groupIndex);
   girderKey.girderIndex = (girderKey.girderIndex == INVALID_INDEX ? 0 : girderKey.girderIndex);

   CSpanGirderReportDlg dlg(GetBroker(), rptDesc, CSpanGirderReportDlg::Mode::GirderAndChapters, pOldRptSpec);
   dlg.m_SegmentKey = CSegmentKey(girderKey, INVALID_INDEX);

   if ( dlg.DoModal() == IDOK )
   {
      // If possible, copy information from old spec. Otherwise header/footer and other info will be lost
      std::shared_ptr<CGirderLineReportSpecification> pOldGRptSpec( std::dynamic_pointer_cast<CGirderLineReportSpecification>(pOldRptSpec) );

      std::shared_ptr<WBFL::Reporting::ReportSpecification> pNewRptSpec;
      if(pOldGRptSpec)
      {
         std::shared_ptr<CGirderLineReportSpecification> pNewGRptSpec(std::make_shared<CGirderLineReportSpecification>(*pOldGRptSpec) );

         pNewGRptSpec->SetGirderIndex(dlg.m_SegmentKey.girderIndex);

         pNewRptSpec = std::static_pointer_cast<WBFL::Reporting::ReportSpecification>(pNewGRptSpec);
      }
      else
      {
         pNewRptSpec = std::make_shared<CGirderLineReportSpecification>(rptDesc.GetReportName(),m_pBroker,dlg.m_SegmentKey.girderIndex);
      }

      std::vector<std::_tstring> chList = dlg.m_ChapterList;
      rptDesc.ConfigureReportSpecification(chList,pNewRptSpec);

      return pNewRptSpec;
   }

   return nullptr;
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CGirderLineReportSpecificationBuilder::CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const
{
   // Get the selected span and girder
   GET_IFACE2(GetBroker(),ISelection,pSelection);
   CGirderKey girderKey = pSelection->GetSelectedGirder();
   girderKey.groupIndex  = (girderKey.groupIndex  == INVALID_INDEX ? 0 : girderKey.groupIndex);
   girderKey.girderIndex = (girderKey.girderIndex == INVALID_INDEX ? 0 : girderKey.girderIndex);

   std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec(std::make_shared<CGirderLineReportSpecification>(rptDesc.GetReportName(),m_pBroker,girderKey.girderIndex) );

   rptDesc.ConfigureReportSpecification(pRptSpec);

   return pRptSpec;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

CGirderReportSpecificationBuilder::CGirderReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker,const CGirderKey& girderKey) :
CBrokerReportSpecificationBuilder(pBroker)
{
   m_GirderKey = girderKey;
}

CGirderReportSpecificationBuilder::~CGirderReportSpecificationBuilder(void)
{
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CGirderReportSpecificationBuilder::CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification> pOldRptSpec) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for group, girder, and chapter list
   GET_IFACE2(GetBroker(),ISelection,pSelection);
   CSelection selection = pSelection->GetSelection();
   CGirderKey girderKey(m_GirderKey);
   if ( selection.Type == CSelection::Segment || selection.Type == CSelection::Girder )
   {
      girderKey.groupIndex   = selection.GroupIdx;
      girderKey.girderIndex  = selection.GirderIdx;
   }

   CSpanGirderReportDlg dlg(GetBroker(), rptDesc, CSpanGirderReportDlg::Mode::GroupGirderAndChapters, pOldRptSpec);
   dlg.m_SegmentKey = CSegmentKey(girderKey, ALL_SEGMENTS);

   if ( dlg.DoModal() == IDOK )
   {
      girderKey = dlg.m_SegmentKey;

      // If possible, copy information from old spec. Otherwise header/footer and other info will be lost
      std::shared_ptr<CGirderReportSpecification> pOldGRptSpec = std::dynamic_pointer_cast<CGirderReportSpecification>(pOldRptSpec);

      std::shared_ptr<WBFL::Reporting::ReportSpecification> pNewRptSpec;
      if(pOldGRptSpec)
      {
         std::shared_ptr<CGirderReportSpecification> pNewGRptSpec( std::make_shared<CGirderReportSpecification>(*pOldGRptSpec));
         pNewGRptSpec->SetGirderKey(girderKey);

         pNewRptSpec = std::static_pointer_cast<WBFL::Reporting::ReportSpecification>(pNewGRptSpec);
      }
      else
      {
         pNewRptSpec = std::make_shared<CGirderReportSpecification>(rptDesc.GetReportName(),m_pBroker,girderKey);
      }

      std::vector<std::_tstring> chList = dlg.m_ChapterList;
      rptDesc.ConfigureReportSpecification(chList,pNewRptSpec);

      return pNewRptSpec;
   }

   return nullptr;
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CGirderReportSpecificationBuilder::CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const
{
   // Get the selected group and girder
   GET_IFACE2(GetBroker(),ISelection,pSelection);
   CSelection selection = pSelection->GetSelection();
   CGirderKey girderKey(m_GirderKey);
   if ( selection.Type == CSelection::Girder || selection.Type == CSelection::Segment )
   {
      GET_IFACE2(GetBroker(),IBridge,pBridge);
      GroupIndexType nGroups = pBridge->GetGirderGroupCount();
      if ( selection.GroupIdx == ALL_GROUPS && selection.GirderIdx != ALL_GIRDERS && nGroups == 1 )
      {
         // there is exactly one group and we have a valid girder number so take ALL_GROUPS to mean group 0... no prompting required
         girderKey.groupIndex = 0;
         girderKey.girderIndex = selection.GirderIdx;
      }
      else if ( selection.GroupIdx == ALL_GROUPS || selection.GirderIdx == ALL_GIRDERS )
      {
         // we need a specific girder and don't have it.... going to have to prompt the user

         // lets try to get the girder key close
         if ( selection.GroupIdx != ALL_GROUPS )
         {
            m_GirderKey.groupIndex = selection.GroupIdx;
         }

         if ( selection.GirderIdx != ALL_GIRDERS )
         {
            m_GirderKey.girderIndex = selection.GirderIdx;
         }

         std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec = CreateReportSpec(rptDesc, std::shared_ptr<WBFL::Reporting::ReportSpecification>());

         // put the girder key back the way it was
         m_GirderKey = girderKey;

         return pRptSpec;
      }
      else
      {
         girderKey.groupIndex   = selection.GroupIdx;
         girderKey.girderIndex  = selection.GirderIdx;
      }
   }
   std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec(std::make_shared<CGirderReportSpecification>(rptDesc.GetReportName(),m_pBroker,girderKey) );

   rptDesc.ConfigureReportSpecification(pRptSpec);


   return pRptSpec;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

CSegmentReportSpecificationBuilder::CSegmentReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker, const CSegmentKey& segmentKey) :
   CBrokerReportSpecificationBuilder(pBroker)
{
   m_SegmentKey = segmentKey;
}

CSegmentReportSpecificationBuilder::~CSegmentReportSpecificationBuilder(void)
{
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CSegmentReportSpecificationBuilder::CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc, std::shared_ptr<WBFL::Reporting::ReportSpecification> pOldRptSpec) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for group, girder, and chapter list
   GET_IFACE2(GetBroker(),ISelection, pSelection);
   CSelection selection = pSelection->GetSelection();
   CSegmentKey segmentKey(m_SegmentKey);
   if (selection.Type == CSelection::Segment || selection.Type == CSelection::Girder)
   {
      segmentKey.groupIndex = selection.GroupIdx;
      segmentKey.girderIndex = selection.GirderIdx;
      segmentKey.segmentIndex = selection.SegmentIdx;
   }

   CSpanGirderReportDlg dlg(GetBroker(), rptDesc, CSpanGirderReportDlg::Mode::GroupGirderSegmentAndChapters, pOldRptSpec);
   dlg.m_SegmentKey = segmentKey;

   if (dlg.DoModal() == IDOK)
   {
      segmentKey = dlg.m_SegmentKey;

      // If possible, copy information from old spec. Otherwise header/footer and other info will be lost
      std::shared_ptr<CSegmentReportSpecification> pOldGRptSpec = std::dynamic_pointer_cast<CSegmentReportSpecification>(pOldRptSpec);

      std::shared_ptr<WBFL::Reporting::ReportSpecification> pNewRptSpec;
      if (pOldGRptSpec)
      {
         std::shared_ptr<CSegmentReportSpecification> pNewGRptSpec(std::make_shared<CSegmentReportSpecification>(*pOldGRptSpec));
         pNewGRptSpec->SetSegmentKey(segmentKey);

         pNewRptSpec = std::static_pointer_cast<WBFL::Reporting::ReportSpecification>(pNewGRptSpec);
      }
      else
      {
         pNewRptSpec = std::make_shared<CSegmentReportSpecification>(rptDesc.GetReportName(), m_pBroker, segmentKey);
      }

      std::vector<std::_tstring> chList = dlg.m_ChapterList;
      rptDesc.ConfigureReportSpecification(chList, pNewRptSpec);

      return pNewRptSpec;
   }

   return nullptr;
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CSegmentReportSpecificationBuilder::CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const
{
   // Get the selected group and girder
   GET_IFACE2(GetBroker(),ISelection, pSelection);
   CSelection selection = pSelection->GetSelection();

   CSegmentKey segmentKey(m_SegmentKey);
   segmentKey.groupIndex = selection.GroupIdx;
   segmentKey.girderIndex = selection.GirderIdx;
   segmentKey.segmentIndex = selection.SegmentIdx;
   if (selection.Type != CSelection::Segment)
   {
      segmentKey.groupIndex = selection.GroupIdx;
      segmentKey.girderIndex = selection.GirderIdx;
      segmentKey.segmentIndex = selection.SegmentIdx;

      std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec = CreateReportSpec(rptDesc, std::shared_ptr<WBFL::Reporting::ReportSpecification>());
      return pRptSpec;
   }

   std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec(std::make_shared<CSegmentReportSpecification>(rptDesc.GetReportName(), m_pBroker, segmentKey));

   rptDesc.ConfigureReportSpecification(pRptSpec);


   return pRptSpec;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

CMultiGirderReportSpecificationBuilder::CMultiGirderReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker) :
CBrokerReportSpecificationBuilder(pBroker)
{
}

CMultiGirderReportSpecificationBuilder::~CMultiGirderReportSpecificationBuilder(void)
{
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CMultiGirderReportSpecificationBuilder::CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification> pOldRptSpec) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   std::vector<CGirderKey> girderKeys;

   // Get information from old spec if we had one
   std::shared_ptr<CMultiGirderReportSpecification> pOldGRptSpec = std::dynamic_pointer_cast<CMultiGirderReportSpecification>(pOldRptSpec);
   if(pOldGRptSpec)
   {
      girderKeys = pOldGRptSpec->GetGirderKeys();
   }
   else
   {
      GET_IFACE2(GetBroker(),ISelection,pSelection);
      CSelection selection = pSelection->GetSelection();

      CGirderKey girderKey;
      if ( selection.Type == CSelection::Span )
      {
         GET_IFACE2(GetBroker(),IBridge,pBridge);
         girderKey.groupIndex = pBridge->GetGirderGroupIndex(selection.SpanIdx);
         girderKey.girderIndex = 0;
      }
      else if ( selection.Type == CSelection::Girder || selection.Type == CSelection::Segment || selection.Type == CSelection::ClosureJoint )
      {
         girderKey.groupIndex  = selection.GroupIdx;
         girderKey.girderIndex = selection.GirderIdx;
      }
      else
      {
         girderKey.groupIndex  = 0;
         girderKey.girderIndex = 0;
      }

      girderKeys.push_back(girderKey);
   }

   if (girderKeys.size() == 1 && (girderKeys.front().groupIndex == ALL_GROUPS || girderKeys.front().girderIndex == ALL_GIRDERS))
   {
      // multiple girders are selected... fill up the girder key vector
      CGirderKey girderKey = girderKeys.front();
      girderKeys.clear();
      GET_IFACE2(GetBroker(),IBridge, pBridge);
      GroupIndexType nGroups = pBridge->GetGirderGroupCount();
      GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
      GroupIndexType lastGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? nGroups - 1 : firstGroupIdx);
      for (GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++)
      {
         GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
         GirderIndexType firstGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex);
         GirderIndexType lastGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? nGirders - 1 : firstGirderIdx);
         for (GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++)
         {
            girderKeys.emplace_back(grpIdx, gdrIdx);
         }
      }
   }

   // Prompt for span, girder, and chapter list
   CMultiGirderReportDlg dlg(GetBroker(), rptDesc, pOldRptSpec);
   dlg.m_GirderKeys = girderKeys;

   if ( dlg.DoModal() == IDOK )
   {
      // If possible, get information from old spec. Otherwise header/footer and other info will be lost
      std::shared_ptr<WBFL::Reporting::ReportSpecification> pNewRptSpec;
      if(pOldGRptSpec)
      {
         std::shared_ptr<CMultiGirderReportSpecification> pNewGRptSpec = std::make_shared<CMultiGirderReportSpecification>(*pOldGRptSpec);

         pNewGRptSpec->SetGirderKeys(dlg.m_GirderKeys);

         pNewRptSpec = std::static_pointer_cast<WBFL::Reporting::ReportSpecification>(pNewGRptSpec);
      }
      else
      {
         pNewRptSpec = std::make_shared<CMultiGirderReportSpecification>(rptDesc.GetReportName(),m_pBroker,dlg.m_GirderKeys);
      }

      std::vector<std::_tstring> chList = dlg.m_ChapterList;
      rptDesc.ConfigureReportSpecification(chList,pNewRptSpec);

      return pNewRptSpec;
   }

   return nullptr;
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CMultiGirderReportSpecificationBuilder::CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const
{
   GET_IFACE2(GetBroker(),ISelection,pSelection);
   CSelection selection = pSelection->GetSelection();

   CGirderKey girderKey;
   if ( selection.Type == CSelection::Span )
   {
      GET_IFACE2(GetBroker(),IBridge,pBridge);
      girderKey.groupIndex = pBridge->GetGirderGroupIndex(selection.SpanIdx);
      girderKey.girderIndex = 0;
   }
   else if ( selection.Type == CSelection::Girder || selection.Type == CSelection::Segment || selection.Type == CSelection::ClosureJoint )
   {
      girderKey.groupIndex  = selection.GroupIdx;
      girderKey.girderIndex = selection.GirderIdx;
   }
   else
   {
      girderKey.groupIndex  = 0;
      girderKey.girderIndex = 0;
   }

   GET_IFACE2(GetBroker(),IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   if ( girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex != ALL_GIRDERS && nGroups == 1 )
   {
      // there is exactly one group and we have a valid girder number so take ALL_GROUPS to mean group 0... no prompting required
      girderKey.groupIndex = 0;
   }
   else if ( girderKey.groupIndex == ALL_GROUPS || girderKey.girderIndex == ALL_GIRDERS )
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      // we don't have a proper girder key.... prompt the user
      std::shared_ptr<WBFL::Reporting::ReportSpecification> nullSpec;
      CSpanGirderReportDlg dlg(GetBroker(), rptDesc, CSpanGirderReportDlg::Mode::GroupGirderAndChapters, nullSpec);
      dlg.m_SegmentKey.groupIndex = girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex;
      dlg.m_SegmentKey.girderIndex = girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex;

      if ( dlg.DoModal() == IDOK )
      {
         girderKey = dlg.m_SegmentKey;
      }
      else
      {
         return nullptr;
      }
   }

   std::vector<CGirderKey> girderKeys;
   girderKeys.push_back(girderKey);

   std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec( std::make_shared<CMultiGirderReportSpecification>(rptDesc.GetReportName(),m_pBroker, girderKeys) );

   rptDesc.ConfigureReportSpecification(pRptSpec);

   return pRptSpec;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

CMultiViewSpanGirderReportSpecificationBuilder::CMultiViewSpanGirderReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker) :
CSpanReportSpecificationBuilder(pBroker)
{
}

CMultiViewSpanGirderReportSpecificationBuilder::~CMultiViewSpanGirderReportSpecificationBuilder(void)
{
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CMultiViewSpanGirderReportSpecificationBuilder::CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification> pOldRptSpec) const
{
   // First check if we are getting a CGirderReportSpecification. If so, use our bro to take care of this
   std::shared_ptr<CGirderReportSpecification> pGirderRptSpec = std::dynamic_pointer_cast<CGirderReportSpecification,WBFL::Reporting::ReportSpecification>(pOldRptSpec);
   if ( pGirderRptSpec != nullptr )
   {
      std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> pGirderRptSpecBuilder( std::make_shared<CGirderReportSpecificationBuilder>(m_pBroker,pGirderRptSpec->GetGirderKey()) );
      return pGirderRptSpecBuilder->CreateReportSpec(rptDesc, pOldRptSpec);
   }
   else
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      // Prompt for span, girder, and chapter list
      GET_IFACE2(GetBroker(),ISelection,pSelection);
      CSelection selection = pSelection->GetSelection();

      CGirderKey girderKey;
      if ( selection.Type == CSelection::Span )
      {
         GET_IFACE2(GetBroker(),IBridge,pBridge);
         girderKey.groupIndex = pBridge->GetGirderGroupIndex(selection.SpanIdx);
         girderKey.girderIndex = 0;
      }
      else if ( selection.Type == CSelection::Girder || selection.Type == CSelection::Segment || selection.Type == CSelection::ClosureJoint )
      {
         girderKey.groupIndex = (selection.GroupIdx == INVALID_INDEX ? 0 : selection.GroupIdx);
         girderKey.girderIndex = selection.GirderIdx;
      }
      else
      {
         girderKey.groupIndex  = 0;
         girderKey.girderIndex = 0;
      }

      CGirderMultiViewReportDlg dlg(girderKey, GetBroker(), rptDesc, pOldRptSpec);

      if ( dlg.DoModal() == IDOK )
      {
         std::vector<CGirderKey> girderKeys = dlg.GetGirderKeys();

         // If possible, copy information from old spec. Otherwise header/footer and other info will be lost
         std::shared_ptr<CMultiViewSpanGirderReportSpecification> pOldGRptSpec = std::dynamic_pointer_cast<CMultiViewSpanGirderReportSpecification>(pOldRptSpec);

         std::shared_ptr<WBFL::Reporting::ReportSpecification> pNewRptSpec;
         if(pOldGRptSpec)
         {
            std::shared_ptr<CMultiViewSpanGirderReportSpecification> pNewGRptSpec(std::make_shared<CMultiViewSpanGirderReportSpecification>(*pOldGRptSpec) );

            pNewGRptSpec->SetGirderKeys(girderKeys);

            pNewRptSpec = std::static_pointer_cast<WBFL::Reporting::ReportSpecification>(pNewGRptSpec);
         }
         else
         {
            pNewRptSpec = std::make_shared<CMultiViewSpanGirderReportSpecification>(rptDesc.GetReportName(),m_pBroker,girderKeys);
         }

         std::vector<std::_tstring> chList = dlg.m_ChapterList;
         rptDesc.ConfigureReportSpecification(chList,pNewRptSpec);

         return pNewRptSpec;
      }

      return nullptr;
   }
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CMultiViewSpanGirderReportSpecificationBuilder::CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const
{
   GET_IFACE2(GetBroker(),ISelection,pSelection);
   CSelection selection = pSelection->GetSelection();

   CGirderKey girderKey;
   if ( selection.Type == CSelection::Span )
   {
      GET_IFACE2(GetBroker(),IBridge,pBridge);
      girderKey.groupIndex = pBridge->GetGirderGroupIndex(selection.SpanIdx);
      girderKey.girderIndex = 0;
   }
   else if ( selection.Type == CSelection::Girder || selection.Type == CSelection::Segment || selection.Type == CSelection::ClosureJoint )
   {
      girderKey.groupIndex  = selection.GroupIdx;
      girderKey.girderIndex = selection.GirderIdx;
   }
   else
   {
      girderKey.groupIndex  = 0;
      girderKey.girderIndex = 0;
   }

   GET_IFACE2(GetBroker(),IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   if ( girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex != ALL_GIRDERS && nGroups == 1 )
   {
      // there is exactly one group and we have a valid girder number so take ALL_GROUPS to mean group 0... no prompting required
      girderKey.groupIndex = 0;
   }
   else if ( girderKey.groupIndex == ALL_GROUPS || girderKey.girderIndex == ALL_GIRDERS )
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      // we don't have a proper girder key.... prompt the user
      CSpanGirderReportDlg dlg(GetBroker(), rptDesc, CSpanGirderReportDlg::Mode::GroupGirderAndChapters, std::shared_ptr<WBFL::Reporting::ReportSpecification>());
      dlg.m_SegmentKey.groupIndex = girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex;
      dlg.m_SegmentKey.girderIndex = girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex;

      if ( dlg.DoModal() == IDOK )
      {
         girderKey = dlg.m_SegmentKey;
      }
      else
      {
         return nullptr;
      }
   }

   std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec( std::make_shared<CGirderReportSpecification>(rptDesc.GetReportName(),m_pBroker,girderKey) );

   rptDesc.ConfigureReportSpecification(pRptSpec);

   return pRptSpec;
}


////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

CPointOfInterestReportSpecificationBuilder::CPointOfInterestReportSpecificationBuilder(std::weak_ptr<WBFL::EAF::Broker> pBroker) :
CBrokerReportSpecificationBuilder(pBroker)
{
}

CPointOfInterestReportSpecificationBuilder::~CPointOfInterestReportSpecificationBuilder(void)
{
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CPointOfInterestReportSpecificationBuilder::CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification> pOldRptSpec) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for span, girder, and chapter list
   // initialize dialog for the current cut location
   GET_IFACE2(GetBroker(),ISelection,pSelection);
   CSelection selection = pSelection->GetSelection();
   CSegmentKey segmentKey;
   if ( selection.Type == CSelection::Girder )
   {
      segmentKey.groupIndex   = selection.GroupIdx;
      segmentKey.girderIndex  = selection.GirderIdx;
      segmentKey.segmentIndex = 0;
   }
   else if ( selection.Type == CSelection::Segment )
   {
      segmentKey.groupIndex   = selection.GroupIdx;
      segmentKey.girderIndex  = selection.GirderIdx;
      segmentKey.segmentIndex = selection.SegmentIdx;
   }
   else
   {
      segmentKey.groupIndex   = 0;
      segmentKey.girderIndex  = 0;
      segmentKey.segmentIndex = 0;
   }

   GET_IFACE2(GetBroker(),IPointOfInterest,pPOI);
   PoiList vPoi;
   pPOI->GetPointsOfInterest(CSegmentKey(segmentKey.groupIndex, segmentKey.girderIndex, ALL_SEGMENTS), POI_5L | POI_SPAN, &vPoi);
   const pgsPointOfInterest& initial_poi = vPoi.front();

   std::shared_ptr<CPointOfInterestReportSpecification> pOldGRptSpec( std::dynamic_pointer_cast<CPointOfInterestReportSpecification>(pOldRptSpec) );

   CSelectPointOfInterestDlg dlg(GetBroker(), pOldGRptSpec, initial_poi, POI_SPAN);

   if ( dlg.DoModal() == IDOK )
   {
      std::shared_ptr<WBFL::Reporting::ReportSpecification> pNewRptSpec;
      if(pOldGRptSpec)
      {
         // Copy old data from old spec
         std::shared_ptr<CPointOfInterestReportSpecification> pNewGRptSpec(std::make_shared<CPointOfInterestReportSpecification>(*pOldGRptSpec) );

         pNewGRptSpec->SetPointOfInterest(dlg.GetPointOfInterest());

         pNewRptSpec = std::static_pointer_cast<WBFL::Reporting::ReportSpecification>(pNewGRptSpec);
      }
      else
      {
         pNewRptSpec = std::make_shared<CPointOfInterestReportSpecification>(rptDesc.GetReportName(),m_pBroker,dlg.GetPointOfInterest());
      }

      rptDesc.ConfigureReportSpecification(pNewRptSpec);

      return pNewRptSpec;
   }

   return nullptr;
}

std::shared_ptr<WBFL::Reporting::ReportSpecification> CPointOfInterestReportSpecificationBuilder::CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const
{
   // always prompt
   return CreateReportSpec(rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification>());
}
