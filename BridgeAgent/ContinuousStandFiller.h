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
   virtual HRESULT SetStraightContinuousFill(IStrandGridModel* pStrandGridModel,  StrandIndexType nStrands) = 0;
   virtual HRESULT SetHarpedContinuousFill(IStrandGridModel* pStrandGridModel,  StrandIndexType nStrands) = 0;
   virtual HRESULT SetTemporaryContinuousFill(IStrandGridModel* pStrandGridModel,  StrandIndexType nStrands) = 0;

   virtual HRESULT IsValidNumStraightStrands(IStrandGridModel* pStrandGridModel, StrandIndexType nStrands,VARIANT_BOOL* bIsValid) const = 0;
   virtual HRESULT IsValidNumStraightStrands(IStrandGridFiller* pGridFiller, StrandIndexType nStrands,VARIANT_BOOL* bIsValid) const = 0;
   virtual HRESULT IsValidNumHarpedStrands(IStrandGridModel* pStrandGridModel, StrandIndexType nStrands,VARIANT_BOOL* bIsValid) const = 0;
   virtual HRESULT IsValidNumHarpedStrands(bool bAllowOddNumberOfHarpedStrands,IStrandGridFiller* pEndGridFiller,IStrandGridFiller* pHPGridFiller, StrandIndexType nStrands, VARIANT_BOOL* bIsValid) const = 0;
   virtual HRESULT IsValidNumTemporaryStrands(IStrandGridModel* pStrandGridModel, StrandIndexType nStrands,VARIANT_BOOL* bIsValid) const = 0;
   virtual HRESULT IsValidNumTemporaryStrands(IStrandGridFiller* pGridFiller, StrandIndexType nStrands,VARIANT_BOOL* bIsValid) const = 0;

   // Compute a fill array for a given number of straight or harped strands
   // NOTE: The strandFill array is for temporary use only - 
   //       do not change it - clone it if you want to hang on to it.
   //       And, it becomes invalid after subsequent calls into this class
   virtual HRESULT ComputeStraightStrandFill(IStrandGridModel* pStrandGridModel, StrandIndexType nStrands, IIndexArray** strandFill) const = 0;
   virtual HRESULT ComputeStraightStrandFill(IStrandGridFiller* pGridFiller, StrandIndexType nStrands, IIndexArray** strandFill) const = 0;
   virtual HRESULT ComputeHarpedStrandFill(IStrandGridModel* pStrandGridModel, StrandIndexType nStrands, IIndexArray** strandFill) const = 0;
   virtual HRESULT ComputeHarpedStrandFill(bool bAllowOddNumberOfHarpedStrands,IStrandGridFiller* pEndGridFiller,IStrandGridFiller* pHPGridFiller, StrandIndexType nStrands, IIndexArray** strandFill) const = 0;
   virtual HRESULT ComputeTemporaryStrandFill(IStrandGridModel* pStrandGridModel, StrandIndexType nStrands, IIndexArray** strandFill) const = 0;
   virtual HRESULT ComputeTemporaryStrandFill(IStrandGridFiller* pGridFiller, StrandIndexType nStrands, IIndexArray** strandFill) const = 0;

   virtual HRESULT GetNextNumberOfStraightStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* nextStrands) const = 0;
   virtual HRESULT GetPrevNumberOfStraightStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* prevStrands) const = 0;
   virtual HRESULT GetNextNumberOfHarpedStrands(bool bAllowOddStrand,IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* nextStrands) const = 0;
   virtual HRESULT GetPrevNumberOfHarpedStrands(bool bAllowOddStrand,IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* prevStrands) const = 0;
   virtual HRESULT GetNextNumberOfTemporaryStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* nextStrands) const = 0;
   virtual HRESULT GetPrevNumberOfTemporaryStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* prevStrands) const = 0;
   virtual HRESULT GetNextNumberOfStraightStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* nextStrands) const = 0;
   virtual HRESULT GetNextNumberOfHarpedStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* nextStrands) const = 0;
   virtual HRESULT GetNextNumberOfTemporaryStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* nextStrands) const = 0;
   virtual HRESULT GetPreviousNumberOfStraightStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* prevStrands) const = 0;
   virtual HRESULT GetPreviousNumberOfHarpedStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* prevStrands) const = 0;
   virtual HRESULT GetPreviousNumberOfTemporaryStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* prevStrands) const = 0;

   // computations for total number of permanent strands
   virtual HRESULT GetMaxNumPermanentStrands(IStrandGridModel* pStrandGridModel, StrandIndexType* numStrands) const = 0;
   virtual HRESULT ComputeNumPermanentStrands(IStrandGridModel* pStrandGridModel, StrandIndexType totalPermanent, StrandIndexType* numStraight, StrandIndexType* numHarped) const = 0;
   virtual HRESULT GetNextNumberOfPermanentStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* nextStrands) const = 0;
   virtual HRESULT GetPreviousNumberOfPermanentStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* prevStrands) const = 0;
};

