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

#ifndef INCLUDED_CRITSECTIONCHAPTERBUILDER_H_
#define INCLUDED_CRITSECTIONCHAPTERBUILDER_H_

#include <Reporting\ReportingExp.h>
#include <Reporter\Chapter.h>
#include <Reporting\PGSuperChapterBuilder.h>

interface IEAFDisplayUnits;


/*****************************************************************************
CLASS 
   CCritSectionChapterBuilder

   Critical Section For Shear Details Chapter Builder.


DESCRIPTION
   Reports the details of the location of the critical section for shear
   calculation.

LOG
   rdp : 02.07.1999 : Created file
*****************************************************************************/

class REPORTINGCLASS CCritSectionChapterBuilder : public CPGSuperChapterBuilder
{
public:
   // GROUP: LIFECYCLE
   CCritSectionChapterBuilder(bool bDesign,bool bRating,bool bSelect = true);

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
   bool m_bDesign;
   bool m_bRating;

   // GROUP: LIFECYCLE
   void Build(rptChapter* pChapter,pgsTypes::LimitState limitState,IBroker* pBroker,const CGirderKey& girderKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level) const;

   // Prevent accidental copying and assignment
   CCritSectionChapterBuilder(const CCritSectionChapterBuilder&) = delete;
   CCritSectionChapterBuilder& operator=(const CCritSectionChapterBuilder&) = delete;

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_CRITSECTIONCHAPTERBUILDER_H_
