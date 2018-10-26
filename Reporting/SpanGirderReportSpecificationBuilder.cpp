///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#include "SelectPointOfInterestDlg.h"

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

boost::shared_ptr<CReportSpecification> CSpanReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pOldRptSpec)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for span and chapter list
   GET_IFACE(ISelection,pSelection);
   SpanIndexType spanIdx = pSelection->GetSelectedSpan();
   spanIdx = (spanIdx == INVALID_INDEX ? 0 : spanIdx );

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   GroupIndexType grpIdx = pGroup->GetIndex();

   CSpanGirderReportDlg dlg(m_pBroker,rptDesc,SpanAndChapters,pOldRptSpec); // span only mode
   dlg.m_Group = grpIdx;

   if ( dlg.DoModal() == IDOK )
   {
      // If possible, copy information from old spec. Otherwise header/footer and other info will be lost
      boost::shared_ptr<CSpanReportSpecification> pOldGRptSpec( boost::dynamic_pointer_cast<CSpanReportSpecification>(pOldRptSpec) );

      boost::shared_ptr<CReportSpecification> pNewRptSpec;
      if(pOldGRptSpec)
      {
         boost::shared_ptr<CSpanReportSpecification> pNewGRptSpec = boost::shared_ptr<CSpanReportSpecification>( new CSpanReportSpecification(*pOldGRptSpec) );

         pNewGRptSpec->SetSpan(dlg.m_Group);

         pNewRptSpec = boost::static_pointer_cast<CReportSpecification>(pNewGRptSpec);
      }
      else
      {
         pNewRptSpec = boost::shared_ptr<CSpanReportSpecification>( new CSpanReportSpecification(rptDesc.GetReportName(),m_pBroker,dlg.m_Group) );
      }

      std::vector<std::_tstring> chList = dlg.m_ChapterList;
      rptDesc.ConfigureReportSpecification(chList,pNewRptSpec);

      return pNewRptSpec;
   }

   return boost::shared_ptr<CReportSpecification>();
}

boost::shared_ptr<CReportSpecification> CSpanReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   // Get the selected span and girder
   GET_IFACE(ISelection,pSelection);
   SpanIndexType spanIdx = pSelection->GetSelectedSpan();

   spanIdx = (spanIdx == INVALID_INDEX ? 0 : spanIdx );
   boost::shared_ptr<CReportSpecification> pRptSpec( new CSpanReportSpecification(rptDesc.GetReportName(),m_pBroker,spanIdx) );

   rptDesc.ConfigureReportSpecification(pRptSpec);

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

boost::shared_ptr<CReportSpecification> CGirderLineReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pOldRptSpec)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for span and chapter list
   GET_IFACE(ISelection,pSelection);
   CGirderKey girderKey = pSelection->GetSelectedGirder();
   girderKey.groupIndex  = (girderKey.groupIndex  == INVALID_INDEX ? 0 : girderKey.groupIndex);
   girderKey.girderIndex = (girderKey.girderIndex == INVALID_INDEX ? 0 : girderKey.girderIndex);

   CSpanGirderReportDlg dlg(m_pBroker,rptDesc,GirderAndChapters,pOldRptSpec);
   dlg.m_Group  = girderKey.groupIndex;
   dlg.m_Girder = girderKey.girderIndex;

   if ( dlg.DoModal() == IDOK )
   {
      // If possible, copy information from old spec. Otherwise header/footer and other info will be lost
      boost::shared_ptr<CGirderLineReportSpecification> pOldGRptSpec( boost::dynamic_pointer_cast<CGirderLineReportSpecification>(pOldRptSpec) );

      boost::shared_ptr<CReportSpecification> pNewRptSpec;
      if(pOldGRptSpec)
      {
         boost::shared_ptr<CGirderLineReportSpecification> pNewGRptSpec = boost::shared_ptr<CGirderLineReportSpecification>( new CGirderLineReportSpecification(*pOldGRptSpec) );

         pNewGRptSpec->SetGirderIndex(dlg.m_Girder);

         pNewRptSpec = boost::static_pointer_cast<CReportSpecification>(pNewGRptSpec);
      }
      else
      {
         pNewRptSpec = boost::shared_ptr<CGirderLineReportSpecification>( new CGirderLineReportSpecification(rptDesc.GetReportName(),m_pBroker,dlg.m_Girder) );
      }

      std::vector<std::_tstring> chList = dlg.m_ChapterList;
      rptDesc.ConfigureReportSpecification(chList,pNewRptSpec);

      return pNewRptSpec;
   }

   return boost::shared_ptr<CReportSpecification>();
}

