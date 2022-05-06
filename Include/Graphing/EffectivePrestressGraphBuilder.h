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

#pragma once

#include <Graphing\GraphingExp.h>
#include <Graphing\GirderGraphBuilderBase.h>

class GRAPHINGCLASS CEffectivePrestressGraphBuilder : public CGirderGraphBuilderBase
{
public:
   enum DuctType { Segment, Girder };

   CEffectivePrestressGraphBuilder();
   CEffectivePrestressGraphBuilder(const CEffectivePrestressGraphBuilder& other);
   virtual ~CEffectivePrestressGraphBuilder();

   virtual BOOL CreateGraphController(CWnd* pParent,UINT nID) override;
   virtual CGraphBuilder* Clone() const override;

   virtual void CreateViewController(IEAFViewController** ppController) override;

   virtual void UpdateXAxis() override;
   virtual void UpdateYAxis() override;

   void ExportGraphData(LPCTSTR rstrDefaultFileName);

protected:
   virtual CGirderGraphControllerBase* CreateGraphController() override;
   virtual bool UpdateNow() override;

   DECLARE_MESSAGE_MAP()

   void UpdateGraphTitle(GroupIndexType grpIdx,GirderIndexType gdrIdx, DuctType ductType,DuctIndexType ductIdx);
   void UpdateGraphData(GroupIndexType grpIdx,GirderIndexType gdrIdx, DuctType ductType, DuctIndexType ductIdx);
   void UpdatePosttensionGraphData(GroupIndexType grpIdx,GirderIndexType gdrIdx, DuctType ductType, DuctIndexType ductIdx);
   void UpdatePretensionGraphData(GroupIndexType grpIdx,GirderIndexType gdrIdx);

   virtual void GetBeamDrawIntervals(IntervalIndexType* pFirstIntervalIdx, IntervalIndexType* pLastIntervalIdx) override;
};
