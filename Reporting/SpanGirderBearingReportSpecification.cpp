/////////////////////////////////////////////////////////////////////////
//// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
//// Copyright © 1999-2025  Washington State Department of Transportation
////                        Bridge and Structures Office
////
//// This program is free software; you can redistribute it and/or modify
//// it under the terms of the Alternate Route Open Source License as 
//// published by the Washington State Department of Transportation, 
//// Bridge and Structures Office.
////
//// This program is distributed in the hope that it will be useful, but 
//// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
//// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
//// the Alternate Route Open Source License for more details.
////
//// You should have received a copy of the Alternate Route Open Source 
//// License along with this program; if not, write to the Washington 
//// State Department of Transportation, Bridge and Structures Office, 
//// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
//// Bridge_Support@wsdot.wa.gov
/////////////////////////////////////////////////////////////////////////
//
#include "stdafx.h"
#include <Reporting\SpanGirderBearingReportSpecification.h>
//#include <IFace\Bridge.h>
#include <IFace\DocumentType.h>
#include <AgentTools.h>
//
//

//
//
//
//
///////////////////////////////////////////////////////////////////////////////////////////////
//
//CBearingReportHint::CBearingReportHint()
//{
//   m_Hint    = 0;
//}
//
//CBearingReportHint::CBearingReportHint(const CBearingData2& bearing,Uint32 lHint) :
//m_Bearing(bearing),m_Hint(lHint)
//{
//}
//
//void CBearingReportHint::SetHint(Uint32 lHint)
//{
//   m_Hint = lHint;
//}
//
//Uint32 CBearingReportHint::GetHint()
//{
//   return m_Hint;
//}
//
//void CBearingReportHint::SetBearing(const CBearingData2& bearing)
//{
//   m_Bearing = bearing;
//}
//
//const CBearingData2& CBearingReportHint::GetBearing() const
//{
//   return m_Bearing;
//}
//
//int CBearingReportHint::IsMyBearing(const std::shared_ptr<const WBFL::Reporting::ReportHint>& pHint, 
//    const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec)
//{
//   auto pBearingRptHint = std::dynamic_pointer_cast<const CBearingReportHint>(pHint);
//   if ( pBearingRptHint == nullptr )
//   {
//      return -1;
//   }
//
//   auto pBearingRptSpec = std::dynamic_pointer_cast<const CBearingReportSpecification>(pRptSpec);
//   if ( pBearingRptSpec != nullptr )
//   {
//      return (pBearingRptHint->m_Bearing == pBearingRptSpec->GetBearing() ? 1 : 0);
//   }
//   else
//   {
//      auto pMultiBearingRptSpec = std::dynamic_pointer_cast<const CMultiBearingReportSpecification>(pRptSpec);
//      if ( pMultiBearingRptSpec != nullptr )
//      {
//         return pMultiBearingRptSpec->IsMyBearing(pBearingRptHint->m_Bearing);
//      }
//   }
//
//   return -1;
//}
//
CBearingReportSpecification::CBearingReportSpecification(const std::_tstring& strReportName, 
    std::weak_ptr<WBFL::EAF::Broker> pBroker, const ReactionLocation& reactionLocation) :
CBrokerReportSpecification(strReportName,pBroker)
{
   m_ReactionLocation = reactionLocation;
}

CBearingReportSpecification::CBearingReportSpecification(const CBearingReportSpecification& other) :
CBrokerReportSpecification(other)
{
   m_ReactionLocation = other.m_ReactionLocation;
}

CBearingReportSpecification::~CBearingReportSpecification(void)
{
}

std::_tstring CBearingReportSpecification::GetReportTitle() const
{
   return GetReportName() + _T(" - ") + GetReportContextString();
}

