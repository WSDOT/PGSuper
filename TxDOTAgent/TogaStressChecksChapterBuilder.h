///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#ifndef INCLUDED_TogaStressChecksChapterBuilder_H_
#define INCLUDED_TogaStressChecksChapterBuilder_H_

#include <Reporter\Chapter.h>
#include <ReportManager\ReportManager.h>
#include <Reporting\PGSuperChapterBuilder.h>

interface IEAFDisplayUnits;
class pgsSegmentArtifact;

/*****************************************************************************
CLASS 
   CTogaStressChecksChapterBuilder

   Chapter builder TxDOT stress cheks


DESCRIPTION
   Chapter builder TxDOT stress cheks

LOG
   rdp : 06.15.2006 : Created file
*****************************************************************************/

class CTogaStressChecksChapterBuilder : public CPGSuperChapterBuilder
{
public:
   // GROUP: LIFECYCLE
   CTogaStressChecksChapterBuilder();

   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   virtual LPCTSTR GetName() const override;

   //------------------------------------------------------------------------
   virtual rptChapter* Build(CReportSpecification* pRptSpec,Uint16 level) const override;

   //------------------------------------------------------------------------
   virtual CChapterBuilder* Clone() const override;

   //------------------------------------------------------------------------
   void BuildTableAndNotes(rptChapter* pChapter, IBroker* pBroker,
                      IEAFDisplayUnits* pDisplayUnits,
                      IntervalIndexType intervalIdx,
                      pgsTypes::LimitState ls,
                      pgsTypes::StressType stress=pgsTypes::Tension) const;

   //------------------------------------------------------------------------
   void BuildTable(rptChapter* pChapter, IBroker* pBroker,
                      const pgsSegmentArtifact* pFactoredGdrArtifact, const pgsSegmentArtifact* pUnfactoredGdrArtifact,
                      IEAFDisplayUnits* pDisplayUnits,
                      IntervalIndexType intervalIdx,
                      pgsTypes::LimitState ls,
                      pgsTypes::StressType stress=pgsTypes::Tension) const;

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE

   // Prevent accidental copying and assignment
   CTogaStressChecksChapterBuilder(const CTogaStressChecksChapterBuilder&) = delete;
   CTogaStressChecksChapterBuilder& operator=(const CTogaStressChecksChapterBuilder&) = delete;

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_SPECCHECKCHAPTERBUILDER_H_