boost::shared_ptr<CReportSpecification> CGirderLineReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   // Get the selected span and girder
   GET_IFACE(ISelection,pSelection);
   CGirderKey girderKey = pSelection->GetSelectedGirder();
   girderKey.groupIndex  = (girderKey.groupIndex  == INVALID_INDEX ? 0 : girderKey.groupIndex);
   girderKey.girderIndex = (girderKey.girderIndex == INVALID_INDEX ? 0 : girderKey.girderIndex);

   boost::shared_ptr<CReportSpecification> pRptSpec( new CGirderLineReportSpecification(rptDesc.GetReportName(),m_pBroker,girderKey.girderIndex) );

   rptDesc.ConfigureReportSpecification(pRptSpec);

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

boost::shared_ptr<CReportSpecification> CGirderReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pOldRptSpec)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for group, girder, and chapter list
   GET_IFACE(ISelection,pSelection);
   CSelection selection = pSelection->GetSelection();
   CGirderKey girderKey(m_GirderKey);
   if ( selection.Type == CSelection::Segment || selection.Type == CSelection::Girder )
   {
      girderKey.groupIndex   = selection.GroupIdx;
      girderKey.girderIndex  = selection.GirderIdx;
   }

   CSpanGirderReportDlg dlg(m_pBroker,rptDesc,SpanGirderAndChapters,pOldRptSpec);
   dlg.m_Group  = girderKey.groupIndex;
   dlg.m_Girder = girderKey.girderIndex;

   if ( dlg.DoModal() == IDOK )
   {
      girderKey.groupIndex  = dlg.m_Group;
      girderKey.girderIndex = dlg.m_Girder;

      // If possible, copy information from old spec. Otherwise header/footer and other info will be lost
      boost::shared_ptr<CGirderReportSpecification> pOldGRptSpec = boost::dynamic_pointer_cast<CGirderReportSpecification>(pOldRptSpec);

      boost::shared_ptr<CReportSpecification> pNewRptSpec;
      if(pOldGRptSpec)
      {
         boost::shared_ptr<CGirderReportSpecification> pNewGRptSpec = boost::shared_ptr<CGirderReportSpecification>( new CGirderReportSpecification(*pOldGRptSpec) );
         pNewGRptSpec->SetGirderKey(girderKey);

         pNewRptSpec = boost::static_pointer_cast<CReportSpecification>(pNewGRptSpec);
      }
      else
      {
         pNewRptSpec = boost::shared_ptr<CGirderReportSpecification>( new CGirderReportSpecification(rptDesc.GetReportName(),m_pBroker,girderKey) );
      }

      std::vector<std::_tstring> chList = dlg.m_ChapterList;
      rptDesc.ConfigureReportSpecification(chList,pNewRptSpec);

      return pNewRptSpec;
   }

   return boost::shared_ptr<CReportSpecification>();
}

