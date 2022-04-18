///////////////////////////////////////////////////////////////////////
// ExtensionAgentExample - Extension Agent Example Project for PGSuper
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

#define SINE_GRAPH 1
#define COSINE_GRAPH 2

#include <EAF\EAFGraphControlWindow.h>

class CTestDialogBar : public CEAFGraphControlWindow
{
public:
   CTestDialogBar();
   DECLARE_DYNCREATE(CTestDialogBar);

   int GetGraphType();

protected:

   virtual BOOL OnInitDialog() override;

	//{{AFX_MSG(CTestDialogBar)
   //}}AFX_MSG
   afx_msg void OnButton();

	DECLARE_MESSAGE_MAP()

#ifdef _DEBUG
public:
   virtual void AssertValid() const override;
   virtual void Dump(CDumpContext& dc) const override;
#endif //_DEBUG
};