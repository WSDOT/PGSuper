///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
#include <Reporter\Chapter.h>
#include <Reporting\PGSuperChapterBuilder.h>


interface IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CLRFDCreepCoefficientChapterBuilder

   Reports creep coefficient calculation details for the creep equations
   in the AASHTO LRFD Bridge Design Specification
*****************************************************************************/

class REPORTINGCLASS CLRFDCreepCoefficientChapterBuilder : public CPGSuperChapterBuilder
{
public:
   CLRFDCreepCoefficientChapterBuilder(bool bSelect=true);

   virtual LPCTSTR GetName() const override;
   virtual rptChapter* Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const override;
   virtual std::unique_ptr<WBFL::Reporting::ChapterBuilder> Clone() const override;

private:
   // Prevent accidental copying and assignment
   CLRFDCreepCoefficientChapterBuilder(const CLRFDCreepCoefficientChapterBuilder&) = delete;
   CLRFDCreepCoefficientChapterBuilder& operator=(const CLRFDCreepCoefficientChapterBuilder&) = delete;

   rptParagraph* Build_CIP_TempStrands(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level) const;
   rptParagraph* Build_CIP(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level) const;
   rptParagraph* Build_SIP_TempStrands(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level) const;
   rptParagraph* Build_SIP(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level) const;
   rptParagraph* Build_NoDeck_TempStrands(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level) const;
   rptParagraph* Build_NoDeck(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level) const;
};
