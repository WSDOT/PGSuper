///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#ifndef INCLUDED_TEXASIBNSCHAPTERBUILDER_H_
#define INCLUDED_TEXASIBNSCHAPTERBUILDER_H_

#include <ReportManager\ReportManager.h>
#include <Reporting\PGSuperChapterBuilder.h>

interface IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CTexasIBNSChapterBuilder

   Chapter builder for Texas IBNS sheet


DESCRIPTION
   Chapter builder for Texas IBNS sheet. This is a customized output chapter
   for TxDOT.

COPYRIGHT
   Copyright © 1997-2002
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 08.12.2002 : Created file
*****************************************************************************/

class CTexasIBNSChapterBuilder : public CPGSuperChapterBuilder
{
public:
   // GROUP: LIFECYCLE
   CTexasIBNSChapterBuilder(bool bSelect = true);

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
   CTexasIBNSChapterBuilder(const CTexasIBNSChapterBuilder&);
   CTexasIBNSChapterBuilder& operator=(const CTexasIBNSChapterBuilder&);

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_TEXASIBNSCHAPTERBUILDER_H_
