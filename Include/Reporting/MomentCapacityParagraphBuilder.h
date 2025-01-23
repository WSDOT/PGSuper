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

#ifndef INCLUDED_MOMENTCAPACITYPARAGRAPHBUILDER_H_
#define INCLUDED_MOMENTCAPACITYPARAGRAPHBUILDER_H_

#include <Reporting\ReportingExp.h>

interface IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CMomentCapacityParagraphBuilder

   Paragraph builder for moment capacity summary


DESCRIPTION
   Paragraph builder for moment capacity summary

LOG
   rdp : 06.13.2006 : Created file
*****************************************************************************/

class REPORTINGCLASS CMomentCapacityParagraphBuilder
{
public:
   // GROUP: LIFECYCLE
   CMomentCapacityParagraphBuilder();

   // GROUP: OPERATORS
   // GROUP: OPERATIONS


   //------------------------------------------------------------------------
   rptParagraph* Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const;

   //------------------------------------------------------------------------

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
   CMomentCapacityParagraphBuilder(const CMomentCapacityParagraphBuilder&) = delete;
   CMomentCapacityParagraphBuilder& operator=(const CMomentCapacityParagraphBuilder&) = delete;

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_TEXASIBNSParagraphBUILDER_H_
