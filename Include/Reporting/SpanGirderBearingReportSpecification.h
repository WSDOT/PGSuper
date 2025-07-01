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

#pragma once
#include <Reporting\ReportingExp.h>
#include <Reporting\BrokerReportSpecification.h>
#include <ReportManager\ReportManager.h>
#include <ReportManager\ReportHint.h>
#include <PsgLib\Keys.h>
#include <IFace/AnalysisResults.h>





class REPORTINGCLASS CBearingReportHint : public WBFL::Reporting::ReportHint
{
//public:
//   CBearingReportHint();
//   CBearingReportHint(const CBearingData2& bearing,Uint32 lHint);
//
//   void SetHint(Uint32 lHint);
//   Uint32 GetHint();
//
//   void SetBearing(const CBearingData2& bearing);
//   const CBearingData2& GetBearing() const;
//
//   static int IsMyBearing(const std::shared_ptr<const WBFL::Reporting::ReportHint>& pHint, 
//	   const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec);
//
//protected:
//   CBearingData2 m_Bearing;
//   Uint32 m_Hint; // one of GCH_xxx constants in IFace\Project.h
};


class REPORTINGCLASS CBearingReportSpecification :
   public CBrokerReportSpecification
{
public:
   CBearingReportSpecification(const std::_tstring& strReportName, std::weak_ptr<WBFL::EAF::Broker> pBroker,const ReactionLocation& reactionLocation);
   CBearingReportSpecification(const CBearingReportSpecification& other);
   ~CBearingReportSpecification(void);

   virtual std::_tstring GetReportTitle() const override;
   virtual std::_tstring GetReportContextString() const override;

   void SetReactionLocation(const ReactionLocation& );
   const ReactionLocation& GetReactionLocation() const;

   virtual bool IsValid() const override;

protected:
   ReactionLocation m_ReactionLocation;
};



class REPORTINGCLASS CMultiBearingReportSpecification :
   public CBrokerReportSpecification
{
public:
   CMultiBearingReportSpecification(const std::_tstring& strReportName, std::weak_ptr<WBFL::EAF::Broker> pBroker,const std::vector<ReactionLocation>& reactionLocations);
   CMultiBearingReportSpecification(const CMultiBearingReportSpecification& other);
   ~CMultiBearingReportSpecification(void);

   //virtual std::_tstring GetReportTitle() const override;
   //virtual std::_tstring GetReportContextString() const override;

   void SetReactionLocations(const std::vector<ReactionLocation>& reactionLocations);
   const std::vector<ReactionLocation>& GetReactionLocations() const;

   //bool IsMyReactionLocation(const ReactionLocation& reactionLocation) const;

   //virtual bool IsValid() const override;

protected:
   std::vector<ReactionLocation> m_ReactionLocations;
};


//////////////////////////////////////////////////////////
// CMultiViewSpanGirderReportSpecification - open a new window for each girder specified
//////////////////////////////////////////////////////////
class REPORTINGCLASS CMultiViewSpanGirderBearingReportSpecification :
   public CBrokerReportSpecification
{
public:

   CMultiViewSpanGirderBearingReportSpecification(const std::_tstring& strReportName, std::weak_ptr<WBFL::EAF::Broker> pBroker,const std::vector<ReactionLocation>& reactionLocations);
   CMultiViewSpanGirderBearingReportSpecification(const CMultiViewSpanGirderBearingReportSpecification& other);
   ~CMultiViewSpanGirderBearingReportSpecification(void);
//
//   virtual std::_tstring GetReportTitle() const override;
//   virtual std::_tstring GetReportContextString() const override;
//
   void SetReactionLocations(const std::vector<ReactionLocation>& reactionLocations);
   const std::vector<ReactionLocation>& GetReactionLocations() const;
//
//   int IsMyBearing(const CBearingData2& bearing) const;
//
//   virtual bool IsValid() const override;
//
protected:
   std::vector<ReactionLocation> m_ReactionLocations;
};

