///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "EditRatingCriteria.h"
#include <IFace\Project.h> // for IEvents
#include <IFace\RatingSpecification.h>
#include "PGSuperDoc.h" // for EAFGetBroker

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnGeneralRatingData::txnGeneralRatingData()
{
   CriteriaName = _T("");
   SystemFactorFlexure = 1.0;
   SystemFactorShear   = 1.0;
   ADTT                = -1; // unknown
   bIncludePedestrianLiveLoad = false;
   bDesignRating       = true;
   bLegalRating        = true;
   bPermitRating       = true;
}

bool txnGeneralRatingData::operator==(const txnGeneralRatingData& other) const
{
   if ( bDesignRating != other.bDesignRating )
      return false;

   if ( bLegalRating != other.bLegalRating )
      return false;

   if ( bPermitRating != other.bPermitRating )
      return false;

   if ( CriteriaName != other.CriteriaName )
      return false;

   if ( ADTT != other.ADTT )
      return false;

   if ( !IsEqual(SystemFactorFlexure,other.SystemFactorFlexure) )
      return false;

   if ( !IsEqual(SystemFactorShear,other.SystemFactorShear) )
      return false;

   if ( bIncludePedestrianLiveLoad != other.bIncludePedestrianLiveLoad )
      return false;

   return true;
}

///////////////////////////////////////////////////////////////////////////
txnDesignRatingData::txnDesignRatingData()
{
   StrengthI_DC = 1.25;
   StrengthI_DW = 1.50;
   StrengthI_LL_Inventory = -1;
   StrengthI_LL_Operating = -1;
   ServiceIII_DC = 1.0;
   ServiceIII_DW = 1.0;
   ServiceIII_LL = 0.8;
   AllowableTensionCoefficient = ::ConvertToSysUnits(0.19,unitMeasure::SqrtKSI);
   bRateForShear = true;
}

bool txnDesignRatingData::operator==(const txnDesignRatingData& other) const
{
   if ( !IsEqual(StrengthI_DC,other.StrengthI_DC) )
      return false;

   if ( !IsEqual(StrengthI_DW,other.StrengthI_DW) )
      return false;

   if ( !IsEqual(StrengthI_LL_Inventory,other.StrengthI_LL_Inventory) )
      return false;

   if ( !IsEqual(StrengthI_LL_Operating,other.StrengthI_LL_Operating) )
      return false;

   if ( !IsEqual(ServiceIII_DC,other.ServiceIII_DC) )
      return false;

   if ( !IsEqual(ServiceIII_DW,other.ServiceIII_DW) )
      return false;

   if ( !IsEqual(ServiceIII_LL,other.ServiceIII_LL) )
      return false;

   if ( !IsEqual(AllowableTensionCoefficient,other.AllowableTensionCoefficient) )
      return false;

   if ( bRateForShear != other.bRateForShear )
      return false;

   return true;
}

///////////////////////////////////////////////////////////////////////////
txnLegalRatingData::txnLegalRatingData()
{
   IM_Truck_Routine = 0.33;
   IM_Lane_Routine  = 0.00;
   IM_Truck_Special = 0.33;
   IM_Lane_Special  = 0.00;

   StrengthI_DC = 1.25;
   StrengthI_DW = 1.50;
   StrengthI_LL_Routine = -1;
   StrengthI_LL_Special = -1;
   ServiceIII_DC = 1.0;
   ServiceIII_DW = 1.0;
   ServiceIII_LL_Routine = -1;
   ServiceIII_LL_Special = -1;
   AllowableTensionCoefficient = ::ConvertToSysUnits(0.19,unitMeasure::SqrtKSI);
   bRateForStress   = true;
   bRateForShear    = true;
   bExcludeLaneLoad = false;
}