// This is a pure virtual adapter class for IPrecastGirder which allows input of strands direct
// user-specified filling of strands
class CDirectStrandFiller  
{
public:
   // Set direct fill strands to girder
   virtual HRESULT SetStraightDirectStrandFill(IStrandGridModel* pStrandGridModel,  const CDirectStrandFillCollection* pCollection) = 0;
   virtual HRESULT SetHarpedDirectStrandFill(IStrandGridModel* pStrandGridModel,  const CDirectStrandFillCollection* pCollection) = 0;
   virtual HRESULT SetTemporaryDirectStrandFill(IStrandGridModel* pStrandGridModel,  const CDirectStrandFillCollection* pCollection) = 0;

   // Compute a WBFL-type fill array for a given CDirectStrandFillCollection
   // NOTE: The strandFill array is for temporary use only - 
   //       do not change it - clone it if you want to hang on to it.
   //       And, it becomes invalid after subsequent calls into this class
   virtual HRESULT ComputeStraightStrandFill(IStrandGridModel* pStrandGridModel, const CDirectStrandFillCollection* pCollection, IIndexArray** strandFill) const = 0;
   virtual HRESULT ComputeHarpedStrandFill(IStrandGridModel* pStrandGridModel, const CDirectStrandFillCollection* pCollection, IIndexArray** strandFill) const = 0;
   virtual HRESULT ComputeTemporaryStrandFill(IStrandGridModel* pStrandGridModel, const CDirectStrandFillCollection* pCollection, IIndexArray** strandFill) const = 0;
};

class CStrandFiller : public CContinuousStrandFiller, public CDirectStrandFiller
{
public:
	CStrandFiller();   
	virtual ~CStrandFiller();

   virtual void Init(const GirderLibraryEntry* pGdrEntry);

   // CContinuousStrandFiller implementations
   // Set given number of strands directly in girder
   virtual HRESULT SetStraightContinuousFill(IStrandGridModel* pStrandGridModel,  StrandIndexType nStrands) override;
   virtual HRESULT SetHarpedContinuousFill(IStrandGridModel* pStrandGridModel,  StrandIndexType nStrands) override;
   virtual HRESULT SetTemporaryContinuousFill(IStrandGridModel* pStrandGridModel,  StrandIndexType nStrands) override;

   virtual HRESULT IsValidNumStraightStrands(IStrandGridModel* pStrandGridModel, StrandIndexType nStrands,VARIANT_BOOL* bIsValid) const override;
   virtual HRESULT IsValidNumStraightStrands(IStrandGridFiller* pGridFiller, StrandIndexType nStrands,VARIANT_BOOL* bIsValid) const override;
   virtual HRESULT IsValidNumHarpedStrands(IStrandGridModel* pStrandGridModel, StrandIndexType nStrands,VARIANT_BOOL* bIsValid) const override;
   virtual HRESULT IsValidNumHarpedStrands(bool bAllowOddNumberOfHarpedStrands,IStrandGridFiller* pEndGridFiller,IStrandGridFiller* pHPGridFiller, StrandIndexType nStrands,VARIANT_BOOL* bIsValid) const override;
   virtual HRESULT IsValidNumTemporaryStrands(IStrandGridModel* pStrandGridModel, StrandIndexType nStrands,VARIANT_BOOL* bIsValid) const override;
   virtual HRESULT IsValidNumTemporaryStrands(IStrandGridFiller* pGridFiller, StrandIndexType nStrands,VARIANT_BOOL* bIsValid) const override;

