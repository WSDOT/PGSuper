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

#include <Graphing\GirderPropertiesGraphBuilder.h>
#include "GirderGraphControllerBase.h"
#include <MfcTools\WideDropDownComboBox.h>

class CGirderPropertiesGraphController : public CIntervalGirderGraphControllerBase
{
public:
   CGirderPropertiesGraphController();
   DECLARE_DYNCREATE(CGirderPropertiesGraphController);

   bool SetPropertyType(CGirderPropertiesGraphBuilder::PropertyType propertyType);
   CGirderPropertiesGraphBuilder::PropertyType GetPropertyType() const;
   bool IsInvariantProperty(CGirderPropertiesGraphBuilder::PropertyType propertyType) const;

   bool SetSectionPropertyType(pgsTypes::SectionPropertyType type);
   pgsTypes::SectionPropertyType GetSectionPropertyType() const;

   virtual IndexType GetGraphCount();

protected:
   CWideDropDownComboBox m_cbDropList;

   virtual void DoDataExchange(CDataExchange* pDX);
   virtual BOOL OnInitDialog() override;

	//{{AFX_MSG(CGirderPropertiesGraphController)
   afx_msg void OnPropertyChanged();
   afx_msg void OnSectionPropertiesChanged();
   //}}AFX_MSG

	DECLARE_MESSAGE_MAP()

   void FillPropertyCtrl();

   // control variables
   CGirderPropertiesGraphBuilder::PropertyType m_PropertyType;
   pgsTypes::SectionPropertyType m_SectionPropertyType;

   void UpdateSectionPropertyTypeControls();
   int GetSectionPropertyControlID(pgsTypes::SectionPropertyType type);
   pgsTypes::SectionPropertyType GetSectionPropertyType(int nIDC);

   virtual bool ShowBeamBelowGraph() const override { return false; }

#ifdef _DEBUG
public:
   void AssertValid() const;
   void Dump(CDumpContext& dc) const;
#endif //_DEBUG
};