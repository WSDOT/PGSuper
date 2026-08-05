///////////////////////////////////////////////////////////////////////
// ExtensionAgentExample - Extension Agent Example Project for PGSuper
// Copyright © 1999-2026  Washington State Department of Transportation
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

#include <Reporting\PGSuperChapterBuilder.h>

/*****************************************************************************
CLASS 
   CMyChapterBuilder

   This is an example chapter builder.


DESCRIPTION
   This is an example chapter builder.

LOG
   rab : 11.25.2009 : Created file
*****************************************************************************/

/// @brief Chapter builder for the example agent's own "Extension Agent Report", added via
/// WBFL::ReportMgr::ReportBuilder::AddChapterBuilder. Note this example only adds a chapter to a
/// brand-new report - see \ref creating_an_extension_agent "Creating an Extension Agent" for how
/// InsertChapterBuilder/RemoveChapterBuilder are used to modify an *existing* report instead.
class CMyChapterBuilder : public CPGSuperChapterBuilder
{
public:
   CMyChapterBuilder(bool bSelect = true);

   virtual LPCTSTR GetName() const override;
   virtual rptChapter* Build(const std::shared_ptr<const WBFL::ReportMgr::ReportSpecification>& pRptSpec,Uint16 level) const override;
};
