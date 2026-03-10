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

#ifndef INCLUDED_FLEXURALSTRESSCHECKTABLE_H_
#define INCLUDED_FLEXURALSTRESSCHECKTABLE_H_

#include <Reporting\ReportingExp.h>

class IEAFDisplayUnits;
class pgsGirderArtifact;
class pgsSegmentArtifact;
struct StressCheckTask;

/*****************************************************************************
CLASS 
   CFlexuralStressCheckTable

   Encapsulates the construction of the flexural stress check table.


DESCRIPTION
   Encapsulates the construction of the flexural stress check table.

LOG
   rab : 11.13.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CFlexuralStressCheckTable
{
public:
   CFlexuralStressCheckTable();
   CFlexuralStressCheckTable(const CFlexuralStressCheckTable& rOther);
   virtual ~CFlexuralStressCheckTable();

   CFlexuralStressCheckTable& operator = (const CFlexuralStressCheckTable& rOther);

   virtual void Build(rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsGirderArtifact* pGirderArtifact, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const StressCheckTask& task, bool bGirderStresses ) const;

   //------------------------------------------------------------------------
   // Builds the stress table and notes
   virtual void Build(rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsGirderArtifact* pGirderArtifact, SegmentIndexType segIdx, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const StressCheckTask& task, bool bGirderStresses ) const;

   //------------------------------------------------------------------------
   // Builds the notes above stress table
   void BuildNotes(rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsGirderArtifact* pGirderArtifact, SegmentIndexType segIdx, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const StressCheckTask& task, bool bGirderStresses) const;

   //------------------------------------------------------------------------
   // Builds the table only
   void BuildTable(rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsGirderArtifact* pGirderArtifact, SegmentIndexType segIdx, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const StressCheckTask& task, bool bGirderStresses) const;

protected:
   void MakeCopy(const CFlexuralStressCheckTable& rOther);
   void MakeAssignment(const CFlexuralStressCheckTable& rOther);


   void BuildSectionHeading(rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsGirderArtifact* pGirderArtifact, SegmentIndexType segIdx, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const StressCheckTask& task, bool bGirderStresses) const;

   void BuildStressLimitInformation(rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsGirderArtifact* pGirderArtifact, SegmentIndexType segIdx, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const StressCheckTask& task, bool bGirderStresses) const;

   void BuildGirderStressLimitInformation(rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsGirderArtifact* pGirderArtifact, SegmentIndexType segIdx, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,const StressCheckTask& task) const;

   void BuildDeckStressLimitInformation(rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsGirderArtifact* pGirderArtifact, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const StressCheckTask& task) const;

   void BuildSegmentStressLimitInformation(rptParagraph* pPara, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsSegmentArtifact* pSegmentArtifact, IndexType artifactIdx, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const StressCheckTask& task) const;

   void BuildClosureJointStressLimitInformation(rptParagraph* pPara, std::shared_ptr<WBFL::EAF::Broker> pBroker, const pgsSegmentArtifact* pSegmentArtifact, IndexType artifactIdx, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits, const StressCheckTask& task) const;
};

#endif // INCLUDED_FLEXURALSTRESSCHECKTABLE_H_
