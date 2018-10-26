///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
interface IBroker;
class pgsPointOfInterest;
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
   //------------------------------------------------------------------------
   // Default constructor
   pgsPsForceEng();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsPsForceEng(const pgsPsForceEng& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsPsForceEng();

   //------------------------------------------------------------------------
   // Assignment operator
   pgsPsForceEng& operator = (const pgsPsForceEng& rOther);

   void SetBroker(IBroker* pBroker);
   void SetStatusGroupID(StatusGroupIDType statusGroupID);

   const LOSSDETAILS* GetLosses(const pgsPointOfInterest& poi);
   const LOSSDETAILS* GetLosses(const pgsPointOfInterest& poi,const GDRCONFIG& config);

   //------------------------------------------------------------------------
   // Clears all prestress loss calculations
   void Invalidate();

   //------------------------------------------------------------------------
   // Clears all prevously computed and stored design losses
   void ClearDesignLosses();

   //------------------------------------------------------------------------
   // Reports the details of the prestress loss calculations
   void ReportLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);

   //------------------------------------------------------------------------
   // Reports a summary of the final prestress losses
   void ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);

   //------------------------------------------------------------------------
   // Computes the basic anchor set loss details (basically the anchor set length and
   // anchor set loss a seating end)
   const ANCHORSETDETAILS* GetAnchorSetDetails(const CGirderKey& girderKey,DuctIndexType ductIdx);

   //------------------------------------------------------------------------
   // Computes the tendon elongation during jacking
   Float64 GetElongation(const CGirderKey& girderKey,DuctIndexType ductIdx,pgsTypes::MemberEndType endType);

   //------------------------------------------------------------------------
   // Returns the maximum jacking force
   Float64 GetPjackMax(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType,StrandIndexType nStrands);
   Float64 GetPjackMax(const CSegmentKey& segmentKey,const matPsStrand& strand,StrandIndexType nStrands);

   //------------------------------------------------------------------------
   // Returns the prestress transfer length
   Float64 GetXferLength(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType);
   Float64 GetXferLength(const matPsStrand& strand);

   //------------------------------------------------------------------------
   // Returns the transfer length adjustment factor. The factor is 0 at the
   // point where bond begins and 1.0 at the end of the transfer length
   Float64 GetXferLengthAdjustment(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType);
   Float64 GetXferLengthAdjustment(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config);

   //------------------------------------------------------------------------
   // Returns the prestress development length
   Float64 GetDevLength(const pgsPointOfInterest& poi,bool bDebonded);
   Float64 GetDevLength(const pgsPointOfInterest& poi,bool bDebonded,const GDRCONFIG& config);

   //------------------------------------------------------------------------
   // Returns the development length adjustment factor. The factor is 0 at the
   // point where bond begins and 1.0 at the end of the development length
   Float64 GetDevLengthAdjustment(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType);
   Float64 GetDevLengthAdjustment(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,Float64 fps,Float64 fpe);
   Float64 GetDevLengthAdjustment(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG& config);
   Float64 GetDevLengthAdjustment(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG& config,Float64 fps,Float64 fpe);

   //------------------------------------------------------------------------
   // Returns the details of the develpment lenght computations
   STRANDDEVLENGTHDETAILS GetDevLengthDetails(const pgsPointOfInterest& poi,bool bDebonded);
   STRANDDEVLENGTHDETAILS GetDevLengthDetails(const pgsPointOfInterest& poi,bool bDebonded,Float64 fps,Float64 fpe);
   STRANDDEVLENGTHDETAILS GetDevLengthDetails(const pgsPointOfInterest& poi,bool bDebonded,const GDRCONFIG& config);
   STRANDDEVLENGTHDETAILS GetDevLengthDetails(const pgsPointOfInterest& poi,bool bDebonded,const GDRCONFIG& config,Float64 fps,Float64 fpe);

   //------------------------------------------------------------------------
   // Returns the prestress hold down force
   Float64 GetHoldDownForce(const CSegmentKey& segmentKey);
   Float64 GetHoldDownForce(const CSegmentKey& segmentKey,const GDRCONFIG& config);

   //------------------------------------------------------------------------
   // Returns the effective force in prestressing strand at the specified interval
   Float64 GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime);
   Float64 GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG& config);

   //------------------------------------------------------------------------
   // Returns the effective prestress at the specified interval
   Float64 GetEffectivePrestress(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime);
   Float64 GetEffectivePrestress(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig);

   //------------------------------------------------------------------------
   // Returns the prestress loss at the specified interval
   Float64 GetPrestressLoss(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime);
   Float64 GetPrestressLoss(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig);

   //------------------------------------------------------------------------
   // Returns the effective prestress force, effective prestress, and prestress losses including
   // the effect of elastic gains due to live load
   Float64 GetPrestressForceWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG* pConfig);
   Float64 GetEffectivePrestressWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG* pConfig);
   Float64 GetPrestressLossWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG* pConfig);

protected:
   void MakeCopy(const pgsPsForceEng& rOther);
   virtual void MakeAssignment(const pgsPsForceEng& rOther);

private:
   IBroker* m_pBroker;
   StatusGroupIDType m_StatusGroupID;
   CComPtr<IPsLossEngineer> m_LossEngineer;

   // method used to compute prestress transfer length
   pgsTypes::PrestressTransferComputationType m_PrestressTransferComputationType;

   void CreateLossEngineer(const CGirderKey& girderKey);
   Float64 GetPrestressLoss(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const LOSSDETAILS* pDetails);


   //// Losses are cached for two different cases:
   //// 1) This data structure caches losses for the current project data
   //std::map<PoiIDKey,LOSSDETAILS> m_PsLosses;

   //// 2) This data structure is for design cases. It caches the most recently
   ////    computed losses
   //CDesignLosses m_DesignLosses;
};

#endif // INCLUDED_PSFORCEENG_H_
