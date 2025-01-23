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

#include <PgsExt\PgsExtExp.h>

#define PIERTYPE_START 0
#define PIERTYPE_INTERMEDIATE 1
#define PIERTYPE_END 2

// Usage notes:
// Add the following members to your dialog
// CBoundaryConditionComboBox m_cbBoundaryCondition
// pgsTypes::BoundaryConditionType m_BoundaryCondition
//
// In DoDataExchange, add the following
//   DDX_Control(pDX,IDC_BOUNDARY_CONDITION,m_cbBoundaryCondition);
//   DDX_CBItemData(pDX, IDC_BOUNDARY_CONDITION, m_BoundaryCondition);


class PGSEXTCLASS CBoundaryConditionComboBox : public CComboBox
{
public:
   CBoundaryConditionComboBox();
   
   void Initialize(bool bIsBoundaryPier,const std::vector<pgsTypes::BoundaryConditionType>& connections,bool bNoDeck=false);
   void SetPierType(int pierType);

   virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) override;

private:
   int AddBoundaryCondition(pgsTypes::BoundaryConditionType type);
   int m_PierType;
   bool m_bIsBoundaryPier;
   bool m_bNoDeck;
public:
   DECLARE_MESSAGE_MAP()
   afx_msg void OnCbnDropdown();
};
