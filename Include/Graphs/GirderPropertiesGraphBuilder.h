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

#include <Graphs/GraphsExp.h>
#include <Graphs\GirderGraphBuilderBase.h>

class CGirderPropertiesGraphController;

class GRAPHCLASS CGirderPropertiesGraphBuilder : public CGirderGraphBuilderBase
{
public:
   enum PropertyType
   {
      Height,
      Area,
      MomentOfInertia,
      Centroid,
      SectionModulus,
      AreaPrestress,
      KernPoint,
      StrandEccentricity,
      TendonEccentricity,
      TendonProfile,
      EffectiveFlangeWidth,
      Fc,
      Ec,
      PropertyTypeCount // this must always be last
   };

   CGirderPropertiesGraphBuilder();
   CGirderPropertiesGraphBuilder(const CGirderPropertiesGraphBuilder& other);
   virtual ~CGirderPropertiesGraphBuilder();

   virtual int InitializeGraphController(CWnd* pParent,UINT nID) override;
   virtual BOOL CreateGraphController(CWnd* pParent,UINT nID) override;
   virtual std::unique_ptr<WBFL::Graphing::GraphBuilder> Clone() const override;

   virtual void UpdateXAxis() override;

   LPCTSTR GetPropertyLabel(PropertyType propertyType);

   virtual void CreateViewController(IEAFViewController** ppController) override;

   void ExportGraphData(LPCTSTR rstrDefaultFileName);
protected:
   virtual CGirderGraphControllerBase* CreateGraphController() override;
   virtual bool UpdateNow() override;

   DECLARE_MESSAGE_MAP()

   void UpdateYAxisUnits(PropertyType propertyType);

   void UpdateGraphTitle(const CGirderKey& girderKey,IntervalIndexType intervalIdx,PropertyType propertyType);
   void UpdateGraphData(const CGirderKey& girderKey,IntervalIndexType intervalIdx,PropertyType propertType,pgsTypes::SectionPropertyType sectPropType);

   void InitializeGraph(PropertyType propertyType,const CGirderKey& girderKey,IntervalIndexType intervalIdx,IndexType* pGraph1,IndexType* pGraph2, IndexType* pGraph3, IndexType* pGraph4);
   void UpdateTendonGraph(PropertyType propertyType,const CGirderKey& girderKey,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals);

   virtual void GetBeamDrawIntervals(IntervalIndexType* pFirstIntervalIdx, IntervalIndexType* pLastIntervalIdx) override;
};
