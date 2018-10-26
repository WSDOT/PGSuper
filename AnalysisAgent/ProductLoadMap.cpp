///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

CProductLoadMap::CProductLoadMap()
{
   // These are the load names used in the LBAM model. Mapping the
   // ProductForceType to a consistent string makes it easier to 
   // avoid issues with using the wrong, or misspelled, name in the LBAM
   AddLoadItem(pftGirder,                _T("Girder"));
   AddLoadItem(pftDiaphragm,             _T("Diaphragm"));
   AddLoadItem(pftConstruction,          _T("Construction"));
   AddLoadItem(pftSlab,                  _T("Slab"));
   AddLoadItem(pftSlabPad,               _T("Haunch"));
   AddLoadItem(pftSlabPanel,             _T("Slab Panel"));
   AddLoadItem(pftOverlay,               _T("Overlay"));
   AddLoadItem(pftOverlayRating,         _T("Overlay Rating"));
   AddLoadItem(pftTrafficBarrier,        _T("Traffic Barrier"));
   AddLoadItem(pftSidewalk,              _T("Sidewalk"));
   AddLoadItem(pftUserDC,                _T("UserDC"));
   AddLoadItem(pftUserDW,                _T("UserDW"));
   AddLoadItem(pftUserLLIM,              _T("UserLLIM"));
   AddLoadItem(pftShearKey,              _T("Shear Key"));
   //AddLoadItem(pftPretension,             _T("Pretensioning")); // not modeled in the LBAM
   AddLoadItem(pftEquivPostTensioning,   _T("Equiv Post Tensioning"));
   //AddLoadItem(pftPrimaryPostTensioning, _T("Primary Post Tensioning")); // not modeled in the LBAM
   //AddLoadItem(pftSecondaryEffects,      _T("Secondary Effects")); // not modeled in the LBAM
   AddLoadItem(pftCreep,                 _T("Creep"));
   AddLoadItem(pftShrinkage,             _T("Shrinkage"));
   AddLoadItem(pftRelaxation,            _T("Relaxation"));
}

ProductForceType CProductLoadMap::GetProductForceType(CComBSTR bstrName)
{
   std::map<CComBSTR,ProductForceType>::iterator found( m_LoadNameToLoadID.find(bstrName) );
   if ( found == m_LoadNameToLoadID.end() )
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
   std::map<ProductForceType,CComBSTR>::iterator found( m_LoadIDToLoadName.find(pfType) );
   if ( found == m_LoadIDToLoadName.end() )
   {
      ATLASSERT(false);
      return CComBSTR();
   }
   else
   {
      return found->second;
   }
}

void CProductLoadMap::AddLoadItem(ProductForceType productForceType,CComBSTR bstrName)
{
   m_LoadIDToLoadName.insert(std::make_pair(productForceType,bstrName));
   m_LoadNameToLoadID.insert(std::make_pair(bstrName,productForceType));
}