bool txnLegalRatingData::operator==(const txnLegalRatingData& other) const
{
   if ( RoutineNames != other.RoutineNames )
      return false;

   if ( SpecialNames != other.SpecialNames )
      return false;

   if ( !IsEqual(IM_Truck_Routine,other.IM_Truck_Routine) )
      return false;

   if ( !IsEqual(IM_Lane_Routine,other.IM_Lane_Routine) )
      return false;

   if ( !IsEqual(IM_Truck_Special,other.IM_Truck_Special) )
      return false;

   if ( !IsEqual(IM_Lane_Special,other.IM_Lane_Special) )
      return false;

   if ( !IsEqual(StrengthI_DC,other.StrengthI_DC) )
      return false;

   if ( !IsEqual(StrengthI_DW,other.StrengthI_DW) )
      return false;

   if ( !IsEqual(StrengthI_LL_Routine,other.StrengthI_LL_Routine) )
      return false;

   if ( !IsEqual(StrengthI_LL_Special,other.StrengthI_LL_Special) )
      return false;

   if ( !IsEqual(ServiceIII_DC,other.ServiceIII_DC) )
      return false;

   if ( !IsEqual(ServiceIII_DW,other.ServiceIII_DW) )
      return false;

   if ( !IsEqual(ServiceIII_LL_Routine,other.ServiceIII_LL_Routine) )
      return false;

   if ( !IsEqual(ServiceIII_LL_Special,other.ServiceIII_LL_Special) )
      return false;

   if ( !IsEqual(AllowableTensionCoefficient,other.AllowableTensionCoefficient) )
      return false;

   if ( bRateForStress != other.bRateForStress )
      return false;

   if ( bRateForShear != other.bRateForShear )
      return false;

   if ( bExcludeLaneLoad != other.bExcludeLaneLoad )
      return false;

   return true;
}


///////////////////////////////////////////////////////////////////////////
txnPermitRatingData::txnPermitRatingData()
{
   IM_Truck_Routine = 0.33;
   IM_Lane_Routine  = 0.00;

   IM_Truck_Special = 0.33;
   IM_Lane_Special  = 0.00;

   StrengthII_DC = 1.25;
   StrengthII_DW = 1.50;
   StrengthII_LL_Routine = -1;
   StrengthII_LL_Special = -1;
   ServiceI_DC = 1.0;
   ServiceI_DW = 1.0;
   ServiceI_LL_Routine = -1;
   ServiceI_LL_Special = -1;
   bRateForShear = true;
   bCheckReinforcementYielding = true;
   YieldStressCoefficient = 0.9;

   SpecialPermitType = pgsTypes::ptSingleTripWithEscort;
}

bool txnPermitRatingData::operator==(const txnPermitRatingData& other) const
{
   if ( RoutinePermitNames != other.RoutinePermitNames )
      return false;

   if ( SpecialPermitNames != other.SpecialPermitNames )
      return false;

   if ( !IsEqual(IM_Truck_Routine,other.IM_Truck_Routine) )
      return false;

   if ( !IsEqual(IM_Lane_Routine,other.IM_Lane_Routine) )
      return false;

   if ( !IsEqual(IM_Truck_Special,other.IM_Truck_Special) )
      return false;

   if ( !IsEqual(IM_Lane_Special,other.IM_Lane_Special) )
      return false;

   if ( !IsEqual(StrengthII_DC,other.StrengthII_DC) )
      return false;

   if ( !IsEqual(StrengthII_DW,other.StrengthII_DW) )
      return false;

   if ( !IsEqual(StrengthII_LL_Routine,other.StrengthII_LL_Routine) )
      return false;

   if ( !IsEqual(StrengthII_LL_Special,other.StrengthII_LL_Special) )
      return false;

   if ( !IsEqual(ServiceI_DC,other.ServiceI_DC) )
      return false;

   if ( !IsEqual(ServiceI_DW,other.ServiceI_DW) )
      return false;

   if ( !IsEqual(ServiceI_LL_Routine,other.ServiceI_LL_Routine) )
      return false;

   if ( !IsEqual(ServiceI_LL_Special,other.ServiceI_LL_Special) )
      return false;

   if ( bRateForShear != other.bRateForShear )
      return false;

   if ( bCheckReinforcementYielding != other.bCheckReinforcementYielding )
      return false;

   if ( !IsEqual(YieldStressCoefficient,other.YieldStressCoefficient) )
      return false;

   if ( SpecialPermitType != other.SpecialPermitType )
      return false;

   return true;
}

///////////////////////////////////////////////////////////////////////////

txnEditRatingCriteria::txnEditRatingCriteria(const txnRatingCriteriaData& oldData,const txnRatingCriteriaData& newData)
{
   m_Data[0] = oldData;
   m_Data[1] = newData;
}

txnEditRatingCriteria::~txnEditRatingCriteria()
{
}

bool txnEditRatingCriteria::Execute()
{
   Execute(1);
   return true;
}

void txnEditRatingCriteria::Undo()
{
   Execute(0);
}