   // NOTE: The strandFill array is for temporary use only - 
   //       do not change it - clone it if you want to hang on to it.
   //       And, it becomes invalid after subsequent calls into this class
   virtual HRESULT ComputeStraightStrandFill(IStrandGridModel* pStrandGridModel, StrandIndexType nStrands, IIndexArray** strandFill) const override;
   virtual HRESULT ComputeStraightStrandFill(IStrandGridFiller* pGridFiller, StrandIndexType nStrands, IIndexArray** strandFill) const override;
   virtual HRESULT ComputeHarpedStrandFill(IStrandGridModel* pStrandGridModel, StrandIndexType nStrands, IIndexArray** strandFill) const override;
   virtual HRESULT ComputeHarpedStrandFill(bool bAllowOddNumberOfHarpedStrands,IStrandGridFiller* pEndGridFiller,IStrandGridFiller* pHPGridFiller, StrandIndexType nStrands, IIndexArray** strandFill) const override;
   virtual HRESULT ComputeTemporaryStrandFill(IStrandGridModel* pStrandGridModel, StrandIndexType nStrands, IIndexArray** strandFill) const override;
   virtual HRESULT ComputeTemporaryStrandFill(IStrandGridFiller* pGridFiller, StrandIndexType nStrands, IIndexArray** strandFill) const override;

   virtual HRESULT GetNextNumberOfStraightStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* nextStrands) const override;
   virtual HRESULT GetPrevNumberOfStraightStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* prevStrands) const override;
   virtual HRESULT GetNextNumberOfHarpedStrands(bool bAllowOddStrand,IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* nextStrands) const override;
   virtual HRESULT GetPrevNumberOfHarpedStrands(bool bAllowOddStrand,IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* prevStrands) const override;
   virtual HRESULT GetNextNumberOfTemporaryStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* nextStrands) const override;
   virtual HRESULT GetPrevNumberOfTemporaryStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* prevStrands) const override;
   virtual HRESULT GetNextNumberOfStraightStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* nextStrands) const override;
   virtual HRESULT GetNextNumberOfHarpedStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* nextStrands) const override;
   virtual HRESULT GetNextNumberOfTemporaryStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* nextStrands) const override;
   virtual HRESULT GetPreviousNumberOfStraightStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* prevStrands) const override;
   virtual HRESULT GetPreviousNumberOfHarpedStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* prevStrands) const override;
   virtual HRESULT GetPreviousNumberOfTemporaryStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* prevStrands) const override;

   // computations for total number of permanent strands
   virtual HRESULT GetMaxNumPermanentStrands(IStrandGridModel* pStrandGridModel, StrandIndexType* numStrands) const override;
   virtual HRESULT ComputeNumPermanentStrands(IStrandGridModel* pStrandGridModel, StrandIndexType totalPermanent, StrandIndexType* numStraight, StrandIndexType* numHarped) const override;
   virtual HRESULT GetNextNumberOfPermanentStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* nextStrands) const override;
   virtual HRESULT GetPreviousNumberOfPermanentStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* prevStrands) const override;

   // CDirectStrandFiller implementations
   // NOTE: The strandFill array is for temporary use only - 
   //       do not change it - clone it if you want to hang on to it.
   //       And, it becomes invalid after subsequent calls into this class
   virtual HRESULT ComputeStraightStrandFill(IStrandGridModel* pStrandGridModel, const CDirectStrandFillCollection* pCollection, IIndexArray** strandFill) const override;
   virtual HRESULT ComputeHarpedStrandFill(IStrandGridModel* pStrandGridModel, const CDirectStrandFillCollection* pCollection, IIndexArray** strandFill) const override;
   virtual HRESULT ComputeTemporaryStrandFill(IStrandGridModel* pStrandGridModel, const CDirectStrandFillCollection* pCollection, IIndexArray** strandFill) const override;

   virtual HRESULT SetStraightDirectStrandFill(IStrandGridModel* pStrandGridModel,  const CDirectStrandFillCollection* pCollection) override;
   virtual HRESULT SetHarpedDirectStrandFill(IStrandGridModel* pStrandGridModel,  const CDirectStrandFillCollection* pCollection) override;
   virtual HRESULT SetTemporaryDirectStrandFill(IStrandGridModel* pStrandGridModel,  const CDirectStrandFillCollection* pCollection) override;

private:

   // keep a local array handy so we don't need to allocate
   mutable CComPtr<IIndexArray> m_TempArray;  

   CComPtr<IStrandFillTool> m_StrandFillTool;

   const GirderLibraryEntry* m_pGdrEntry;

   // cache of permanent strand orders
   mutable bool m_NeedToCompute;
   mutable std::vector<GirderLibraryEntry::StrandDefinitionType> m_PermStrands;

   void ValidatePermanent() const;
   HRESULT GetNextNumberOfStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* nextStrands) const;
   HRESULT GetPrevNumberOfStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* prevStrands) const;
};

#endif // !defined(AFX_CONTINUOUSSTANDFILLER_H__314FC998_ED67_4365_BBE9_63A2874BAE65__INCLUDED_)
