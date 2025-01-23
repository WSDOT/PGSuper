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

#ifndef INCLUDED_CREEPCOEFFICIENTCHAPTERBUILDER_H_
#define INCLUDED_CREEPCOEFFICIENTCHAPTERBUILDER_H_

#include <Reporting\ReportingExp.h>
#include <Reporter\Chapter.h>
#include <Reporting\PGSuperChapterBuilder.h>


interface IEAFDisplayUnits;

class REPORTINGCLASS rptCreepCoefficient : public rptRcScalar
{
public:
   rptCreepCoefficient()
   {
      SetFormat(WBFL::System::NumericFormatTool::Format::Fixed);
      //SetWidth(6);
      SetPrecision(4);
   }
};


/*****************************************************************************
CLASS 
   CCreepCoefficientChapterBuilder

   Creep coefficient chapter builder.


DESCRIPTION
   Creep coefficient chapter builder.  Reports the details of computing the
   creep coefficient.

LOG
   rab : 11.03.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CCreepCoefficientChapterBuilder : public CPGSuperChapterBuilder
{
public:
   // GROUP: LIFECYCLE
   CCreepCoefficientChapterBuilder(bool bSelect = true);

   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   virtual LPCTSTR GetName() const override;
   

   //------------------------------------------------------------------------
   virtual rptChapter* Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const override;

   //------------------------------------------------------------------------
   virtual std::unique_ptr<WBFL::Reporting::ChapterBuilder> Clone() const override;

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
   CCreepCoefficientChapterBuilder(const CCreepCoefficientChapterBuilder&) = delete;
   CCreepCoefficientChapterBuilder& operator=(const CCreepCoefficientChapterBuilder&) = delete;

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_CREEPCOEFFICIENTCHAPTERBUILDER_H_