boost::shared_ptr<CReportSpecification> CGirderReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   // Get the selected group and girder
   GET_IFACE(ISelection,pSelection);
   CSelection selection = pSelection->GetSelection();
   CGirderKey girderKey(m_GirderKey);
   if ( selection.Type == CSelection::Girder || selection.Type == CSelection::Segment )
   {
      GET_IFACE(IBridge,pBridge);
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

         boost::shared_ptr<CReportSpecification> nullSpec;
         boost::shared_ptr<CReportSpecification> pRptSpec = CreateReportSpec(rptDesc,nullSpec);

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
   boost::shared_ptr<CReportSpecification> pRptSpec( new CGirderReportSpecification(rptDesc.GetReportName(),m_pBroker,girderKey) );

   rptDesc.ConfigureReportSpecification(pRptSpec);


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

boost::shared_ptr<CReportSpecification> CMultiGirderReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pOldRptSpec)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   std::vector<CGirderKey> girderKeys;

   // Get information from old spec if we had one
   boost::shared_ptr<CMultiGirderReportSpecification> pOldGRptSpec = boost::dynamic_pointer_cast<CMultiGirderReportSpecification>(pOldRptSpec);
   if(pOldGRptSpec)
   {
      girderKeys = pOldGRptSpec->GetGirderKeys();
   }
   else
   {
      GET_IFACE(ISelection,pSelection);
      CSelection selection = pSelection->GetSelection();

      CGirderKey girderKey;
      if ( selection.Type == CSelection::Span )
      {
         GET_IFACE(IBridge,pBridge);
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

   // Prompt for span, girder, and chapter list
   CMultiGirderReportDlg dlg(m_pBroker,rptDesc,pOldRptSpec);
   dlg.m_GirderKeys = girderKeys;

   if ( dlg.DoModal() == IDOK )
   {
      // If possible, get information from old spec. Otherwise header/footer and other info will be lost
      boost::shared_ptr<CReportSpecification> pNewRptSpec;
      if(pOldGRptSpec)
      {
         boost::shared_ptr<CMultiGirderReportSpecification> pNewGRptSpec = boost::shared_ptr<CMultiGirderReportSpecification>( new CMultiGirderReportSpecification(*pOldGRptSpec) );

         pNewGRptSpec->SetGirderKeys(dlg.m_GirderKeys);

         pNewRptSpec = boost::static_pointer_cast<CReportSpecification>(pNewGRptSpec);
      }
      else
      {
         pNewRptSpec = boost::shared_ptr<CMultiGirderReportSpecification>( new CMultiGirderReportSpecification(rptDesc.GetReportName(),m_pBroker,dlg.m_GirderKeys) );
      }

      std::vector<std::_tstring> chList = dlg.m_ChapterList;
      rptDesc.ConfigureReportSpecification(chList,pNewRptSpec);

      return pNewRptSpec;
   }

   return boost::shared_ptr<CReportSpecification>();
}

boost::shared_ptr<CReportSpecification> CMultiGirderReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   GET_IFACE(ISelection,pSelection);
   CSelection selection = pSelection->GetSelection();

   CGirderKey girderKey;
   if ( selection.Type == CSelection::Span )
   {
      GET_IFACE(IBridge,pBridge);
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

   GET_IFACE(IBridge,pBridge);
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
      boost::shared_ptr<CReportSpecification> nullSpec;
      CSpanGirderReportDlg dlg(m_pBroker,rptDesc,SpanGirderAndChapters,nullSpec);
      dlg.m_Group  = girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex;
      dlg.m_Girder = girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex;

      if ( dlg.DoModal() == IDOK )
      {
         girderKey.groupIndex  = dlg.m_Group;
         girderKey.girderIndex = dlg.m_Girder;
      }
      else
      {
         return boost::shared_ptr<CReportSpecification>();
      }
   }

   std::vector<CGirderKey> girderKeys;
   girderKeys.push_back(girderKey);

   boost::shared_ptr<CReportSpecification> pRptSpec( new CMultiGirderReportSpecification(rptDesc.GetReportName(),m_pBroker, girderKeys) );

   rptDesc.ConfigureReportSpecification(pRptSpec);

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

boost::shared_ptr<CReportSpecification> CMultiViewSpanGirderReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pOldRptSpec)
{
   // First check if we are getting a CGirderReportSpecification. If so, use our bro to take care of this
   boost::shared_ptr<CGirderReportSpecification> pGirderRptSpec = boost::dynamic_pointer_cast<CGirderReportSpecification,CReportSpecification>(pOldRptSpec);
   if ( pGirderRptSpec != NULL )
   {
      boost::shared_ptr<CReportSpecificationBuilder> pGirderRptSpecBuilder( new CGirderReportSpecificationBuilder(m_pBroker,pGirderRptSpec->GetGirderKey()) );
      return pGirderRptSpecBuilder->CreateReportSpec(rptDesc, pOldRptSpec);
   }
   else
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      // Prompt for span, girder, and chapter list
      GET_IFACE(ISelection,pSelection);
      CSelection selection = pSelection->GetSelection();

      CGirderKey girderKey;
      if ( selection.Type == CSelection::Span )
      {
         GET_IFACE(IBridge,pBridge);
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

      ASSERT_GIRDER_KEY(girderKey);

      CMultiViewReportDlg dlg(m_pBroker,rptDesc,pOldRptSpec,girderKey);

      if ( dlg.DoModal() == IDOK )
      {
         std::vector<CGirderKey> girderKeys = dlg.GetGirderKeys();

         // If possible, copy information from old spec. Otherwise header/footer and other info will be lost
         boost::shared_ptr<CMultiViewSpanGirderReportSpecification> pOldGRptSpec = boost::dynamic_pointer_cast<CMultiViewSpanGirderReportSpecification>(pOldRptSpec);

         boost::shared_ptr<CReportSpecification> pNewRptSpec;
         if(pOldGRptSpec)
         {
            boost::shared_ptr<CMultiViewSpanGirderReportSpecification> pNewGRptSpec = boost::shared_ptr<CMultiViewSpanGirderReportSpecification>( new CMultiViewSpanGirderReportSpecification(*pOldGRptSpec) );

            pNewGRptSpec->SetGirderKeys(girderKeys);

            pNewRptSpec = boost::static_pointer_cast<CReportSpecification>(pNewGRptSpec);
         }
         else
         {
            pNewRptSpec = boost::shared_ptr<CMultiViewSpanGirderReportSpecification>( new CMultiViewSpanGirderReportSpecification(rptDesc.GetReportName(),m_pBroker,girderKeys) );
         }

         std::vector<std::_tstring> chList = dlg.m_ChapterList;
         rptDesc.ConfigureReportSpecification(chList,pNewRptSpec);

         return pNewRptSpec;
      }

      return boost::shared_ptr<CReportSpecification>();
   }
}

