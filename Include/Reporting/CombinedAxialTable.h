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
#include <Reporting\ReportNotes.h>
#include <IFace\Project.h>

/*****************************************************************************
CLASS 
   CCombinedAxialTable

   Encapsulates the construction of the combined axial table.


DESCRIPTION
   Encapsulates the construction of the combined axial table.

LOG
   rab : 11.08.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CCombinedAxialTable
{
public:
   CCombinedAxialTable();
   CCombinedAxialTable(const CCombinedAxialTable& rOther);
   virtual ~CCombinedAxialTable();

   CCombinedAxialTable& operator = (const CCombinedAxialTable& rOther);

   //------------------------------------------------------------------------
   // Builds the combined results table
   // bDesign and bRating are only considered for intervalIdx = live load interval index
   virtual void Build(IBroker* pBroker, rptChapter* pChapter,
                      const CGirderKey& girderKey,
                      IEAFDisplayUnits* pDisplayUnits,
                      IntervalIndexType intervalIdx,pgsTypes::AnalysisType analysisType,
                      bool bDesign,bool bRating) const;
protected:
   void BuildCombinedDeadTable(IBroker* pBroker, rptChapter* pChapter,
                      const CGirderKey& girderKey,
                      IEAFDisplayUnits* pDisplayUnits,
                      IntervalIndexType intervalIdx,pgsTypes::AnalysisType analysisType,
                      bool bDesign,bool bRating) const;

   void BuildCombinedLiveTable(IBroker* pBroker, rptChapter* pChapter,
                      const CGirderKey& girderKey,
                      IEAFDisplayUnits* pDisplayUnits,
                      pgsTypes::AnalysisType analysisType,
                      bool bDesign,bool bRating) const;

   void BuildLimitStateTable(IBroker* pBroker, rptChapter* pChapter,
                      const CGirderKey& girderKey,
                      IEAFDisplayUnits* pDisplayUnits,IntervalIndexType intervalIdx,
                      pgsTypes::AnalysisType analysisType,
                      bool bDesign,bool bRating) const;

   void MakeCopy(const CCombinedAxialTable& rOther);

   void MakeAssignment(const CCombinedAxialTable& rOther);
};
