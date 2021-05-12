///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CProductLoadMap::CProductLoadMap()
{
   // These are the load names used in the LBAM model. Mapping the
   // pgsTypes::ProductForceType to a consistent string makes it easier to 
   // avoid issues with using the wrong, or misspelled, name in the LBAM
   m_LoadCaseID = 0;

   AddLoadItem(pgsTypes::pftGirder,                  _T("Girder"),        m_LoadCaseID++);
   AddLoadItem(pgsTypes::pftDiaphragm,               _T("Diaphragm"),     m_LoadCaseID++);
   AddLoadItem(pgsTypes::pftConstruction,            _T("Construction"),  m_LoadCaseID++);
   AddLoadItem(pgsTypes::pftSlab,                    _T("Slab"),          m_LoadCaseID++);
   AddLoadItem(pgsTypes::pftSlabPad,                 _T("Haunch"),        m_LoadCaseID++);
   AddLoadItem(pgsTypes::pftSlabPanel,               _T("Slab Panel"),    m_LoadCaseID++);
   AddLoadItem(pgsTypes::pftOverlay,                 _T("Overlay"),       m_LoadCaseID++);
   AddLoadItem(pgsTypes::pftOverlayRating,           _T("Overlay Rating"),  m_LoadCaseID++);
   AddLoadItem(pgsTypes::pftTrafficBarrier,          _T("Traffic Barrier"), m_LoadCaseID++);
   AddLoadItem(pgsTypes::pftSidewalk,                _T("Sidewalk"),        m_LoadCaseID++);
   AddLoadItem(pgsTypes::pftUserDC,                  _T("UserDC"),          m_LoadCaseID++);
   AddLoadItem(pgsTypes::pftUserDW,                  _T("UserDW"),          m_LoadCaseID++);
   AddLoadItem(pgsTypes::pftUserLLIM,                _T("UserLLIM"),        m_LoadCaseID++);
   AddLoadItem(pgsTypes::pftShearKey,                _T("Shear Key"), m_LoadCaseID++);
   AddLoadItem(pgsTypes::pftLongitudinalJoint,       _T("Longitudinal Joint"), m_LoadCaseID++);
   //AddLoadItem(pgsTypes::pftPretension,              _T("Pretensioning"),   m_LoadCaseID++); // not modeled in the LBAM
   //AddLoadItem(pgsTypes::pftPostTensioning, _T("Post Tensioning"), m_LoadCaseID++); // not modeled in the LBAM
   AddLoadItem(pgsTypes::pftSecondaryEffects,       _T("Secondary Effects"), m_LoadCaseID++); // not modeled in the LBAM
   AddLoadItem(pgsTypes::pftCreep,                   _T("Creep"),         m_LoadCaseID++);
   AddLoadItem(pgsTypes::pftShrinkage,               _T("Shrinkage"),     m_LoadCaseID++);
   AddLoadItem(pgsTypes::pftRelaxation,              _T("Relaxation"),    m_LoadCaseID++);
}

pgsTypes::ProductForceType CProductLoadMap::GetProductForceType(CComBSTR bstrName) const
{
   auto found( m_LoadNameToProductForceType.find(bstrName) );
   if ( found == m_LoadNameToProductForceType.cend() )
   {
      ATLASSERT(false);
      return pgsTypes::pftGirder;
   }
   else
   {
      return found->second;
   }
}

CComBSTR CProductLoadMap::GetGroupLoadName(pgsTypes::ProductForceType pfType) const
{
   auto found( m_ProductForceTypeToLoadName.find(pfType) );
   if ( found == m_ProductForceTypeToLoadName.cend() )
   {
      ATLASSERT(false);
      return CComBSTR();
   }
   else
   {
      return found->second;
   }
}

LoadCaseIDType CProductLoadMap::GetLoadCaseID(pgsTypes::ProductForceType pfType) const
{
   auto found( m_ProductForceTypeToLoadCaseID.find(pfType) );
   if ( found == m_ProductForceTypeToLoadCaseID.cend() )
   {
      ATLASSERT(false);
      return INVALID_ID;
   }
   else
   {
      return found->second;
   }
}

LoadCaseIDType CProductLoadMap::GetMaxLoadCaseID() const
{
   return m_LoadCaseID;
}

void CProductLoadMap::AddLoadItem(pgsTypes::ProductForceType pfType,CComBSTR bstrName,LoadCaseIDType lcid)
{
   m_ProductForceTypeToLoadName.insert(std::make_pair(pfType,bstrName));
   m_LoadNameToProductForceType.insert(std::make_pair(bstrName,pfType));
   m_ProductForceTypeToLoadCaseID.insert(std::make_pair(pfType,lcid));
}

