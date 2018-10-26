///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#include <Graphing\GraphingExp.h>
#include <Graphing\GirderGraphBuilderBase.h>

class CGirderPropertiesGraphController;

class GRAPHINGCLASS CGirderPropertiesGraphBuilder : public CGirderGraphBuilderBase
{
public:
   enum PropertyType
   {
      Height,
      Area,
      MomentOfInertia,
      YTop,
      YBottom,
      TopSectionModulus,
      BottomSectionModulus,
      TopKernPoint,
      BottomKernPoint,
      StrandEccentricity,
      TendonEccentricity,
      TendonProfile,
      EffectiveFlangeWidth,
      Fc,
      PropertyTypeCount // this must always be last
   };

   CGirderPropertiesGraphBuilder();
   CGirderPropertiesGraphBuilder(const CGirderPropertiesGraphBuilder& other);
   virtual ~CGirderPropertiesGraphBuilder();

   virtual int CreateControls(CWnd* pParent,UINT nID);
   virtual CGraphBuilder* Clone();

   LPCTSTR GetPropertyLabel(PropertyType propertyType);

protected:
   virtual CGirderGraphControllerBase* CreateGraphController();
   virtual BOOL InitGraphController(CWnd* pParent,UINT nID);
   virtual bool UpdateNow();

   DECLARE_MESSAGE_MAP()

   void UpdateYAxisUnits(PropertyType propertyType);

   void UpdateGraphTitle(GroupIndexType grpIdx,GirderIndexType gdrIdx,IntervalIndexType intervalIdx,PropertyType propertyType);
   void UpdateGraphData(GroupIndexType grpIdx,GirderIndexType gdrIdx,IntervalIndexType intervalIdx,PropertyType propertType,pgsTypes::SectionPropertyType sectPropType);

   IndexType InitializeGraph(PropertyType propertyType);
   void UpdateTendonGraph(PropertyType propertyType,const CGirderKey& girderKey,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,const std::vector<Float64>& xVals);
};
