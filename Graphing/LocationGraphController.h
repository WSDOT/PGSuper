///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#include <EAF\EAFGraphControlWindow.h>

// X-Axis Display Type
#define X_AXIS_TIME_LINEAR  0
#define X_AXIS_TIME_LOG     1
#define X_AXIS_INTERVAL      4

class CLocationGraphController : public CEAFGraphControlWindow
{
public:
   CLocationGraphController();
   DECLARE_DYNCREATE(CLocationGraphController);

   virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);

   CGirderKey GetGirderKey();
   pgsPointOfInterest GetLocation();
   int GetXAxisType();

   // if set to TRUE, the location control is always selected to a valid value
   // otherwise and unselected state is acceptable
   void AlwaysSelect(BOOL bAlwaysSelect);
   BOOL AlwaysSelect() const;

protected:
   virtual BOOL OnInitDialog();

   virtual void UpdateGraph() = 0;

	//{{AFX_MSG(CLocationGraphController)
   afx_msg void OnGroupChanged();
   afx_msg void OnGirderChanged();
   afx_msg void OnLocationChanged();
   afx_msg void OnXAxis();
   //}}AFX_MSG

	DECLARE_MESSAGE_MAP()

   void FillGroupCtrl(bool bInit=false);
   void FillGirderCtrl(bool bInit=false);
   void FillLocationCtrl();

   CComPtr<IBroker> m_pBroker;

   CGirderKey m_GirderKey;
   pgsPointOfInterest m_Poi;
   BOOL m_bAlwaysSelect;

#ifdef _DEBUG
public:
   void AssertValid() const;
   void Dump(CDumpContext& dc) const;
#endif //_DEBUG
};