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

#pragma once

#include <Reporter\Reporter.h>
#include <Details.h>

struct IEAFDisplayUnits;

/*****************************************************************************
INTERFACE
   IPsLossEngineer

   Interface for computing and report prestress losses

DESCRIPTION
   Interface for computing and report prestress losses
*****************************************************************************/
// {69ABD84E-733A-4e1f-B64E-1EA888EA4935}
DEFINE_GUID(IID_IPsLossEngineer, 
0x69abd84e, 0x733a, 0x4e1f, 0xb6, 0x4e, 0x1e, 0xa8, 0x88, 0xea, 0x49, 0x35);
interface IPsLossEngineer : IUnknown
{
   //---------------------------------------------------------------------
   // Returns the details of the prestress loss calculation for losses computed upto and including
   // intervalIdx. Loses may be computed beyond this interval as well, however they are only
   // guarenteed to be computed upto and including the specified interval. An intervalIdx of
   // INVALID_INDEX means that losses are computed through all intervals
   virtual const LOSSDETAILS* GetLosses(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx = INVALID_INDEX) = 0;
   
   //---------------------------------------------------------------------
   // Returns prestress losses at a point of interest, but uses the input slab offset (the current design value)
   virtual const LOSSDETAILS* GetLosses(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx = INVALID_INDEX) = 0;

   //---------------------------------------------------------------------
   // Clears all losses that were computed as a result of calling GetLosses(poi,config)
   virtual void ClearDesignLosses() = 0;

   //---------------------------------------------------------------------
   // Reports loss calculations details
   virtual void BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) = 0;

   //---------------------------------------------------------------------
   // Reports summary of final losses
   virtual void ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) = 0;

   //---------------------------------------------------------------------
   // Returns the anchor set details for a particular tendon (this is basically the seating wedge parameters)
   virtual const ANCHORSETDETAILS* GetGirderTendonAnchorSetDetails(const CGirderKey& girderKey, DuctIndexType ductIdx) = 0;
   virtual const ANCHORSETDETAILS* GetSegmentTendonAnchorSetDetails(const CSegmentKey& segmentKey, DuctIndexType ductIdx) = 0;

   //---------------------------------------------------------------------
   // Returns the tendon elongation duration jacking at the specified end of the girder.
   virtual Float64 GetGirderTendonElongation(const CGirderKey& girderKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) = 0;
   virtual Float64 GetSegmentTendonElongation(const CSegmentKey& segmentKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) = 0;

   //---------------------------------------------------------------------
   // Returns the average friction and anchor set losses. The average values are
   // typically used to adjust Pjack so that a constant uniform post-tension force
   // is used for equivalent post-tensioning force analysis
   virtual void GetGirderTendonAverageFrictionAndAnchorSetLoss(const CGirderKey& girderKey, DuctIndexType ductIdx, Float64* pfpF, Float64* pfpA) = 0;
   virtual void GetSegmentTendonAverageFrictionAndAnchorSetLoss(const CSegmentKey& segmentKey, DuctIndexType ductIdx, Float64* pfpF, Float64* pfpA) = 0;
};

