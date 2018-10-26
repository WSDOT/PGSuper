///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#include <Graphing\InfluenceLineGraphBuilder.h>
#include <Graphing\GirderGraphControllerBase.h>

class CInfluenceLineGraphController : public CGirderGraphControllerBase
{
public:
   CInfluenceLineGraphController();
   DECLARE_DYNCREATE(CInfluenceLineGraphController);

   virtual IndexType GetGraphCount();

   CInfluenceLineGraphBuilder::GraphType GetGraphType();

   // called by the framework when the view's OnUpdate method is called
   virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);

protected:

   virtual BOOL OnInitDialog();

   void FillGraphTypeControl();

   virtual void OnGirderChanged();

	//{{AFX_MSG(CInfluenceLineGraphController)
   //}}AFX_MSG

	DECLARE_MESSAGE_MAP()

#ifdef _DEBUG
public:
   void AssertValid() const;
   void Dump(CDumpContext& dc) const;
#endif //_DEBUG
};