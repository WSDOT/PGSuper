///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#include "StdAfx.h"
#include <psgLib\OldHaulTruck.h>
#include <psgLib\HaulTruckLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


COldHaulTruck::COldHaulTruck()
{
}

bool COldHaulTruck::IsEqual(const HaulTruckLibraryEntry* pEntry) const
{
   if ( m_TruckRollStiffnessMethod != ROLLSTIFFNESS_LUMPSUM )
      return false; // can't be equal if roll stiffness method isn't constant value

   return IsEqual(m_TruckRollStiffness,pEntry);
}

bool COldHaulTruck::IsEqual(Float64 Ktheta,const HaulTruckLibraryEntry* pEntry) const
{
   if ( !::IsEqual(Ktheta,pEntry->GetRollStiffness()) )
      return false;

   if ( !::IsEqual(m_Hbg,pEntry->GetBottomOfGirderHeight()) )
      return false;

   if ( !::IsEqual(m_Hrc,pEntry->GetRollCenterHeight()) )
      return false;

   if ( !::IsEqual(m_Wcc,pEntry->GetAxleWidth()) )
      return false;

   if ( !::IsEqual(m_Lmax,pEntry->GetMaxDistanceBetweenBunkPoints()) )
      return false;

   if ( !::IsEqual(m_MaxOH,pEntry->GetMaximumLeadingOverhang()) )
      return false;

   if ( !::IsEqual(m_MaxWeight,pEntry->GetMaxGirderWeight()) )
      return false;

   return true;
}

void COldHaulTruck::InitEntry(HaulTruckLibraryEntry* pEntry) const
{
   InitEntry(m_TruckRollStiffness,pEntry);
}

void COldHaulTruck::InitEntry(Float64 Ktheta,HaulTruckLibraryEntry* pEntry) const
{
   pEntry->SetRollStiffness(Ktheta);
   pEntry->SetBottomOfGirderHeight(m_Hbg);
   pEntry->SetRollCenterHeight(m_Hrc);
   pEntry->SetAxleWidth(m_Wcc);
   pEntry->SetMaxDistanceBetweenBunkPoints(m_Lmax);
   pEntry->SetMaximumLeadingOverhang(m_MaxOH);
   pEntry->SetMaxGirderWeight(m_MaxWeight);
}
