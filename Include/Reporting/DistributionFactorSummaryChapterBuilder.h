///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

#ifndef INCLUDED_DISTRIBUTIONFactorSummaryCHAPTERBUILDER_H_
#define INCLUDED_DISTRIBUTIONFactorSummaryCHAPTERBUILDER_H_

#include <Reporting\ReportingExp.h>
#include <Lrfd\ILiveLoadDistributionFactor.h>
#include <Reporter\Chapter.h>
#include <Reporting\PGSuperChapterBuilder.h>



/*****************************************************************************
CLASS 
   CDistributionFactorSummaryChapterBuilder

   Distribution Factor Calculations details Chapter Builder.


DESCRIPTION
   Reports the details of Distribution Factor Calculations details

LOG
   rab : 10.19.1999 : Created file
*****************************************************************************/

class REPORTINGCLASS CDistributionFactorSummaryChapterBuilder : public CPGSuperChapterBuilder
{
public:
   // GROUP: LIFECYCLE
   CDistributionFactorSummaryChapterBuilder(bool bSelect = true);

   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   virtual LPCTSTR GetName() const;
   

   //------------------------------------------------------------------------
   virtual rptChapter* Build(CReportSpecification* pRptSpec,Uint16 level) const;

   //------------------------------------------------------------------------
   virtual CChapterBuilder* Clone() const;

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
   CDistributionFactorSummaryChapterBuilder(const CDistributionFactorSummaryChapterBuilder&) = delete;
   CDistributionFactorSummaryChapterBuilder& operator=(const CDistributionFactorSummaryChapterBuilder&) = delete;

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_DISTRIBUTIONFactorSummaryCHAPTERBUILDER_H_
