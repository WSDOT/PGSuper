///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

#include <System\Transaction.h>

class txnGeneralRatingData
{
public:
   std::_tstring CriteriaName;
   Int16 ADTT; // < 0 means unknown
   Float64 SystemFactorFlexure;
   Float64 SystemFactorShear;
   bool bIncludePedestrianLiveLoad;
   bool bDesignRating;
   bool bLegalRating;
   bool bPermitRating;

   txnGeneralRatingData();
   bool operator==(const txnGeneralRatingData& other) const;
   bool operator!=(const txnGeneralRatingData& other) const
   {
      return !operator==(other);
   }
};

class txnDesignRatingData
{
public:
   Float64 StrengthI_DC;
   Float64 StrengthI_DW;
   Float64 StrengthI_LL_Inventory;
   Float64 StrengthI_LL_Operating;
   Float64 ServiceIII_DC;
   Float64 ServiceIII_DW;
   Float64 ServiceIII_LL;
   Float64 AllowableTensionCoefficient;
   bool    bRateForShear;

   txnDesignRatingData();
   bool operator==(const txnDesignRatingData& other) const;
   bool operator!=(const txnDesignRatingData& other) const
   {
      return !operator==(other);
   }
};

class txnLegalRatingData
{
public:
   std::vector<std::_tstring> RoutineNames;
   std::vector<std::_tstring> SpecialNames;
   Float64 IM_Truck_Routine;
   Float64 IM_Lane_Routine;
   Float64 IM_Truck_Special;
   Float64 IM_Lane_Special;
   Float64 StrengthI_DC;
   Float64 StrengthI_DW;
   Float64 StrengthI_LL_Routine;
   Float64 StrengthI_LL_Special;
   Float64 ServiceIII_DC;
   Float64 ServiceIII_DW;
   Float64 ServiceIII_LL_Routine;
   Float64 ServiceIII_LL_Special;
   Float64 AllowableTensionCoefficient;
   bool    bRateForStress;
   bool    bRateForShear;
   bool    bExcludeLaneLoad;

   txnLegalRatingData();
   bool operator==(const txnLegalRatingData& other) const;
   bool operator!=(const txnLegalRatingData& other) const
   {
      return !operator==(other);
   }
};


class txnPermitRatingData
{
public:
   std::vector<std::_tstring> RoutinePermitNames;
   std::vector<std::_tstring> SpecialPermitNames;
   Float64 IM_Truck_Routine;
   Float64 IM_Lane_Routine;
   Float64 IM_Truck_Special;
   Float64 IM_Lane_Special;
   Float64 StrengthII_DC;
   Float64 StrengthII_DW;
   Float64 StrengthII_LL_Routine;
   Float64 StrengthII_LL_Special;
   Float64 ServiceI_DC;
   Float64 ServiceI_DW;
   Float64 ServiceI_LL_Routine;
   Float64 ServiceI_LL_Special;
   bool    bRateForShear;
   bool    bCheckReinforcementYielding;
   Float64 YieldStressCoefficient;
   pgsTypes::SpecialPermitType SpecialPermitType;

   txnPermitRatingData();
   bool operator==(const txnPermitRatingData& other) const;
   bool operator!=(const txnPermitRatingData& other) const
   {
      return !operator==(other);
   }
};

struct txnRatingCriteriaData
{
   txnGeneralRatingData m_General;
   txnDesignRatingData  m_Design;
   txnLegalRatingData   m_Legal;
   txnPermitRatingData  m_Permit;

   bool operator==(const txnRatingCriteriaData& other) const
   {
      if ( m_General != other.m_General )
         return false;

      if ( m_Design != other.m_Design )
         return false;

      if ( m_Legal != other.m_Legal )
         return false;

      if ( m_Permit != other.m_Permit )
         return false;

      return true;
   }

   bool operator!=(const txnRatingCriteriaData& other) const
   {
      return !operator==(other);
   }
};

class txnEditRatingCriteria : public txnTransaction
{
public:
   txnEditRatingCriteria(const txnRatingCriteriaData& oldData,const txnRatingCriteriaData& oldNewData);

   ~txnEditRatingCriteria();

   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   void Execute(int i);

   txnRatingCriteriaData m_Data[2];
};
