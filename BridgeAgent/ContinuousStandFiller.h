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

// ContinuousStandFiller.h: interface for the CContinuousStandFiller class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CONTINUOUSSTANDFILLER_H__314FC998_ED67_4365_BBE9_63A2874BAE65__INCLUDED_)
#define AFX_CONTINUOUSSTANDFILLER_H__314FC998_ED67_4365_BBE9_63A2874BAE65__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <PgsExt\StrandData.h>
#include <psgLib\GirderLibraryEntry.h>

// This is a pure virtual adapter class for IPrecastGirder which allows input of strands using 
// number of strands to fill continuously through the fill count
class CContinuousStrandFiller  
{
public:
   // Set given number of strands directly in girder
   virtual HRESULT SetStraightContinuousFill(IPrecastGirder* girder,  StrandIndexType nStrands) = 0;
   virtual HRESULT SetHarpedContinuousFill(IPrecastGirder* girder,  StrandIndexType nStrands) = 0;
   virtual HRESULT SetTemporaryContinuousFill(IPrecastGirder* girder,  StrandIndexType nStrands) = 0;

   virtual HRESULT IsValidNumStraightStrands(IPrecastGirder* girder, StrandIndexType nStrands,VARIANT_BOOL* bIsValid) = 0;
   virtual HRESULT IsValidNumStraightStrands(IStrandGridFiller* pGridFiller, StrandIndexType nStrands,VARIANT_BOOL* bIsValid) = 0;
   virtual HRESULT IsValidNumHarpedStrands(IPrecastGirder* girder, StrandIndexType nStrands,VARIANT_BOOL* bIsValid) = 0;
   virtual HRESULT IsValidNumHarpedStrands(bool bAllowOddNumberOfHarpedStrands,IStrandGridFiller* pEndGridFiller,IStrandGridFiller* pHPGridFiller, StrandIndexType nStrands, VARIANT_BOOL* bIsValid) = 0;
   virtual HRESULT IsValidNumTemporaryStrands(IPrecastGirder* girder, StrandIndexType nStrands,VARIANT_BOOL* bIsValid) = 0;
   virtual HRESULT IsValidNumTemporaryStrands(IStrandGridFiller* pGridFiller, StrandIndexType nStrands,VARIANT_BOOL* bIsValid) = 0;

   // Compute a fill array for a given number of straight or harped strands
   // NOTE: The strandFill array is for temporary use only - 
   //       do not change it - clone it if you want to hang on to it.
   //       And, it becomes invalid after subsequent calls into this class
   virtual HRESULT ComputeStraightStrandFill(IPrecastGirder* girder, StrandIndexType nStrands, IIndexArray** strandFill)= 0;
   virtual HRESULT ComputeStraightStrandFill(IStrandGridFiller* pGridFiller, StrandIndexType nStrands, IIndexArray** strandFill)= 0;
   virtual HRESULT ComputeHarpedStrandFill(IPrecastGirder* girder, StrandIndexType nStrands, IIndexArray** strandFill)= 0;
   virtual HRESULT ComputeHarpedStrandFill(bool bAllowOddNumberOfHarpedStrands,IStrandGridFiller* pEndGridFiller,IStrandGridFiller* pHPGridFiller, StrandIndexType nStrands, IIndexArray** strandFill)= 0;
   virtual HRESULT ComputeTemporaryStrandFill(IPrecastGirder* girder, StrandIndexType nStrands, IIndexArray** strandFill)= 0;
   virtual HRESULT ComputeTemporaryStrandFill(IStrandGridFiller* pGridFiller, StrandIndexType nStrands, IIndexArray** strandFill)= 0;

   virtual HRESULT GetNextNumberOfStraightStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* nextStrands) = 0;
   virtual HRESULT GetPrevNumberOfStraightStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* prevStrands) = 0;
   virtual HRESULT GetNextNumberOfHarpedStrands(bool bAllowOddStrand,IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* nextStrands) = 0;
   virtual HRESULT GetPrevNumberOfHarpedStrands(bool bAllowOddStrand,IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* prevStrands) = 0;
   virtual HRESULT GetNextNumberOfTemporaryStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* nextStrands) = 0;
   virtual HRESULT GetPrevNumberOfTemporaryStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* prevStrands) = 0;
   virtual HRESULT GetNextNumberOfStraightStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* nextStrands) = 0;
   virtual HRESULT GetNextNumberOfHarpedStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* nextStrands) = 0;
   virtual HRESULT GetNextNumberOfTemporaryStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* nextStrands) = 0;
   virtual HRESULT GetPreviousNumberOfStraightStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* prevStrands) = 0;
   virtual HRESULT GetPreviousNumberOfHarpedStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* prevStrands) = 0;
   virtual HRESULT GetPreviousNumberOfTemporaryStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* prevStrands) = 0;

   // computations for total number of permanent strands
   virtual HRESULT GetMaxNumPermanentStrands(IPrecastGirder* girder, StrandIndexType* numStrands) = 0;
   virtual HRESULT ComputeNumPermanentStrands(IPrecastGirder* girder, StrandIndexType totalPermanent, StrandIndexType* numStraight, StrandIndexType* numHarped) = 0;
   virtual HRESULT GetNextNumberOfPermanentStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* nextStrands) = 0;
   virtual HRESULT GetPreviousNumberOfPermanentStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* prevStrands) = 0;
};

// This is a pure virtual adapter class for IPrecastGirder which allows input of strands direct
// user-specified filling of strands
class CDirectStrandFiller  
{
public:
   // Set direct fill strands to girder
   virtual HRESULT SetStraightDirectStrandFill(IPrecastGirder* girder,  const CDirectStrandFillCollection* pCollection) = 0;
   virtual HRESULT SetHarpedDirectStrandFill(IPrecastGirder* girder,  const CDirectStrandFillCollection* pCollection) = 0;
   virtual HRESULT SetTemporaryDirectStrandFill(IPrecastGirder* girder,  const CDirectStrandFillCollection* pCollection) = 0;

