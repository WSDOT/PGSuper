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
#include <Reporting\SpanGirderReportSpecificationBuilder.h>
#include <Reporting\SpanGirderReportSpecification.h>
#include <ReportManager\ReportManager.h>
#include <WBFLCore.h>

/////////////////// Full load rating report ///////////////////

class REPORTINGCLASS CLoadRatingReportSpecificationBuilder :
   public CBrokerReportSpecificationBuilder
{
public:
   CLoadRatingReportSpecificationBuilder(IBroker* pBroker);
   ~CLoadRatingReportSpecificationBuilder(void);

   virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec) const override;
   virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const override;
};

////////////// Summary Load Rating Report //////////////////////

class REPORTINGCLASS CLoadRatingSummaryReportSpecificationBuilder :
   public CBrokerReportSpecificationBuilder
{
public:
   CLoadRatingSummaryReportSpecificationBuilder(IBroker* pBroker);
   ~CLoadRatingSummaryReportSpecificationBuilder(void);

   virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateReportSpec(const WBFL::Reporting::ReportDescription& rptDesc,std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec) const override;
   virtual std::shared_ptr<WBFL::Reporting::ReportSpecification> CreateDefaultReportSpec(const WBFL::Reporting::ReportDescription& rptDesc) const override;
};

// Base class for LoadRatingReportSpecs
class REPORTINGCLASS CLoadRatingReportSpecificationBase
{
public:
   CLoadRatingReportSpecificationBase(bool bReportForAllPoi);

   void ReportAtAllPointsOfInterest(bool bReportAtAllPoi);
   bool ReportAtAllPointsOfInterest() const;

   virtual std::vector<CGirderKey> GetGirderKeys() const = 0;

protected:
   bool m_bReportAtAllPoi;

private:
   CLoadRatingReportSpecificationBase();
};


// Single girder selection reports
class REPORTINGCLASS CGirderLoadRatingReportSpecification :
   public CLoadRatingReportSpecificationBase, public CGirderReportSpecification
{
public:
   CGirderLoadRatingReportSpecification(const std::_tstring& strReportName, IBroker* pBroker, const CGirderKey& gdrKey, bool bReportForAllPoi);
   ~CGirderLoadRatingReportSpecification(void);

   std::vector<CGirderKey> GetGirderKeys() const override;

protected:

private:
   CGirderLoadRatingReportSpecification();
};

// Single girderline selection reports
class REPORTINGCLASS CGirderLineLoadRatingReportSpecification :
   public CLoadRatingReportSpecificationBase, public CGirderLineReportSpecification
{
public:
   CGirderLineLoadRatingReportSpecification(const std::_tstring& strReportName, IBroker* pBroker, GirderIndexType gdrIdx, bool bReportForAllPoi);
   ~CGirderLineLoadRatingReportSpecification(void);

   std::vector<CGirderKey> GetGirderKeys() const override;

protected:

private:
   CGirderLineLoadRatingReportSpecification();
};

// Multi girder selection reports
class REPORTINGCLASS CMultiGirderLoadRatingReportSpecification :
   public CLoadRatingReportSpecificationBase, public CMultiGirderReportSpecification
{
public:
   CMultiGirderLoadRatingReportSpecification(const std::_tstring& strReportName, IBroker* pBroker, const std::vector<CGirderKey>& gdrKeys, bool bReportForAllPoi);
   ~CMultiGirderLoadRatingReportSpecification(void);

   bool IsSingleGirderLineReport() const;

   std::vector<CGirderKey> GetGirderKeys() const override;

   virtual bool IsValid() const override;
   virtual std::_tstring GetReportContextString() const override;

protected:

private:
   CMultiGirderLoadRatingReportSpecification();
};
