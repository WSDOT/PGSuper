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

#ifndef INCLUDED_PSFORCEENG_H_
#define INCLUDED_PSFORCEENG_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#include <Details.h>
#include <IFace\PsLossEngineer.h>
#include <IFace\Project.h>
#include <psgLib/HoldDownCriteria.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
interface IBroker;
class pgsPointOfInterest;
class pgsDevelopmentLength;
class rptChapter;
interface IEAFDisplayUnits;

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   pgsPsForceEng

   Prestress force engineer.

DESCRIPTION
   Prestress force engineer. Responsible for computing prestressing forces, 
   stress, and losses. Also caches loss computation details.
*****************************************************************************/

class pgsPsForceEng
{
public:
   pgsPsForceEng();
   ~pgsPsForceEng();

   void SetBroker(IBroker* pBroker);
   void SetStatusGroupID(StatusGroupIDType statusGroupID);

   const LOSSDETAILS* GetLosses(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx=INVALID_INDEX) const;
   const LOSSDETAILS* GetLosses(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx=INVALID_INDEX) const;

   //------------------------------------------------------------------------
   // Clears all prestress loss calculations
   void Invalidate();

   //------------------------------------------------------------------------
   // Clears all previously computed and stored design losses
   void ClearDesignLosses();

   //------------------------------------------------------------------------
   // Reports the details of the prestress loss calculations
   void ReportLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) const;

   //------------------------------------------------------------------------
   // Reports a summary of the final prestress losses
   void ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) const;

   //------------------------------------------------------------------------
   // Computes the basic anchor set loss details (basically the anchor set length and
   // anchor set loss a seating end)
   const ANCHORSETDETAILS* GetGirderTendonAnchorSetDetails(const CGirderKey& girderKey, DuctIndexType ductIdx) const;
   const ANCHORSETDETAILS* GetSegmentTendonAnchorSetDetails(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const;

   //------------------------------------------------------------------------
   // Computes the tendon elongation during jacking
   Float64 GetGirderTendonElongation(const CGirderKey& girderKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) const;
   Float64 GetSegmentTendonElongation(const CSegmentKey& segmentKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) const;

   //------------------------------------------------------------------------
   // Returns the average friction loss
   Float64 GetGirderTendonAverageFrictionLoss(const CGirderKey& girderKey, DuctIndexType ductIdx) const;
   Float64 GetSegmentTendonAverageFrictionLoss(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const;

   //------------------------------------------------------------------------
   // Returns the average anchor set loss
   Float64 GetGirderTendonAverageAnchorSetLoss(const CGirderKey& girderKey, DuctIndexType ductIdx) const;
   Float64 GetSegmentTendonAverageAnchorSetLoss(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const;

   //------------------------------------------------------------------------
   // Returns the maximum jacking force
   Float64 GetPjackMax(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType,StrandIndexType nStrands) const;
   Float64 GetPjackMax(const CSegmentKey& segmentKey,const WBFL::Materials::PsStrand& strand,StrandIndexType nStrands) const;

   //------------------------------------------------------------------------
   // Returns the prestress hold down force
   Float64 GetHoldDownForce(const CSegmentKey& segmentKey,HoldDownCriteria::Type type,Float64* pSlope, pgsPointOfInterest* pPoi, const GDRCONFIG* pConfig = nullptr) const;

   //------------------------------------------------------------------------
   // Returns the effective force in prestressing strand at the specified interval.. includes losses and gains
   // P = (aps)(Nstrands)(fpe)
   Float64 GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,pgsTypes::TransferLengthType xferLengthType,const GDRCONFIG* pConfig=nullptr) const;
   Float64 GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,bool bIncludeElasticEffects, pgsTypes::TransferLengthType xferLengthType, const GDRCONFIG* pConfig = nullptr) const;

   //------------------------------------------------------------------------
   // Returns the effective prestress at the specified interval... includes loss and gains
   // fpe = fpj - loss + gain
   Float64 GetEffectivePrestress(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, const GDRCONFIG* pConfig) const;
   Float64 GetEffectivePrestress(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, bool bIncludeElasticEffects, bool bApplyElasticGainReduction, const GDRCONFIG* pConfig) const;

   //------------------------------------------------------------------------
   // Returns the effective prestress loss... The effective losses are time dependent losses + elastic effects
   // effective losses = fpj - fpe
   Float64 GetEffectivePrestressLoss(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime, bool bApplyElasticGainReduction,const GDRCONFIG* pConfig) const;
   Float64 GetEffectivePrestressLossWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx, bool bIncludeElasticEffects, bool bApplyElasticGainReduction, const GDRCONFIG* pConfig) const;

   //------------------------------------------------------------------------
   // Returns the prestress loss at the specified interval (does not include elastic effects)
   // Loss due to creep, shrinkage, and relaxation only
   Float64 GetTimeDependentLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig) const;

   //------------------------------------------------------------------------
   // Returns the elastic gains due to externally applied loads, including elastic shortening effects, at the specified interval
   Float64 GetInstantaneousEffects(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime, bool bApplyElasticGainReduction,const GDRCONFIG* pConfig) const;
   Float64 GetInstantaneousEffectsWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState, bool bApplyElasticGainReduction, VehicleIndexType vehicleIdx, const GDRCONFIG* pConfig) const;

   //------------------------------------------------------------------------
   // Returns the effective prestress force and effective prestress... includes losses and elastic gains
   // fpe = fpj - loss + gain + gain due to live load
   Float64 GetPrestressForceWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState, VehicleIndexType vehicleIndex, bool bIncludeElasticEffects, const GDRCONFIG* pConfig) const;
   Float64 GetEffectivePrestressWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState, VehicleIndexType vehicleIndex, bool bIncludeElasticEffects, bool bApplyElasticGainReduction, const GDRCONFIG* pConfig) const;

protected:

private:
   IBroker* m_pBroker;
   StatusGroupIDType m_StatusGroupID;
   mutable CComPtr<IPsLossEngineer> m_LossEngineer;

   void CreateLossEngineer(const CGirderKey& girderKey) const;

   Float64 GetTimeDependentLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig,const LOSSDETAILS* pDetails) const;
   Float64 GetInstantaneousEffects(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime, bool bApplyElasticGainReduction,const GDRCONFIG* pConfig,const LOSSDETAILS* pDetails) const;
   Float64 GetInstantaneousEffectsWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState, bool bApplyElasticGainReduction,VehicleIndexType vehicleIdx,const GDRCONFIG* pConfig,const LOSSDETAILS* pDetails) const;

   Float64 GetElasticGainDueToLiveLoad(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, pgsTypes::LimitState limitState, VehicleIndexType vehicleIndex, bool bIncludeLiveLoadFactor, bool bApplyElasticGainReduction, const GDRCONFIG* pConfig, const LOSSDETAILS* pDetails) const;
};

#endif // INCLUDED_PSFORCEENG_H_
