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

#pragma once


#include <Reporter\Chapter.h>
#include <Reporting\PGSuperChapterBuilder.h>
#include <GeomModel/ShapeProperties.h>

class REPORTINGCLASS CSectionPropertiesChapterBuilder : public CPGSuperChapterBuilder
{
public:

   typedef  std::vector<std::pair<Float64, Float64>> Points2D;

   CSectionPropertiesChapterBuilder(bool bSelect = true,bool simplifiedVersion=false);

   LPCTSTR GetName() const override;

   rptChapter* Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const override;

private:
   bool m_SimplifiedVersion;

   // This is a list of temporary files that were created on the fly
// Delete them in the destructor
   mutable std::vector<std::_tstring> m_TemporaryFiles;

   void ExportCoordinatesToExcel(
       const std::vector<std::vector<std::pair<Float64, Float64>>>& primaryPoints,
       const std::_tstring& outputFilePath) const;

   rptRcTable* WriteXSTable2(std::shared_ptr<WBFL::EAF::Broker> pBroker,
       pgsTypes::SectionPropertyType spType,
       const pgsPointOfInterest& poi,
       IntervalIndexType intervalIdx,
       std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;

   void WriteSectionProperties(rptParagraph& para, CComPtr<IShapeProperties>& shapeProps) const;

   typedef enum SteelPropType { Straight, Harped, Temporary, Rebar, DeckRebar, Tendon, Duct } SteelPropType;

   struct SteelProps
   {
	   SteelPropType Type;
       Float64 X;
       Float64 Y;
   };

   rptRcImage* CreateImage(const std::vector<Points2D>& primaryPoints, 
       const Points2D& secondaryPoints,
       const std::vector<SteelProps>& steelProps,
       const std::pair<Float64, Float64>& cg) const;

};
