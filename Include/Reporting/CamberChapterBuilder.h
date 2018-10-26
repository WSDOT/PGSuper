///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#ifndef INCLUDED_CAMBERCHAPTERBUILDER_H_
#define INCLUDED_CAMBERCHAPTERBUILDER_H_

#include <Reporting\ReportingExp.h>
#include <Reporter\Chapter.h>
#include <Reporting\PGSuperChapterBuilder.h>


interface IDisplayUnits;


/*****************************************************************************
CLASS 
   CCamberChapterBuilder

   Camber Chapter Builder.


DESCRIPTION
   Camber Chapter Builder. Reports the camber and the equations used to 
   computer camber.

COPYRIGHT
   Copyright © 1997-1999
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 03.17.1999 : Created file
*****************************************************************************/

class REPORTINGCLASS CCamberChapterBuilder : public CPGSuperChapterBuilder
{
public:
   // GROUP: LIFECYCLE
   CCamberChapterBuilder();

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

   rptChapter* Build_CIP_TempStrands(   CReportSpecification* pRptSpec,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IDisplayUnits* pDispUnits,Uint16 level) const;
   rptChapter* Build_CIP(               CReportSpecification* pRptSpec,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IDisplayUnits* pDispUnits,Uint16 level) const;
   rptChapter* Build_SIP_TempStrands(   CReportSpecification* pRptSpec,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IDisplayUnits* pDispUnits,Uint16 level) const;
   rptChapter* Build_SIP(               CReportSpecification* pRptSpec,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IDisplayUnits* pDispUnits,Uint16 level) const;
   rptChapter* Build_NoDeck_TempStrands(CReportSpecification* pRptSpec,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IDisplayUnits* pDispUnits,Uint16 level) const;
   rptChapter* Build_NoDeck(            CReportSpecification* pRptSpec,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IDisplayUnits* pDispUnits,Uint16 level) const;

   // Prevent accidental copying and assignment
   CCamberChapterBuilder(const CCamberChapterBuilder&);
   CCamberChapterBuilder& operator=(const CCamberChapterBuilder&);

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_CAMBERCHAPTERBUILDER_H_
