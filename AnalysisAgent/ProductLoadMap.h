///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#include <IFace\AnalysisResults.h>

// Mapping between pgsTypes::ProductForceType contants and consistent human-readable name
class CProductLoadMap
{
public:
   CProductLoadMap();
   pgsTypes::ProductForceType GetProductForceType(CComBSTR bstrLoadGroupName) const;
   CComBSTR GetGroupLoadName(pgsTypes::ProductForceType pfType) const;
   LoadCaseIDType GetLoadCaseID(pgsTypes::ProductForceType pfType) const;
   LoadCaseIDType GetMaxLoadCaseID() const;

   static std::vector<pgsTypes::ProductForceType> GetProductForces(IBroker* pBroker,LoadingCombinationType combo);

private:
   void AddLoadItem(pgsTypes::ProductForceType pfType,CComBSTR bstrLoadGroupName,LoadCaseIDType lcid);

   std::map<pgsTypes::ProductForceType,CComBSTR> m_ProductForceTypeToLoadName;
   std::map<CComBSTR,pgsTypes::ProductForceType> m_LoadNameToProductForceType;
   std::map<pgsTypes::ProductForceType,LoadCaseIDType> m_ProductForceTypeToLoadCaseID;
   LoadCaseIDType m_LoadCaseID;
};
