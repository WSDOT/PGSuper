///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#include "stdafx.h"
#include "ProductLoadMap.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>

CProductLoadMap::CProductLoadMap()
{
   // These are the load names used in the LBAM model. Mapping the
   // ProductForceType to a consistent string makes it easier to 
   // avoid issues with using the wrong, or misspelled, name in the LBAM
   m_LoadCaseID = 0;

   AddLoadItem(pftGirder,                  _T("Girder"),        m_LoadCaseID++);
   AddLoadItem(pftDiaphragm,               _T("Diaphragm"),     m_LoadCaseID++);
   AddLoadItem(pftConstruction,            _T("Construction"),  m_LoadCaseID++);
   AddLoadItem(pftSlab,                    _T("Slab"),          m_LoadCaseID++);
   AddLoadItem(pftSlabPad,                 _T("Haunch"),        m_LoadCaseID++);
   AddLoadItem(pftSlabPanel,               _T("Slab Panel"),    m_LoadCaseID++);
   AddLoadItem(pftOverlay,                 _T("Overlay"),       m_LoadCaseID++);
   AddLoadItem(pftOverlayRating,           _T("Overlay Rating"),  m_LoadCaseID++);
   AddLoadItem(pftTrafficBarrier,          _T("Traffic Barrier"), m_LoadCaseID++);
   AddLoadItem(pftSidewalk,                _T("Sidewalk"),        m_LoadCaseID++);
   AddLoadItem(pftUserDC,                  _T("UserDC"),          m_LoadCaseID++);
   AddLoadItem(pftUserDW,                  _T("UserDW"),          m_LoadCaseID++);
   AddLoadItem(pftUserLLIM,                _T("UserLLIM"),        m_LoadCaseID++);
   AddLoadItem(pftShearKey,                _T("Shear Key"),       m_LoadCaseID++);
   //AddLoadItem(pftPretension,              _T("Pretensioning"),   m_LoadCaseID++); // not modeled in the LBAM
   AddLoadItem(pftEquivPostTensioning,     _T("Equiv Post Tensioning"), m_LoadCaseID++);
   //AddLoadItem(pftPrimaryPostTensioning, _T("Primary Post Tensioning"), m_LoadCaseID++); // not modeled in the LBAM
   //AddLoadItem(pftSecondaryEffects,      _T("Secondary Effects"), m_LoadCaseID++); // not modeled in the LBAM
   AddLoadItem(pftCreep,                   _T("Creep"),         m_LoadCaseID++);
   AddLoadItem(pftShrinkage,               _T("Shrinkage"),     m_LoadCaseID++);
   AddLoadItem(pftRelaxation,              _T("Relaxation"),    m_LoadCaseID++);
}

ProductForceType CProductLoadMap::GetProductForceType(CComBSTR bstrName)
{
   std::map<CComBSTR,ProductForceType>::iterator found( m_LoadNameToProductForceType.find(bstrName) );
   if ( found == m_LoadNameToProductForceType.end() )
   {
      ATLASSERT(false);
      return pftGirder;
   }
   else
   {
      return found->second;
   }
}

CComBSTR CProductLoadMap::GetGroupLoadName(ProductForceType pfType)
{
   std::map<ProductForceType,CComBSTR>::iterator found( m_ProductForceTypeToLoadName.find(pfType) );
   if ( found == m_ProductForceTypeToLoadName.end() )
   {
      ATLASSERT(false);
      return CComBSTR();
   }
   else
   {
      return found->second;
   }
}

LoadCaseIDType CProductLoadMap::GetLoadCaseID(ProductForceType pfType)
{
   std::map<ProductForceType,LoadCaseIDType>::iterator found( m_ProductForceTypeToLoadCaseID.find(pfType) );
   if ( found == m_ProductForceTypeToLoadCaseID.end() )
   {
      ATLASSERT(false);
      return INVALID_ID;
   }
   else
   {
      return found->second;
   }
}

LoadCaseIDType CProductLoadMap::GetMaxLoadCaseID()
{
   return m_LoadCaseID;
}

void CProductLoadMap::AddLoadItem(ProductForceType pfType,CComBSTR bstrName,LoadCaseIDType lcid)
{
   m_ProductForceTypeToLoadName.insert(std::make_pair(pfType,bstrName));
   m_LoadNameToProductForceType.insert(std::make_pair(bstrName,pfType));
   m_ProductForceTypeToLoadCaseID.insert(std::make_pair(pfType,lcid));
}

std::vector<ProductForceType> CProductLoadMap::GetProductForces(IBroker* pBroker,LoadingCombinationType combo)
{
   // This method defines in one location the individual product loads that make up
   // each load combination.
   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bFutureOverlay = pBridge->IsFutureOverlay();

   std::vector<ProductForceType> pfTypes;
   pfTypes.reserve(pftProductForceTypeCount);

   switch(combo)
   {
   case lcDC:
      pfTypes.push_back(pftGirder);
      pfTypes.push_back(pftConstruction);
      pfTypes.push_back(pftSlab);
      pfTypes.push_back(pftSlabPad);
      pfTypes.push_back(pftSlabPanel);
      pfTypes.push_back(pftDiaphragm);
      pfTypes.push_back(pftSidewalk);
      pfTypes.push_back(pftTrafficBarrier);
      pfTypes.push_back(pftUserDC);
      pfTypes.push_back(pftShearKey);
      break;

   case lcDW:
      pfTypes.push_back(pftOverlay);
      pfTypes.push_back(pftUserDW);
      break;

   case lcDWRating:
      {
      GET_IFACE2(pBroker,ILossParameters,pLossParameters);
      pfTypes.push_back(pftUserDW);
      if ( pLossParameters->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         pfTypes.push_back(pftOverlay);
      }
      else
      {
         if ( !bFutureOverlay )
         {
            pfTypes.push_back(pftOverlayRating);
         }
      }
      }
      break;

   case lcDWp:
      {
      GET_IFACE2(pBroker,ILossParameters,pLossParameters);
      pfTypes.push_back(pftUserDW);
      if ( pLossParameters->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         pfTypes.push_back(pftOverlay);
      }
      else
      {
         if ( !bFutureOverlay )
         {
            pfTypes.push_back(pftOverlayRating);
         }
      }
      }
      break;

   case lcDWf:
      if ( bFutureOverlay )
      {
         pfTypes.push_back(pftOverlay);
      }
      break;

   case lcCR:
      pfTypes.push_back(pftCreep);
      break;

   case lcSH:
      pfTypes.push_back(pftShrinkage);
      break;

   case lcRE:
      pfTypes.push_back(pftRelaxation);
      break;

   case lcPS:
      pfTypes.push_back(pftSecondaryEffects);
      break;

   default:
      ATLASSERT(false);
   }

   return pfTypes;
}
