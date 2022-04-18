///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#ifndef INCLUDED_MVRCHAPTERBUILDER_H_
#define INCLUDED_MVRCHAPTERBUILDER_H_

#include <Reporting\ReportingExp.h>
#include <Reporter\Chapter.h>
#include <Reporting\PGSuperChapterBuilder.h>


/*****************************************************************************
CLASS 
   CMVRChapterBuilder

   Chapter builder moments, shears, and reactions.


DESCRIPTION
   Chapter builder moments, shears, and reactions.

LOG
   rab : 11.04.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CMVRChapterBuilder : public CPGSuperChapterBuilder
{
public:
   CMVRChapterBuilder(bool bDesign,bool bRating,bool bSelect = true);

   //------------------------------------------------------------------------
   virtual LPCTSTR GetName() const;

   //------------------------------------------------------------------------
   virtual rptChapter* Build(CReportSpecification* pRptSpec,Uint16 level) const;

   //------------------------------------------------------------------------
   virtual CChapterBuilder* Clone() const;

private:
   bool m_bDesign;
   bool m_bRating;

   // Prevent accidental copying and assignment
   CMVRChapterBuilder(const CMVRChapterBuilder&) = delete;
   CMVRChapterBuilder& operator=(const CMVRChapterBuilder&) = delete;
};

#endif // INCLUDED_MVRCHAPTERBUILDER_H_
