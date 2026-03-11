/////////////////////////////////////////////////////////////////////////
//// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
//// Copyright © 1999-2026  Washington State Department of Transportation
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
#include <IFace\DocumentType.h>
#include <AgentTools.h>

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
          msg.Format(_T("Group %s Girder %s - %s"), LABEL_SPAN(m_ReactionLocation.GirderKey.groupIndex),
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

std::_tstring CMultiBearingReportSpecification::GetReportTitle() const
{
   return GetReportName() + _T(" - ") + GetReportContextString();
}


// comparator to sort girder keys
struct rlComparer
{
    bool operator() (ReactionLocation i, ReactionLocation j)
    {
        if (i.GirderKey.groupIndex == j.GirderKey.groupIndex)
        {
            return i.GirderKey.girderIndex < j.GirderKey.girderIndex;
        }
        else
        {
            return i.GirderKey.groupIndex < j.GirderKey.groupIndex;
        }
    }
};

std::_tstring CMultiBearingReportSpecification::GetReportContextString() const
{
   GET_IFACE2(GetBroker(), IDocumentType,pDocType);
   bool bIsPGSuper = pDocType->IsPGSuperDocument();

   // sort so that reaction locations are grouped by span/girder
   std::vector<ReactionLocation> rls(m_ReactionLocations);
   rlComparer comp;
   std::sort(rls.begin(), rls.end(), comp);

   std::_tostringstream os;
   bool bFirstGrp(true);
   GroupIndexType curGrp = m_ReactionLocations.front().GirderKey.groupIndex;
   for (auto& rl : rls)
   {
      if (bFirstGrp || rl.GirderKey.groupIndex != curGrp)
      {
         if (!bFirstGrp)
         {
            os << _T("), ");
         }

         if (bIsPGSuper)
         {
            os << _T("Span ") << LABEL_SPAN(rl.GirderKey.groupIndex) << _T(" (Girder ") << LABEL_GIRDER(rl.GirderKey.girderIndex) << _T(" - ") << rl.PierLabel.c_str();
         }
         else
         {
            os << _T("Group ") << LABEL_GROUP(rl.GirderKey.groupIndex) << _T(" (Girder ") << LABEL_GIRDER(rl.GirderKey.girderIndex) << _T(" - ") << rl.PierLabel.c_str();
         }

         curGrp = rl.GirderKey.groupIndex;
      }
      else
      {
         os << _T(",") << _T(" Girder ") << LABEL_GIRDER(rl.GirderKey.girderIndex) << _T(" - ") << rl.PierLabel.c_str();
      }

      bFirstGrp = false;
   }

   os << _T(")");

   return os.str();
}

void CMultiBearingReportSpecification::SetReactionLocations(const std::vector<ReactionLocation>& reactionLocations)
{
   m_ReactionLocations = reactionLocations;
}

const std::vector<ReactionLocation>& CMultiBearingReportSpecification::GetReactionLocations() const
{
   return m_ReactionLocations;
}

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

void CMultiViewSpanGirderBearingReportSpecification::SetReactionLocations(const std::vector<ReactionLocation>& reactionLocations)
{
   m_ReactionLocations = reactionLocations;
}

const std::vector<ReactionLocation>& CMultiViewSpanGirderBearingReportSpecification::GetReactionLocations() const
{
   return m_ReactionLocations;
}

