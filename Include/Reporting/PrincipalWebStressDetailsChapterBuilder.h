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
#include <Reporter\Chapter.h>
#include <Reporting\PGSuperChapterBuilder.h>

#include <IFace\AnalysisResults.h>

struct TIME_STEP_DETAILS;
interface IIntervals;
interface IMaterials;


/*****************************************************************************
CLASS 
   CPrincipalWebStressDetailsChapterBuilder

DESCRIPTION
   Chapter builder for reporting details of time-step analysis calculations
   at a specified POI
*****************************************************************************/

class REPORTINGCLASS CPrincipalWebStressDetailsChapterBuilder : public CPGSuperChapterBuilder
{
public:
   CPrincipalWebStressDetailsChapterBuilder(bool bSelect = true);

   //------------------------------------------------------------------------
   virtual LPCTSTR GetName() const;
   

   //------------------------------------------------------------------------
   virtual rptChapter* Build(CReportSpecification* pRptSpec,Uint16 level) const;

   //------------------------------------------------------------------------
   virtual CChapterBuilder* Clone() const;

protected:
   void BuildIncrementalStressTables(rptChapter* pChapter, IBroker* pBroker, IntervalIndexType intervalIdx, PoiList vPoi, const std::vector<pgsTypes::ProductForceType>& vLoads, IEAFDisplayUnits* pDisplayUnits) const;
   void BuildLiveLoadStressTable(rptChapter* pChapter, IBroker* pBroker, IntervalIndexType intervalIdx, PoiList vPoi, IEAFDisplayUnits* pDisplayUnits) const;
   void BuildCombinedStressTables(rptChapter* pChapter, IBroker* pBroker, IntervalIndexType intervalIdx, PoiList vPoi, IEAFDisplayUnits* pDisplayUnits) const;


   // Prevent accidental copying and assignment
   CPrincipalWebStressDetailsChapterBuilder(const CPrincipalWebStressDetailsChapterBuilder&) = delete;
   CPrincipalWebStressDetailsChapterBuilder& operator=(const CPrincipalWebStressDetailsChapterBuilder&) = delete;

   mutable bool m_bReportShear;
   mutable bool m_bReportAxial;
};