   // Compute a WBFL-type fill array for a given CDirectStrandFillCollection
   // NOTE: The strandFill array is for temporary use only - 
   //       do not change it - clone it if you want to hang on to it.
   //       And, it becomes invalid after subsequent calls into this class
   virtual HRESULT ComputeStraightStrandFill(IPrecastGirder* girder, const CDirectStrandFillCollection* pCollection, IIndexArray** strandFill) = 0;
   virtual HRESULT ComputeHarpedStrandFill(IPrecastGirder* girder, const CDirectStrandFillCollection* pCollection, IIndexArray** strandFill) = 0;
   virtual HRESULT ComputeTemporaryStrandFill(IPrecastGirder* girder, const CDirectStrandFillCollection* pCollection, IIndexArray** strandFill) = 0;
};

class CStrandFiller : public CContinuousStrandFiller, public CDirectStrandFiller
{
public:
	CStrandFiller();   
	virtual ~CStrandFiller();

   void Init(const GirderLibraryEntry* pGdrEntry);

   // CContinuousStrandFiller implementations
   // Set given number of strands directly in girder
   HRESULT SetStraightContinuousFill(IPrecastGirder* girder,  StrandIndexType nStrands);
   HRESULT SetHarpedContinuousFill(IPrecastGirder* girder,  StrandIndexType nStrands);
   HRESULT SetTemporaryContinuousFill(IPrecastGirder* girder,  StrandIndexType nStrands);

   HRESULT IsValidNumStraightStrands(IPrecastGirder* girder, StrandIndexType nStrands,VARIANT_BOOL* bIsValid);
   HRESULT IsValidNumStraightStrands(IStrandGridFiller* pGridFiller, StrandIndexType nStrands,VARIANT_BOOL* bIsValid);
   HRESULT IsValidNumHarpedStrands(IPrecastGirder* girder, StrandIndexType nStrands,VARIANT_BOOL* bIsValid);
   HRESULT IsValidNumHarpedStrands(bool bAllowOddNumberOfHarpedStrands,IStrandGridFiller* pEndGridFiller,IStrandGridFiller* pHPGridFiller, StrandIndexType nStrands,VARIANT_BOOL* bIsValid);
   HRESULT IsValidNumTemporaryStrands(IPrecastGirder* girder, StrandIndexType nStrands,VARIANT_BOOL* bIsValid);
   HRESULT IsValidNumTemporaryStrands(IStrandGridFiller* pGridFiller, StrandIndexType nStrands,VARIANT_BOOL* bIsValid);

   // NOTE: The strandFill array is for temporary use only - 
   //       do not change it - clone it if you want to hang on to it.
   //       And, it becomes invalid after subsequent calls into this class
   HRESULT ComputeStraightStrandFill(IPrecastGirder* girder, StrandIndexType nStrands, IIndexArray** strandFill);
   HRESULT ComputeStraightStrandFill(IStrandGridFiller* pGridFiller, StrandIndexType nStrands, IIndexArray** strandFill);
   HRESULT ComputeHarpedStrandFill(IPrecastGirder* girder, StrandIndexType nStrands, IIndexArray** strandFill);
   HRESULT ComputeHarpedStrandFill(bool bAllowOddNumberOfHarpedStrands,IStrandGridFiller* pEndGridFiller,IStrandGridFiller* pHPGridFiller, StrandIndexType nStrands, IIndexArray** strandFill);
   HRESULT ComputeTemporaryStrandFill(IPrecastGirder* girder, StrandIndexType nStrands, IIndexArray** strandFill);
   HRESULT ComputeTemporaryStrandFill(IStrandGridFiller* pGridFiller, StrandIndexType nStrands, IIndexArray** strandFill);

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

   // CDirectStrandFiller implementations
   // NOTE: The strandFill array is for temporary use only - 
   //       do not change it - clone it if you want to hang on to it.
   //       And, it becomes invalid after subsequent calls into this class
   HRESULT ComputeStraightStrandFill(IPrecastGirder* girder, const CDirectStrandFillCollection* pCollection, IIndexArray** strandFill);
   HRESULT ComputeHarpedStrandFill(IPrecastGirder* girder, const CDirectStrandFillCollection* pCollection, IIndexArray** strandFill);
   HRESULT ComputeTemporaryStrandFill(IPrecastGirder* girder, const CDirectStrandFillCollection* pCollection, IIndexArray** strandFill);

   HRESULT SetStraightDirectStrandFill(IPrecastGirder* girder,  const CDirectStrandFillCollection* pCollection);
   HRESULT SetHarpedDirectStrandFill(IPrecastGirder* girder,  const CDirectStrandFillCollection* pCollection);
   HRESULT SetTemporaryDirectStrandFill(IPrecastGirder* girder,  const CDirectStrandFillCollection* pCollection);

private:

   // keep a local array handy so we don't need to allocate
   CComPtr<IIndexArray> m_TempArray;  

   CComPtr<IStrandFillTool> m_StrandFillTool;

   const GirderLibraryEntry* m_pGdrEntry;

   // cache of permanent strand orders
   bool m_NeedToCompute;
   std::vector<GirderLibraryEntry::StrandDefinitionType> m_PermStrands;

   void ValidatePermanent();
   HRESULT GetNextNumberOfStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* nextStrands);
   HRESULT GetPrevNumberOfStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* prevStrands);
};

#endif // !defined(AFX_CONTINUOUSSTANDFILLER_H__314FC998_ED67_4365_BBE9_63A2874BAE65__INCLUDED_)
