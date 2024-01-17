///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
#include <IFace\ExtendUI.h>


/*****************************************************************************
CLASS 
   CCopyTempSupportPropertiesChapterBuilder

   Chapter builder for reporting construction event CopyTempSupportProperties
*****************************************************************************/

class REPORTINGCLASS CCopyTempSupportPropertiesChapterBuilder : public CPGSuperChapterBuilder
{
public:
   CCopyTempSupportPropertiesChapterBuilder(bool bSelect = true);

   //------------------------------------------------------------------------
   virtual LPCTSTR GetName() const override;
   

   //------------------------------------------------------------------------
   virtual rptChapter* Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const override;

   //------------------------------------------------------------------------
   virtual std::unique_ptr<WBFL::Reporting::ChapterBuilder> Clone() const override;

   //================================
   // special function to work with control in copy per dialog
   void SetCopyTempSupportProperties(std::vector<ICopyTemporarySupportPropertiesCallback*>& rCallBacks, PierIndexType fromTempSupportIdx, const std::vector<PierIndexType>& toTempSupports);

protected:
   std::vector<ICopyTemporarySupportPropertiesCallback*> m_CallBacks;
   PierIndexType m_FromTempSupportIdx;
   std::vector<PierIndexType> m_ToTempSupports;

private:
   // Prevent accidental copying and assignment
   CCopyTempSupportPropertiesChapterBuilder(const CCopyTempSupportPropertiesChapterBuilder&) = delete;
   CCopyTempSupportPropertiesChapterBuilder& operator=(const CCopyTempSupportPropertiesChapterBuilder&) = delete;
};
