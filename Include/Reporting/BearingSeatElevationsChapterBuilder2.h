///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include <Reporting\PGSuperChapterBuilder.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>

// Base class for bearing seat elevations and bearing deduct chapters
/////////////////////////////////////////////////////////////////////////
class REPORTINGCLASS CBearingSeatElevationsChapterBuilderBase :
   public CPGSuperChapterBuilder
{
public:
   enum TableType { ttBearingElevations, ttBearingDeduct };

   CBearingSeatElevationsChapterBuilderBase(TableType type, bool bSelect = true);
   ~CBearingSeatElevationsChapterBuilderBase(void);

   virtual rptChapter* Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const override;

private:
   rptRcTable* CBearingSeatElevationsChapterBuilderBase::BuildTable(const CString& strLabel, PierIndexType pierIdx, pgsTypes::PierFaceType face, 
                           IEAFDisplayUnits* pDisplayUnits, IBridge* pBridge, IBridgeDescription* pIBridgeDesc, GirderIndexType girderIndex) const;

   rptRcTable* CBearingSeatElevationsChapterBuilderBase::BuildGirderEdgeTable(const CString& strLabel, PierIndexType pierIdx, pgsTypes::PierFaceType face, 
                           IEAFDisplayUnits* pDisplayUnits, IBridge* pBridge, IBridgeDescription* pIBridgeDesc,GirderIndexType girderIndex) const;

   CBearingSeatElevationsChapterBuilderBase();
   TableType m_TableType;
};

// Specialized class for bearing seat elevations chapter
/////////////////////////////////////////////////////////////////////////
class REPORTINGCLASS CBearingSeatElevationsChapterBuilder2 :
   public CBearingSeatElevationsChapterBuilderBase
{
public:
   CBearingSeatElevationsChapterBuilder2(bool bSelect = true);
   ~CBearingSeatElevationsChapterBuilder2(void);

   virtual LPCTSTR GetName() const override;
   virtual std::unique_ptr<WBFL::Reporting::ChapterBuilder> Clone() const override;
};

// Specialized class for bearing deduct chapter
/////////////////////////////////////////////////////////////////////////
class REPORTINGCLASS CBearingDeductChapterBuilder :
   public CBearingSeatElevationsChapterBuilderBase
{
public:
   CBearingDeductChapterBuilder(bool bSelect = true);
   ~CBearingDeductChapterBuilder(void);

   virtual LPCTSTR GetName() const override;
   virtual std::unique_ptr<WBFL::Reporting::ChapterBuilder> Clone() const override;
};
