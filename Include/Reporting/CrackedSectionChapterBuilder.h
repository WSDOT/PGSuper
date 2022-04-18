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

// The moment Capacity Details report was contributed by BridgeSight Inc.

#pragma once
#include <Reporting\PGSuperChapterBuilder.h>

#include <atlimage.h>

#include <WBFLRCCapacity.h>

#include <GraphicsLib\PointMapper.h>

class REPORTINGCLASS CCrackedSectionChapterBuilder :
   public CPGSuperChapterBuilder
{
public:
   CCrackedSectionChapterBuilder(bool bSelect = true);
   ~CCrackedSectionChapterBuilder(void);

   virtual LPCTSTR GetName() const;
   virtual rptChapter* Build(CReportSpecification* pRptSpec,Uint16 level) const;
   virtual CChapterBuilder* Clone() const;

private:
   rptRcImage* CreateImage(ICrackedSectionSolution* pSolution,bool bPositiveMoment) const;
   void DrawSection(CImage& image,ICrackedSectionSolution* pSolution,bool bPositiveMoment) const;
   void DrawSlice(IShape* pShape,CDC* pDC,grlibPointMapper& mapper) const;

   // This is a list of temporary files that were created on the fly
   // Delete them in the destructor
   std::vector<std::_tstring> m_TemporaryImageFiles;
};