std::_tstring CBearingReportSpecification::GetReportContextString() const
{
   GroupIndexType grpIdx  = m_ReactionLocation.GirderKey.groupIndex;
   GirderIndexType gdrIdx = m_ReactionLocation.GirderKey.girderIndex;

   GET_IFACE2(GetBroker(), IDocumentType,pDocType);
   bool bIsPGSuper = pDocType->IsPGSuperDocument();

   CString msg;
   if (bIsPGSuper)
   {
      if ( grpIdx != INVALID_INDEX && gdrIdx != INVALID_INDEX)
      {
         msg.Format(_T("Span %s Girder %s - %s"), LABEL_SPAN(m_ReactionLocation.GirderKey.groupIndex), 
             LABEL_GIRDER(m_ReactionLocation.GirderKey.girderIndex), m_ReactionLocation.PierLabel.c_str());
      }
      else if( grpIdx != INVALID_INDEX )
      {
         msg.Format(_T("Span %s"), LABEL_SPAN(m_ReactionLocation.GirderKey.groupIndex));
      }
      else if ( gdrIdx != INVALID_INDEX )
      {
         msg.Format(_T("Girder %s"), LABEL_GIRDER(m_ReactionLocation.GirderKey.girderIndex));
      }
   }
   else
   {
      if ( grpIdx != INVALID_INDEX && gdrIdx != INVALID_INDEX )
      {
          msg.Format(_T("Span %s Girder %s - %s"), LABEL_SPAN(m_ReactionLocation.GirderKey.groupIndex),
              LABEL_GIRDER(m_ReactionLocation.GirderKey.girderIndex), m_ReactionLocation.PierLabel.c_str());
      }
      else if( grpIdx != INVALID_INDEX )
      {
         msg.Format(_T("Group %d"), LABEL_GROUP(m_ReactionLocation.GirderKey.groupIndex));
      }
      else if ( gdrIdx != INVALID_INDEX )
      {
         msg.Format(_T("Girder %s"), LABEL_GIRDER(m_ReactionLocation.GirderKey.girderIndex));
      }
   }

   return std::_tstring(msg);
}
//
//void CBearingReportSpecification::SetGroupIndex(GroupIndexType grpIdx)
//{
//   //m_GirderKey.groupIndex = grpIdx;
//}
//
////GroupIndexType CBearingReportSpecification::GetGroupIndex() const
////{
////   //return m_GirderKey.groupIndex;
////}
//
//void CBearingReportSpecification::SetGirderIndex(GirderIndexType gdrIdx)
//{
//   //m_GirderKey.girderIndex = gdrIdx;
//}
//
////GirderIndexType CBearingReportSpecification::GetGirderIndex() const
////{
////   //return m_GirderKey.girderIndex;
////}
//
void CBearingReportSpecification::SetReactionLocation(const ReactionLocation& reactionLocation)
{
   m_ReactionLocation = reactionLocation;
}

const ReactionLocation& CBearingReportSpecification::GetReactionLocation() const
{
   return m_ReactionLocation;
}

bool CBearingReportSpecification::IsValid() const
{
   //GET_IFACE(IBridge,pBridge);

   //GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   //if (nGroups <= m_GirderKey.groupIndex && m_GirderKey.groupIndex != ALL_GROUPS)
   //{
   //   // the group index is out of range (group probably got deleted)
   //   return false;
   //}

   //GirderIndexType nGirders = 0;
   //if (m_GirderKey.groupIndex == ALL_GROUPS)
   //{
   //   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   //   {
   //      nGirders = Max(nGirders, pBridge->GetGirderCount(grpIdx));
   //   }
   //}
   //else
   //{
   //   nGirders = pBridge->GetGirderCount(m_GirderKey.groupIndex);
   //}

   //if ( nGirders <= m_GirderKey.girderIndex )
   //{
   //   return false;
   //}

   return CBrokerReportSpecification::IsValid();
}
//
//
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//
CMultiBearingReportSpecification::CMultiBearingReportSpecification(const std::_tstring& strReportName, std::weak_ptr<WBFL::EAF::Broker> pBroker,const std::vector<ReactionLocation>& reactionLocations) :
CBrokerReportSpecification(strReportName,pBroker),
m_ReactionLocations(reactionLocations)
{
}

CMultiBearingReportSpecification::CMultiBearingReportSpecification(const CMultiBearingReportSpecification& other) :
CBrokerReportSpecification(other)
{
   m_ReactionLocations = other.m_ReactionLocations;
}

CMultiBearingReportSpecification::~CMultiBearingReportSpecification(void)
{
}

////std::_tstring CMultiGirderReportSpecification::GetReportTitle() const
////{
////   return GetReportName() + _T(" - ") + GetReportContextString();
////}
////
////// comparator to sort girderkeys
////struct gkeyComparer
////{
////   bool operator() (CGirderKey i, CGirderKey j)
////   {
////      if (i.groupIndex == j.groupIndex)
////      {
////         return i.girderIndex < j.girderIndex;
////      }
////      else
////      {
////         return i.groupIndex < j.groupIndex;
////      }
////   }
////};
////
////std::_tstring CMultiGirderReportSpecification::GetReportContextString() const
////{
////   GET_IFACE(IDocumentType,pDocType);
////   bool bIsPGSuper = pDocType->IsPGSuperDocument();
////
////   // sort so that keys are grouped by span
////   std::vector<CGirderKey> Keys(m_GirderKeys);
////   gkeyComparer comp;
////   std::sort(Keys.begin(), Keys.end(), comp);
////
////   std::_tostringstream os;
////   bool bFirstGrp(true);
////   GroupIndexType curGrp = Keys.front().groupIndex;
////   for (auto& key : Keys)
////   {
////      if (bFirstGrp || key.groupIndex != curGrp)
////      {
////         if (!bFirstGrp)
////         {
////            os << _T("), ");
////         }
////
////         if (bIsPGSuper)
////         {
////            os << _T("Span ") << LABEL_SPAN(key.groupIndex) << _T(" (Girder ") << LABEL_GIRDER(key.girderIndex);
////         }
////         else
////         {
////            os << _T("Group ") << LABEL_GROUP(key.groupIndex) << _T(" (Girder ") << LABEL_GIRDER(key.girderIndex);
////         }
////
////         curGrp = key.groupIndex;
////      }
////      else
////      {
////         os << _T(",") << LABEL_GIRDER(key.girderIndex);
////      }
////
////      bFirstGrp = false;
////   }
////
////   os << _T(")");
////
////   return os.str();
////}
////
void CMultiBearingReportSpecification::SetReactionLocations(const std::vector<ReactionLocation>& reactionLocations)
{
   m_ReactionLocations = reactionLocations;
}

