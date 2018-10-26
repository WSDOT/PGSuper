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

// ContinuousStandFiller.h: interface for the CContinuousStandFiller class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CONTINUOUSSTANDFILLER_H__314FC998_ED67_4365_BBE9_63A2874BAE65__INCLUDED_)
#define AFX_CONTINUOUSSTANDFILLER_H__314FC998_ED67_4365_BBE9_63A2874BAE65__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <psgLib\GirderLibraryEntry.h>

// This is an adapter class for IPrecastGirder which allows input of strands using 
// number of strands to fill continuously through the fill count
class CContinuousStandFiller  
{
public:
	CContinuousStandFiller();   
	virtual ~CContinuousStandFiller();

   void Init(const GirderLibraryEntry* pGdrEntry);

   // Set given number of strands directly in girder
   HRESULT SetStraightStrandCount(IPrecastGirder* girder,  StrandIndexType nStrands);
   HRESULT SetHarpedStrandCount(IPrecastGirder* girder,  StrandIndexType nStrands);
   HRESULT SetTemporaryStrandCount(IPrecastGirder* girder,  StrandIndexType nStrands);

   HRESULT IsValidNumStraightStrands(IPrecastGirder* girder, StrandIndexType nStrands,VARIANT_BOOL* bIsValid);
   HRESULT IsValidNumHarpedStrands(IPrecastGirder* girder, StrandIndexType nStrands,VARIANT_BOOL* bIsValid);
   HRESULT IsValidNumTemporaryStrands(IPrecastGirder* girder, StrandIndexType nStrands,VARIANT_BOOL* bIsValid);

   // Compute a fill array for a given number of straight or harped strands
   // NOTE: The strandFill array is for temporary use only - 
   //       do not change it - clone it if you want to hang on to it.
   //       And, it becomes invalid after subsequent calls into this class
   HRESULT ComputeStraightStrandFill(IPrecastGirder* girder, StrandIndexType nStrands, ILongArray** strandFill);
   HRESULT ComputeStraightStrandFill(IStrandGridFiller* pGridFiller, StrandIndexType nStrands, ILongArray** strandFill);
   HRESULT ComputeHarpedStrandFill(IPrecastGirder* girder, StrandIndexType nStrands, ILongArray** strandFill);
   HRESULT ComputeHarpedStrandFill(bool bAllowOddNumberOfHarpedStrands,IStrandGridFiller* pEndGridFiller,IStrandGridFiller* pHPGridFiller, StrandIndexType nStrands, ILongArray** strandFill);
   HRESULT ComputeTemporaryStrandFill(IPrecastGirder* girder, StrandIndexType nStrands, ILongArray** strandFill);
   HRESULT ComputeTemporaryStrandFill(IStrandGridFiller* pGridFiller, StrandIndexType nStrands, ILongArray** strandFill);

   HRESULT GetNextNumberOfStraightStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* nextStrands);
   HRESULT GetPrevNumberOfStraightStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* prevStrands);
   HRESULT GetNextNumberOfHarpedStrands(bool bAllowOddStrand,IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* nextStrands);
   HRESULT GetPrevNumberOfHarpedStrands(bool bAllowOddStrand,IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* prevStrands);
   HRESULT GetNextNumberOfTemporaryStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* nextStrands);
   HRESULT GetPrevNumberOfTemporaryStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* prevStrands);
   HRESULT GetNextNumberOfStraightStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* nextStrands);
   HRESULT GetNextNumberOfHarpedStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* nextStrands);
   HRESULT GetNextNumberOfTemporaryStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* nextStrands);
   HRESULT GetPreviousNumberOfStraightStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* prevStrands);
   HRESULT GetPreviousNumberOfHarpedStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* prevStrands);
   HRESULT GetPreviousNumberOfTemporaryStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* prevStrands);

   // computations for total number of permanent strands
   HRESULT GetMaxNumPermanentStrands(IPrecastGirder* girder, StrandIndexType* numStrands);
   HRESULT ComputeNumPermanentStrands(IPrecastGirder* girder, StrandIndexType totalPermanent, StrandIndexType* numStraight, StrandIndexType* numHarped);
   HRESULT GetNextNumberOfPermanentStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* nextStrands);
   HRESULT GetPreviousNumberOfPermanentStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* prevStrands);

private:

   // keep a local array handy so we don't need to allocate
   CComPtr<ILongArray> m_TempArray;  

   CComPtr<IStrandFillTool> m_StrandFillTool;

   const GirderLibraryEntry* m_pGdrEntry;

   // cache of permanent strand orders
   bool m_NeedToCompute;
   std::vector<GirderLibraryEntry::PermanentStrandType> m_PermStrands;

   void ValidatePermanent();
   HRESULT GetNextNumberOfStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* nextStrands);
   HRESULT GetPrevNumberOfStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* prevStrands);
};

#endif // !defined(AFX_CONTINUOUSSTANDFILLER_H__314FC998_ED67_4365_BBE9_63A2874BAE65__INCLUDED_)