void txnEditRatingCriteria::Execute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);
   GET_IFACE2(pBroker, IRatingSpecification, pRatingSpec );

   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

   pRatingSpec->SetRatingSpecification( m_Data[i].m_General.CriteriaName );
   pRatingSpec->SetADTT( m_Data[i].m_General.ADTT );
   pRatingSpec->SetSystemFactorFlexure(m_Data[i].m_General.SystemFactorFlexure);
   pRatingSpec->SetSystemFactorShear(m_Data[i].m_General.SystemFactorShear);
   pRatingSpec->IncludePedestrianLiveLoad(m_Data[i].m_General.bIncludePedestrianLiveLoad);
   pRatingSpec->EnableRating(pgsTypes::lrDesign_Inventory,m_Data[i].m_General.bDesignRating);
   pRatingSpec->EnableRating(pgsTypes::lrDesign_Operating,m_Data[i].m_General.bDesignRating);


   // Legal rating must be enabled and live loads must be selected for Routine Commercial Vehicles
   // for load rating to be performed
   if ( m_Data[i].m_General.bLegalRating && m_Data[i].m_Legal.RoutineNames.size() != 0 )
      pRatingSpec->EnableRating(pgsTypes::lrLegal_Routine,   true);
   else
      pRatingSpec->EnableRating(pgsTypes::lrLegal_Routine,   false);

   // Legal rating must be enabled and live loads must be selected for Special Hauling Vehicles
   // for load rating to be performed
   if ( m_Data[i].m_General.bLegalRating && m_Data[i].m_Legal.SpecialNames.size() != 0 )
      pRatingSpec->EnableRating(pgsTypes::lrLegal_Special,   true);
   else
      pRatingSpec->EnableRating(pgsTypes::lrLegal_Special,   false);

   if ( m_Data[i].m_General.bPermitRating && m_Data[i].m_Permit.RoutinePermitNames.size() != 0 )
      pRatingSpec->EnableRating(pgsTypes::lrPermit_Routine, true);
   else
      pRatingSpec->EnableRating(pgsTypes::lrPermit_Routine, false);

   if ( m_Data[i].m_General.bPermitRating && m_Data[i].m_Permit.SpecialPermitNames.size() != 0 )
      pRatingSpec->EnableRating(pgsTypes::lrPermit_Special, true);
   else
      pRatingSpec->EnableRating(pgsTypes::lrPermit_Special, false);

   // Design Rating Parameters
   pRatingSpec->SetDeadLoadFactor(      pgsTypes::StrengthI_Inventory,m_Data[i].m_Design.StrengthI_DC);
   pRatingSpec->SetWearingSurfaceFactor(pgsTypes::StrengthI_Inventory,m_Data[i].m_Design.StrengthI_DW);
   pRatingSpec->SetLiveLoadFactor(      pgsTypes::StrengthI_Inventory,m_Data[i].m_Design.StrengthI_LL_Inventory);

   pRatingSpec->SetDeadLoadFactor(      pgsTypes::ServiceIII_Inventory,m_Data[i].m_Design.ServiceIII_DC);
   pRatingSpec->SetWearingSurfaceFactor(pgsTypes::ServiceIII_Inventory,m_Data[i].m_Design.ServiceIII_DW);
   pRatingSpec->SetLiveLoadFactor(      pgsTypes::ServiceIII_Inventory,m_Data[i].m_Design.ServiceIII_LL);

   pRatingSpec->SetDeadLoadFactor(      pgsTypes::StrengthI_Operating,m_Data[i].m_Design.StrengthI_DC);
   pRatingSpec->SetWearingSurfaceFactor(pgsTypes::StrengthI_Operating,m_Data[i].m_Design.StrengthI_DW);
   pRatingSpec->SetLiveLoadFactor(      pgsTypes::StrengthI_Operating,m_Data[i].m_Design.StrengthI_LL_Operating);

   pRatingSpec->SetDeadLoadFactor(      pgsTypes::ServiceIII_Operating,m_Data[i].m_Design.ServiceIII_DC);
   pRatingSpec->SetWearingSurfaceFactor(pgsTypes::ServiceIII_Operating,m_Data[i].m_Design.ServiceIII_DW);
   pRatingSpec->SetLiveLoadFactor(      pgsTypes::ServiceIII_Operating,m_Data[i].m_Design.ServiceIII_LL);

   pRatingSpec->SetAllowableTensionCoefficient(pgsTypes::lrDesign_Inventory,m_Data[i].m_Design.AllowableTensionCoefficient );
   pRatingSpec->SetAllowableTensionCoefficient(pgsTypes::lrDesign_Operating,m_Data[i].m_Design.AllowableTensionCoefficient );
   
   pRatingSpec->RateForShear(pgsTypes::lrDesign_Inventory,m_Data[i].m_Design.bRateForShear);
   pRatingSpec->RateForShear(pgsTypes::lrDesign_Operating,m_Data[i].m_Design.bRateForShear);


   // Legal Rating Parameters

   // Routine
   pRatingSpec->SetDeadLoadFactor(      pgsTypes::StrengthI_LegalRoutine,m_Data[i].m_Legal.StrengthI_DC);
   pRatingSpec->SetWearingSurfaceFactor(pgsTypes::StrengthI_LegalRoutine,m_Data[i].m_Legal.StrengthI_DW);
   pRatingSpec->SetLiveLoadFactor(      pgsTypes::StrengthI_LegalRoutine,m_Data[i].m_Legal.StrengthI_LL_Routine);

   pRatingSpec->SetDeadLoadFactor(      pgsTypes::ServiceIII_LegalRoutine,m_Data[i].m_Legal.ServiceIII_DC);
   pRatingSpec->SetWearingSurfaceFactor(pgsTypes::ServiceIII_LegalRoutine,m_Data[i].m_Legal.ServiceIII_DW);
   pRatingSpec->SetLiveLoadFactor(      pgsTypes::ServiceIII_LegalRoutine,m_Data[i].m_Legal.ServiceIII_LL_Routine);

   // Specialized Hauling Vehicles
   pRatingSpec->SetDeadLoadFactor(      pgsTypes::StrengthI_LegalSpecial,m_Data[i].m_Legal.StrengthI_DC);
   pRatingSpec->SetWearingSurfaceFactor(pgsTypes::StrengthI_LegalSpecial,m_Data[i].m_Legal.StrengthI_DW);
   pRatingSpec->SetLiveLoadFactor(      pgsTypes::StrengthI_LegalSpecial,m_Data[i].m_Legal.StrengthI_LL_Special);

   pRatingSpec->SetDeadLoadFactor(      pgsTypes::ServiceIII_LegalSpecial,m_Data[i].m_Legal.ServiceIII_DC);
   pRatingSpec->SetWearingSurfaceFactor(pgsTypes::ServiceIII_LegalSpecial,m_Data[i].m_Legal.ServiceIII_DW);
   pRatingSpec->SetLiveLoadFactor(      pgsTypes::ServiceIII_LegalSpecial,m_Data[i].m_Legal.ServiceIII_LL_Special);

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   pLiveLoads->EnablePedestianLoad(pgsTypes::lltLegalRating_Routine,false);
   pLiveLoads->EnablePedestianLoad(pgsTypes::lltLegalRating_Special,false);
   
   pLiveLoads->SetLiveLoadNames(pgsTypes::lltLegalRating_Routine,m_Data[i].m_Legal.RoutineNames);
   pLiveLoads->SetLiveLoadNames(pgsTypes::lltLegalRating_Special,m_Data[i].m_Legal.SpecialNames);
   
   pLiveLoads->SetTruckImpact(pgsTypes::lltLegalRating_Routine,m_Data[i].m_Legal.IM_Truck_Routine);
   pLiveLoads->SetLaneImpact( pgsTypes::lltLegalRating_Routine,m_Data[i].m_Legal.IM_Lane_Routine);

   pLiveLoads->SetTruckImpact(pgsTypes::lltLegalRating_Special,m_Data[i].m_Legal.IM_Truck_Special);
   pLiveLoads->SetLaneImpact( pgsTypes::lltLegalRating_Special,m_Data[i].m_Legal.IM_Lane_Special);

   pRatingSpec->SetAllowableTensionCoefficient(pgsTypes::lrLegal_Routine,m_Data[i].m_Legal.AllowableTensionCoefficient );
   pRatingSpec->SetAllowableTensionCoefficient(pgsTypes::lrLegal_Special,m_Data[i].m_Legal.AllowableTensionCoefficient );
   
   pRatingSpec->RateForStress(pgsTypes::lrLegal_Routine,m_Data[i].m_Legal.bRateForStress);
   pRatingSpec->RateForStress(pgsTypes::lrLegal_Special,m_Data[i].m_Legal.bRateForStress);

   pRatingSpec->RateForShear(pgsTypes::lrLegal_Routine,m_Data[i].m_Legal.bRateForShear);
   pRatingSpec->RateForShear(pgsTypes::lrLegal_Special,m_Data[i].m_Legal.bRateForShear);

   pRatingSpec->ExcludeLegalLoadLaneLoading(m_Data[i].m_Legal.bExcludeLaneLoad);

   // Permit Rating Parameters - Routine
   pRatingSpec->SetDeadLoadFactor(      pgsTypes::StrengthII_PermitRoutine,m_Data[i].m_Permit.StrengthII_DC);
   pRatingSpec->SetWearingSurfaceFactor(pgsTypes::StrengthII_PermitRoutine,m_Data[i].m_Permit.StrengthII_DW);
   pRatingSpec->SetLiveLoadFactor(      pgsTypes::StrengthII_PermitRoutine,m_Data[i].m_Permit.StrengthII_LL_Routine);

   pRatingSpec->SetDeadLoadFactor(      pgsTypes::ServiceI_PermitRoutine,m_Data[i].m_Permit.ServiceI_DC);
   pRatingSpec->SetWearingSurfaceFactor(pgsTypes::ServiceI_PermitRoutine,m_Data[i].m_Permit.ServiceI_DW);
   pRatingSpec->SetLiveLoadFactor(      pgsTypes::ServiceI_PermitRoutine,m_Data[i].m_Permit.ServiceI_LL_Routine);

   // Permit Rating Parameters - Special
   pRatingSpec->SetDeadLoadFactor(      pgsTypes::StrengthII_PermitSpecial,m_Data[i].m_Permit.StrengthII_DC);
   pRatingSpec->SetWearingSurfaceFactor(pgsTypes::StrengthII_PermitSpecial,m_Data[i].m_Permit.StrengthII_DW);
   pRatingSpec->SetLiveLoadFactor(      pgsTypes::StrengthII_PermitSpecial,m_Data[i].m_Permit.StrengthII_LL_Special);

   pRatingSpec->SetDeadLoadFactor(      pgsTypes::ServiceI_PermitSpecial,m_Data[i].m_Permit.ServiceI_DC);
   pRatingSpec->SetWearingSurfaceFactor(pgsTypes::ServiceI_PermitSpecial,m_Data[i].m_Permit.ServiceI_DW);
   pRatingSpec->SetLiveLoadFactor(      pgsTypes::ServiceI_PermitSpecial,m_Data[i].m_Permit.ServiceI_LL_Special);

   pLiveLoads->EnablePedestianLoad(pgsTypes::lltPermitRating_Routine,false);
   pLiveLoads->EnablePedestianLoad(pgsTypes::lltPermitRating_Special,false);

   pLiveLoads->SetLiveLoadNames(pgsTypes::lltPermitRating_Routine,m_Data[i].m_Permit.RoutinePermitNames);
   pLiveLoads->SetTruckImpact(  pgsTypes::lltPermitRating_Routine,m_Data[i].m_Permit.IM_Truck_Routine);
   pLiveLoads->SetLaneImpact(   pgsTypes::lltPermitRating_Routine,m_Data[i].m_Permit.IM_Lane_Routine);

   pLiveLoads->SetLiveLoadNames(pgsTypes::lltPermitRating_Special,m_Data[i].m_Permit.SpecialPermitNames);
   pLiveLoads->SetTruckImpact(  pgsTypes::lltPermitRating_Special,m_Data[i].m_Permit.IM_Truck_Special);
   pLiveLoads->SetLaneImpact(   pgsTypes::lltPermitRating_Special,m_Data[i].m_Permit.IM_Lane_Special);
   
   pRatingSpec->RateForStress(pgsTypes::lrPermit_Routine,m_Data[i].m_Permit.bCheckReinforcementYielding);
   pRatingSpec->RateForStress(pgsTypes::lrPermit_Special,m_Data[i].m_Permit.bCheckReinforcementYielding);
   pRatingSpec->SetYieldStressLimitCoefficient( m_Data[i].m_Permit.YieldStressCoefficient );

   pRatingSpec->RateForShear(pgsTypes::lrPermit_Routine,m_Data[i].m_Permit.bRateForShear);
   pRatingSpec->RateForShear(pgsTypes::lrPermit_Special,m_Data[i].m_Permit.bRateForShear);

   pRatingSpec->SetSpecialPermitType(m_Data[i].m_Permit.SpecialPermitType);

   pEvents->FirePendingEvents();
}

txnTransaction* txnEditRatingCriteria::CreateClone() const
{
   return new txnEditRatingCriteria(m_Data[0],m_Data[1]);
}

std::_tstring txnEditRatingCriteria::Name() const
{
   return _T("Edit Rating Criteria");
}

bool txnEditRatingCriteria::IsUndoable()
{
   return true;
}

bool txnEditRatingCriteria::IsRepeatable()
{
   return false;
}