std::vector<pgsTypes::ProductForceType> CProductLoadMap::GetProductForces(IBroker* pBroker,LoadingCombinationType combo)
{
   // This method defines in one location the individual product loads that make up
   // each load combination.
   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bFutureOverlay = pBridge->IsFutureOverlay();

   std::vector<pgsTypes::ProductForceType> pfTypes;
   pfTypes.reserve(pgsTypes::pftProductForceTypeCount);

   switch(combo)
   {
   case lcDC:
      pfTypes.push_back(pgsTypes::pftGirder);
      pfTypes.push_back(pgsTypes::pftConstruction);
      pfTypes.push_back(pgsTypes::pftSlab);
      pfTypes.push_back(pgsTypes::pftSlabPad);
      pfTypes.push_back(pgsTypes::pftSlabPanel);
      pfTypes.push_back(pgsTypes::pftDiaphragm);
      pfTypes.push_back(pgsTypes::pftSidewalk);
      pfTypes.push_back(pgsTypes::pftTrafficBarrier);
      pfTypes.push_back(pgsTypes::pftUserDC);
      pfTypes.push_back(pgsTypes::pftShearKey);
      pfTypes.push_back(pgsTypes::pftLongitudinalJoint);
      break;

   case lcDW:
      pfTypes.push_back(pgsTypes::pftOverlay);
      pfTypes.push_back(pgsTypes::pftUserDW);
      break;

   case lcDWRating:
      {
      GET_IFACE2(pBroker,ILossParameters,pLossParameters);
      pfTypes.push_back(pgsTypes::pftUserDW);
      if ( pLossParameters->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         pfTypes.push_back(pgsTypes::pftOverlay);
      }
      else
      {
         if ( !bFutureOverlay )
         {
            pfTypes.push_back(pgsTypes::pftOverlayRating);
         }
      }
      }
      break;

   case lcDWp:
      {
      GET_IFACE2(pBroker,ILossParameters,pLossParameters);
      pfTypes.push_back(pgsTypes::pftUserDW);
      if ( pLossParameters->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         pfTypes.push_back(pgsTypes::pftOverlay);
      }
      else
      {
         if ( !bFutureOverlay )
         {
            pfTypes.push_back(pgsTypes::pftOverlayRating);
         }
      }
      }
      break;

   case lcDWf:
      {
      GET_IFACE2(pBroker,ILossParameters,pLossParameters);
      if ( pLossParameters->GetLossMethod() != pgsTypes::TIME_STEP && bFutureOverlay)
      {
         pfTypes.push_back(pgsTypes::pftOverlay);
      }
      }
      break;

   case lcCR:
      pfTypes.push_back(pgsTypes::pftCreep);
      break;

   case lcSH:
      pfTypes.push_back(pgsTypes::pftShrinkage);
      break;

   case lcRE:
      pfTypes.push_back(pgsTypes::pftRelaxation);
      break;

   case lcPS:
      pfTypes.push_back(pgsTypes::pftSecondaryEffects);
      break;

   default:
      ATLASSERT(false);
   }

   return pfTypes;
}

std::vector<pgsTypes::ProductForceType> CProductLoadMap::GetProductForces(IBroker * pBroker, const CGirderKey & girderKey)
{
std::vector<pgsTypes::ProductForceType> vProductForces;

   GET_IFACE2(pBroker,IProductLoads,pLoads);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IUserDefinedLoadData,pUserLoads);

   vProductForces.push_back(pgsTypes::pftGirder);

   vProductForces.push_back(pgsTypes::pftDiaphragm);

   if ( pLoads->HasShearKeyLoad(girderKey) )
   {
      vProductForces.push_back(pgsTypes::pftShearKey);
   }

   if ( !IsZero(pUserLoads->GetConstructionLoad()) )
   {
      vProductForces.push_back(pgsTypes::pftConstruction);
   }

   if (pLoads->HasLongitudinalJointLoad())
   {
      vProductForces.push_back(pgsTypes::pftLongitudinalJoint);
   }

   if ( pBridge->GetDeckType() != pgsTypes::sdtNone )
   {
      vProductForces.push_back(pgsTypes::pftSlab);
      vProductForces.push_back(pgsTypes::pftSlabPad);

      if ( pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP )
      {
         vProductForces.push_back(pgsTypes::pftSlabPanel);
      }
   }

   if ( pLoads->HasSidewalkLoad(girderKey) )
   {
      vProductForces.push_back(pgsTypes::pftSidewalk);
   }

   vProductForces.push_back(pgsTypes::pftTrafficBarrier);

   if ( pBridge->HasOverlay() )
   {
      vProductForces.push_back(pgsTypes::pftOverlay);
   }

   if ( pUserLoads->HasUserDC(girderKey) )
   {
      vProductForces.push_back(pgsTypes::pftUserDC);
   }

   if ( pUserLoads->HasUserDW(girderKey) )
   {
      vProductForces.push_back(pgsTypes::pftUserDW);
   }

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      if ( 0 < pStrandGeom->GetStrandCount(CSegmentKey(girderKey,segIdx),pgsTypes::Straight) ||
           0 < pStrandGeom->GetStrandCount(CSegmentKey(girderKey,segIdx),pgsTypes::Harped)   ||
           0 < pStrandGeom->GetStrandCount(CSegmentKey(girderKey,segIdx),pgsTypes::Temporary)
           )
      {
         vProductForces.push_back(pgsTypes::pftPretension);
         break;
      }
   }

   GET_IFACE2(pBroker,IGirderTendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   if ( 0 < nDucts )
   {
      vProductForces.push_back(pgsTypes::pftPostTensioning);
      vProductForces.push_back(pgsTypes::pftSecondaryEffects);
   }

   // time-depending effects
   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if ( !pLossParams->IgnoreCreepEffects() )
   {
      vProductForces.push_back(pgsTypes::pftCreep);
   }

   if ( !pLossParams->IgnoreShrinkageEffects() )
   {
      vProductForces.push_back(pgsTypes::pftShrinkage);
   }

   if ( !pLossParams->IgnoreRelaxationEffects() )
   {
      vProductForces.push_back(pgsTypes::pftRelaxation);
   }

   return vProductForces;
}