boost::shared_ptr<CReportSpecification> CMultiViewSpanGirderReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   GET_IFACE(ISelection,pSelection);
   CSelection selection = pSelection->GetSelection();

   CGirderKey girderKey;
   if ( selection.Type == CSelection::Span )
   {
      GET_IFACE(IBridge,pBridge);
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

   GET_IFACE(IBridge,pBridge);
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
      boost::shared_ptr<CReportSpecification> nullSpec;
      CSpanGirderReportDlg dlg(m_pBroker,rptDesc,SpanGirderAndChapters,nullSpec);
      dlg.m_Group  = girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex;
      dlg.m_Girder = girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex;

      if ( dlg.DoModal() == IDOK )
      {
         girderKey.groupIndex  = dlg.m_Group;
         girderKey.girderIndex = dlg.m_Girder;
      }
      else
      {
         return boost::shared_ptr<CReportSpecification>();
      }
   }

   boost::shared_ptr<CReportSpecification> pRptSpec( new CGirderReportSpecification(rptDesc.GetReportName(),m_pBroker,girderKey) );

   rptDesc.ConfigureReportSpecification(pRptSpec);

   return pRptSpec;
}


////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

CPointOfInterestReportSpecificationBuilder::CPointOfInterestReportSpecificationBuilder(IBroker* pBroker) :
CBrokerReportSpecificationBuilder(pBroker)
{
}

CPointOfInterestReportSpecificationBuilder::~CPointOfInterestReportSpecificationBuilder(void)
{
}

boost::shared_ptr<CReportSpecification> CPointOfInterestReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pOldRptSpec)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for span, girder, and chapter list
   // initialize dialog for the current cut location
   GET_IFACE(ISelection,pSelection);
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

   GET_IFACE(IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPoi( pPOI->GetPointsOfInterest(CSegmentKey(segmentKey.groupIndex,segmentKey.girderIndex,ALL_SEGMENTS),POI_SPAN | POI_5L) );
   pgsPointOfInterest initial_poi = vPoi.front();

   boost::shared_ptr<CPointOfInterestReportSpecification> pOldGRptSpec( boost::dynamic_pointer_cast<CPointOfInterestReportSpecification>(pOldRptSpec) );

   CSelectPointOfInterestDlg dlg(m_pBroker,pOldGRptSpec,initial_poi,POI_SPAN);

   if ( dlg.DoModal() == IDOK )
   {
      boost::shared_ptr<CReportSpecification> pNewRptSpec;
      if(pOldGRptSpec)
      {
         // Copy old data from old spec
         boost::shared_ptr<CPointOfInterestReportSpecification> pNewGRptSpec = boost::shared_ptr<CPointOfInterestReportSpecification>( new CPointOfInterestReportSpecification(*pOldGRptSpec) );

         pNewGRptSpec->SetPointOfInterest(dlg.GetPointOfInterest());

         pNewRptSpec = boost::static_pointer_cast<CReportSpecification>(pNewGRptSpec);
      }
      else
      {
         pNewRptSpec = boost::shared_ptr<CPointOfInterestReportSpecification>( new CPointOfInterestReportSpecification(rptDesc.GetReportName(),m_pBroker,dlg.GetPointOfInterest()) );
      }

      rptDesc.ConfigureReportSpecification(pNewRptSpec);

      return pNewRptSpec;
   }

   return boost::shared_ptr<CReportSpecification>();
}

boost::shared_ptr<CReportSpecification> CPointOfInterestReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   // always prompt
   boost::shared_ptr<CReportSpecification> nullSpec;
   return CreateReportSpec(rptDesc,nullSpec);
}
