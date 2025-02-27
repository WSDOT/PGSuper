///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
#include <Reporter\Chapter.h>
#include <Reporting\PGSuperChapterBuilder.h>


interface IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CLiveLoadDetailsChapterBuilder

   Loading details chapter


DESCRIPTION
   Loading details chapter builder. Details loads applied to the structure.

LOG
   rab : 11.03.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CLiveLoadDetailsChapterBuilder : public CPGSuperChapterBuilder
{
public:
   CLiveLoadDetailsChapterBuilder(bool bDesign,bool bRating,bool bSelect = true);
   virtual LPCTSTR GetName() const override;
   virtual rptChapter* Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const override;
   virtual std::unique_ptr<WBFL::Reporting::ChapterBuilder> Clone() const override;

protected:

private:
   bool m_bDesign, m_bRating;

   // Prevent accidental copying and assignment
   CLiveLoadDetailsChapterBuilder(const CLiveLoadDetailsChapterBuilder&) = delete;
   CLiveLoadDetailsChapterBuilder& operator=(const CLiveLoadDetailsChapterBuilder&) = delete;

   static void ReportLiveLoad(IBroker* pBroker, std::_tstring& load_name, rptParagraph* pPara,IEAFDisplayUnits* pDisplayUnits);
};