const std::vector<ReactionLocation>& CMultiBearingReportSpecification::GetReactionLocations() const
{
   return m_ReactionLocations;
}
////
////bool CMultiGirderReportSpecification::IsMyGirder(const CGirderKey& girderKey) const
////{
////   std::vector<CGirderKey>::const_iterator it = std::find(m_GirderKeys.begin(), m_GirderKeys.end(), girderKey);
////
////   return (it != m_GirderKeys.end()) ? true : false;
////}
////
////bool CMultiGirderReportSpecification::IsValid() const
////{
////   GET_IFACE(IBridge,pBridge);
////   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
////   std::vector<CGirderKey>::const_iterator iter(m_GirderKeys.begin());
////   std::vector<CGirderKey>::const_iterator end(m_GirderKeys.end());
////   for ( ; iter != end; iter++ )
////   {
////      const CGirderKey& girderKey(*iter);
////
////      if ( nGroups <= girderKey.groupIndex )
////      {
////         return false;
////      }
////
////      GirderIndexType nGirders = pBridge->GetGirderCount(girderKey.groupIndex);
////      if ( nGirders <= girderKey.girderIndex )
////      {
////         return false;
////      }
////   }
////
////   return CBrokerReportSpecification::IsValid();
////}
//
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

CMultiViewSpanGirderBearingReportSpecification::CMultiViewSpanGirderBearingReportSpecification(const std::_tstring& strReportName,
    std::weak_ptr<WBFL::EAF::Broker> pBroker, const std::vector<ReactionLocation>& reactionLocations) :
CBrokerReportSpecification(strReportName,pBroker)
{
   SetReactionLocations(reactionLocations);
}

CMultiViewSpanGirderBearingReportSpecification::CMultiViewSpanGirderBearingReportSpecification(const CMultiViewSpanGirderBearingReportSpecification& other) :
CBrokerReportSpecification(other)
{
   SetReactionLocations(other.m_ReactionLocations);
}

CMultiViewSpanGirderBearingReportSpecification::~CMultiViewSpanGirderBearingReportSpecification(void)
{
}

//std::_tstring CMultiViewSpanGirderBearingReportSpecification::GetReportTitle() const
//{
//   return GetReportName();
//}
//
//std::_tstring CMultiViewSpanGirderBearingReportSpecification::GetReportContextString() const
//{
//   return std::_tstring(); // may want to revisit
//}
//
void CMultiViewSpanGirderBearingReportSpecification::SetReactionLocations(const std::vector<ReactionLocation>& reactionLocations)
{
   m_ReactionLocations = reactionLocations;
}

const std::vector<ReactionLocation>& CMultiViewSpanGirderBearingReportSpecification::GetReactionLocations() const
{
   return m_ReactionLocations;
}
//
//int CMultiViewSpanGirderBearingReportSpecification::IsMyBearing(const CBearingData2& bearing) const
//{
//   std::vector<CBearingData2>::const_iterator it = std::find(m_Bearings.begin(), m_Bearings.end(), bearing);
//
//   return (it != m_Bearings.end()) ? 1 : 0;
//}
//
//bool CMultiViewSpanGirderBearingReportSpecification::IsValid() const
//{
//   //GET_IFACE(IBridge,pBridge);
//   //GroupIndexType nGroups = pBridge->GetGirderGroupCount();
//
//   //std::vector<CGirderKey>::const_iterator iter(m_GirderKeys.begin());
//   //std::vector<CGirderKey>::const_iterator end(m_GirderKeys.end());
//   //for ( ; iter != end; iter++ )
//   //{
//   //   const CGirderKey& girderKey(*iter);
//
//   //   if ( nGroups <= girderKey.groupIndex )
//   //   {
//   //      return false;
//   //   }
//
//   //   GirderIndexType nGdrs = pBridge->GetGirderCount(girderKey.groupIndex);
//
//   //   if ( nGdrs <= girderKey.groupIndex )
//   //   {
//   //      return false;
//   //   }
//   //}
//
//   return CBrokerReportSpecification::IsValid();
//}
//
