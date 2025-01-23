///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

// ContinuousStrandFiller.cpp: implementation of the CStrandFiller class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ContinuousStandFiller.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// some usefull Free functions
static HRESULT GetNextNumStrands(StrandIndexType currNum, IIndexArray* array, StrandIndexType* nextNum)
{
   if (currNum == INVALID_INDEX)
   {
      return E_INVALIDARG;
   }

   StrandIndexType running_cnt=0;
   IndexType size;
   array->get_Count(&size);
   for (IndexType ip = 0; ip < size && running_cnt <= currNum; ip++)
   {
      IndexType val;
      array->get_Item(ip, &val);

      running_cnt += (StrandIndexType)val;
   }
   
   if (currNum < running_cnt)
   {
      *nextNum = running_cnt;
   }
   else
   {
      // no more spaces
      *nextNum = INVALID_INDEX;
   }

   return S_OK;
}

static HRESULT GetPrevNumStrands(StrandIndexType currNum, IIndexArray* array, StrandIndexType* prevNum)
{
   if (currNum <= 0 || currNum == INVALID_INDEX)
   {
      ATLASSERT(false);
      return E_INVALIDARG;
   }
   else if (currNum == 1)
   {
      *prevNum = 0;
      return S_OK;
   }
   else
   {
      StrandIndexType running_cnt=0;
      IndexType size;
      array->get_Count(&size);
      for (IndexType ip = 0; ip < size; ip++)
      {
         StrandIndexType prev_cnt = running_cnt;

         IndexType val;
         array->get_Item(ip,&val);

         running_cnt += (StrandIndexType)val;

         if (currNum <= running_cnt)
         {
            *prevNum = prev_cnt;
            return S_OK;
         }
      }

      // currNum is greater than max number of strands (doesn't matter by how much)
      // return max number
      *prevNum = running_cnt;

      return S_OK;
   }
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStrandFiller::CStrandFiller():
m_pGdrEntry(nullptr),
m_NeedToCompute(true)
{
   m_TempArray.CoCreateInstance(CLSID_IndexArray);

   // typical girders will not have this many strands
   m_TempArray->Reserve(128);

   HRESULT hr = m_StrandFillTool.CoCreateInstance(CLSID_StrandFillTool);
}

CStrandFiller::~CStrandFiller()
{
}

void CStrandFiller::Init(const GirderLibraryEntry* pGdrEntry)
{
   // Note - this function can be called multiple times.
   m_pGdrEntry = pGdrEntry;
   m_NeedToCompute = true;
}

void CStrandFiller::ValidatePermanent() const
{
   // permanent strand ordering is nasty. compute once and reuse.
   if(m_NeedToCompute)
   {
     m_PermStrands = m_pGdrEntry->GetPermanentStrands();

      m_NeedToCompute = false;
   }
}

// place strands using continuous fill order
HRESULT CStrandFiller::SetStraightContinuousFill(IStrandGridModel* pStrandGridModel, StrandIndexType nStrands)
{
   CComPtr<IIndexArray> array;
   HRESULT hr = ComputeStraightStrandFill(pStrandGridModel,nStrands, &array);
   if (FAILED(hr))
   {
      ATLASSERT(false);
      return hr;
   }

   hr = pStrandGridModel->putref_StrandFill(Straight,array);
   if (FAILED(hr))
   {
      ATLASSERT(false);
      return hr;
   }

   return S_OK;
}

HRESULT CStrandFiller::SetHarpedContinuousFill(IStrandGridModel* pStrandGridModel,  StrandIndexType nStrands)
{
   CComPtr<IIndexArray> array;
   HRESULT hr = ComputeHarpedStrandFill(pStrandGridModel,nStrands, &array);
   if (FAILED(hr))
   {
      ATLASSERT(false);
      return hr;
   }

   hr = pStrandGridModel->putref_StrandFill(Harped,array);
   if (FAILED(hr))
   {
      ATLASSERT(false);
      return hr;
   }

   return S_OK;
}

HRESULT CStrandFiller::SetTemporaryContinuousFill(IStrandGridModel* pStrandGridModel, StrandIndexType nStrands)
{
   CComPtr<IIndexArray> array;
   HRESULT hr = ComputeTemporaryStrandFill(pStrandGridModel,nStrands, &array);
   if (FAILED(hr))
   {
      ATLASSERT(false);
      return hr;
   }

   hr = pStrandGridModel->putref_StrandFill(Temporary,array);
   if (FAILED(hr))
   {
      ATLASSERT(false);
      return hr;
   }

   return S_OK;
}

HRESULT CStrandFiller::IsValidNumStraightStrands(IStrandGridModel* pStrandGridModel, StrandIndexType nStrands,VARIANT_BOOL* bIsValid) const
{
   CComPtr<IIndexArray> array;
   HRESULT hr = ComputeStraightStrandFill(pStrandGridModel,nStrands,&array);
   *bIsValid = SUCCEEDED(hr) ? VARIANT_TRUE : VARIANT_FALSE;
   return S_OK;
}

HRESULT CStrandFiller::IsValidNumHarpedStrands(IStrandGridModel* pStrandGridModel, StrandIndexType nStrands,VARIANT_BOOL* bIsValid) const
{
   CComPtr<IIndexArray> array;
   HRESULT hr = ComputeHarpedStrandFill(pStrandGridModel,nStrands,&array);
   *bIsValid = SUCCEEDED(hr) ? VARIANT_TRUE : VARIANT_FALSE;
   return S_OK;
}

HRESULT CStrandFiller::IsValidNumTemporaryStrands(IStrandGridModel* pStrandGridModel, StrandIndexType nStrands,VARIANT_BOOL* bIsValid) const
{
   CComPtr<IIndexArray> array;
   HRESULT hr = ComputeTemporaryStrandFill(pStrandGridModel,nStrands,&array);
   *bIsValid = SUCCEEDED(hr) ? VARIANT_TRUE : VARIANT_FALSE;
   return S_OK;
}

HRESULT CStrandFiller::IsValidNumStraightStrands(IStrandGridFiller* pGridFiller, StrandIndexType nStrands,VARIANT_BOOL* bIsValid) const
{
   CComPtr<IIndexArray> array;
   HRESULT hr = ComputeStraightStrandFill(pGridFiller,nStrands,&array);
   *bIsValid = SUCCEEDED(hr) ? VARIANT_TRUE : VARIANT_FALSE;
   return S_OK;
}

HRESULT CStrandFiller::IsValidNumHarpedStrands(bool bAllowOddNumberOfHarpedStrands,IStrandGridFiller* pEndGridFiller,IStrandGridFiller* pHPGridFiller, StrandIndexType nStrands,VARIANT_BOOL* bIsValid) const
{
   CComPtr<IIndexArray> array;
   HRESULT hr = ComputeHarpedStrandFill(bAllowOddNumberOfHarpedStrands,pEndGridFiller,pHPGridFiller,nStrands,&array);
   *bIsValid = SUCCEEDED(hr) ? VARIANT_TRUE : VARIANT_FALSE;
   return S_OK;
}

HRESULT CStrandFiller::IsValidNumTemporaryStrands(IStrandGridFiller* pGridFiller, StrandIndexType nStrands,VARIANT_BOOL* bIsValid) const
{
   CComPtr<IIndexArray> array;
   HRESULT hr = ComputeTemporaryStrandFill(pGridFiller,nStrands,&array);
   *bIsValid = SUCCEEDED(hr) ? VARIANT_TRUE : VARIANT_FALSE;
   return S_OK;
}

HRESULT CStrandFiller::ComputeStraightStrandFill(IStrandGridModel* pStrandGridModel, StrandIndexType nStrands, IIndexArray** strandFill) const
{
   CComPtr<IStrandGrid> grid;
   pStrandGridModel->get_StraightStrandGrid(etStart,&grid);

   CComQIPtr<IStrandGridFiller> gridFiller(grid);
   return ComputeStraightStrandFill(gridFiller,nStrands,strandFill);
}

HRESULT CStrandFiller::ComputeStraightStrandFill(IStrandGridFiller* pGridFiller, StrandIndexType nStrands, IIndexArray** strandFill) const
{
   CComPtr<IIndexArray> array;
   HRESULT hr = pGridFiller->GetMaxStrandFill(&array);
   if (FAILED(hr))
   {
      ATLASSERT(false);
      return hr;
   }

   IndexType size;
   array->get_Count(&size);

   // Array is size of max fill
   m_TempArray->ReDim(size);

   if (0 < nStrands)
   {
      IndexType cnt=0;
      for (IndexType ip = 0; ip < size; ip++)
      {
         IndexType val;
         array->get_Item(ip, &val);

         m_TempArray->put_Item(ip, val);
         cnt += val;
      
         if (nStrands <= cnt)
         {
            m_TempArray.CopyTo(strandFill);
            return (cnt == nStrands ? S_OK : S_FALSE); // S_FALSE means that it was filled as much as possible
         }
      }

      // not enough strand locations available in girder to fit
      m_TempArray.CopyTo(strandFill);
      return S_FALSE;
   }
   else
   {
      // no strands, return empty array
      m_TempArray->Assign(size, 0);
      return m_TempArray.CopyTo(strandFill);
   }
}

HRESULT CStrandFiller::ComputeHarpedStrandFill(IStrandGridModel* pStrandGridModel, StrandIndexType nStrands, IIndexArray** strandFill) const
{
   CComPtr<IStrandGrid> startGrid, hpGrid;
   pStrandGridModel->get_HarpedStrandGridEnd(etStart,&startGrid);
   pStrandGridModel->get_HarpedStrandGridHP(etStart,&hpGrid);

   VARIANT_BOOL vb;
   pStrandGridModel->get_AllowOddNumberOfHarpedStrands(&vb);

   CComQIPtr<IStrandGridFiller> startGridFiller(startGrid);
   CComQIPtr<IStrandGridFiller> hpGridFiller(hpGrid);
   return ComputeHarpedStrandFill(vb == VARIANT_TRUE ? true : false,startGridFiller,hpGridFiller,nStrands,strandFill);
}

HRESULT CStrandFiller::ComputeHarpedStrandFill(bool bAllowOddNumberOfHarpedStrands,IStrandGridFiller* pEndGridFiller, IStrandGridFiller* pHPGridFiller, StrandIndexType nStrands, IIndexArray** strandFill) const
{
   StrandIndexType nMaxStrands;
   CComPtr<IIndexArray> array;
   m_StrandFillTool->ComputeHarpedStrandMaxFill(pEndGridFiller,pHPGridFiller,&nMaxStrands,&array);

   IndexType size;
   array->get_Count(&size);

   // Array is size of max fill
   m_TempArray->ReDim(size);

   if (0 < nStrands)
   {
      IndexType cnt=0;
      for (IndexType ip = 0; ip < size; ip++)
      {
         IndexType val;
         array->get_Item(ip, &val);

         m_TempArray->put_Item(ip, val);
         cnt += val;
      
         if (nStrands <= cnt)
         {
            if (cnt == nStrands)
            {
               // nailed it - return
               return m_TempArray.CopyTo(strandFill);
            }
            else
            {
               // For harped strands, we might be able to fit this extra strand...
               if (bAllowOddNumberOfHarpedStrands)
               {
                  // This puts the odd strand at the beginning of the fill sequence
                  m_TempArray->get_Item(0,&val);
                  if (val == 2)
                  {
                     m_TempArray->put_Item(0,1);
                     return m_TempArray.CopyTo(strandFill);
                  }
               }

               // Nope, doesn't fit
               ATLASSERT(false);
               return E_INVALIDARG;
            }
         }
      }

      // not enough strand locations available in girder to fit
      //ATLASSERT(false);
      return E_INVALIDARG;
   }
   else
   {
      // no strands - return zeroed array
      m_TempArray->Assign(size, 0);
      return m_TempArray.CopyTo(strandFill);
   }
}


HRESULT CStrandFiller::ComputeTemporaryStrandFill(IStrandGridModel* pStrandGridModel, StrandIndexType nStrands, IIndexArray** strandFill) const
{
   CComPtr<IStrandGrid> grid;
   pStrandGridModel->get_TemporaryStrandGrid(etStart,&grid);

   CComQIPtr<IStrandGridFiller> gridFiller(grid);
   return ComputeTemporaryStrandFill(gridFiller,nStrands,strandFill);
}

HRESULT CStrandFiller::ComputeTemporaryStrandFill(IStrandGridFiller* pGridFiller, StrandIndexType nStrands, IIndexArray** strandFill) const
{
   CComPtr<IIndexArray> array;
   HRESULT hr = pGridFiller->GetMaxStrandFill(&array);
   if (FAILED(hr))
   {
      ATLASSERT(false);
      return hr;
   }

   IndexType size;
   array->get_Count(&size);

   // Array is size of max fill
   m_TempArray->ReDim(size);

   if (0 < nStrands)
   {

      IndexType cnt=0;
      for (IndexType ip = 0; ip < size; ip++)
      {
         IndexType val;
         array->get_Item(ip, &val);

         m_TempArray->put_Item(ip, val);
         cnt+=val;
      
         if (nStrands <= cnt)
         {
            if (cnt == nStrands)
            {
               // only safe way out
               return m_TempArray.CopyTo(strandFill);
            }
            else
            {
               // nStrands does not fit into this array
               //ATLASSERT(false);
               return E_INVALIDARG;
            }
         }
      }

      // not enough strand locations available in girder to fit
//      ATLASSERT(false);
      return E_INVALIDARG;
   }
   else
   {
      // no strands, return empty
      m_TempArray->Assign(size, 0);
      return m_TempArray.CopyTo(strandFill);
   }
}





HRESULT CStrandFiller::GetNextNumberOfStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* nextStrands) const
{
   CComPtr<IIndexArray> array;
   HRESULT hr = pGridFiller->GetMaxStrandFill(&array);
   if (FAILED(hr))
   {
      ATLASSERT(false);
      return hr;
   }

   return GetNextNumStrands(currNum, array, nextStrands);
}

HRESULT CStrandFiller::GetPrevNumberOfStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* prevStrands) const
{
   CComPtr<IIndexArray> array;
   HRESULT hr = pGridFiller->GetMaxStrandFill(&array);
   if (FAILED(hr))
   {
      ATLASSERT(false);
      return hr;
   }

   return GetPrevNumStrands(currNum, array, prevStrands);
}

HRESULT CStrandFiller::GetNextNumberOfStraightStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* nextStrands) const
{
   return GetNextNumberOfStrands(pGridFiller,currNum,nextStrands);
}

HRESULT CStrandFiller::GetPrevNumberOfStraightStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* prevStrands) const
{
   return GetPrevNumberOfStrands(pGridFiller,currNum,prevStrands);
}

HRESULT CStrandFiller::GetNextNumberOfHarpedStrands(bool bAllowOddStrand,IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* nextStrands) const
{
   CComPtr<IIndexArray> array;
   HRESULT hr = pGridFiller->GetMaxStrandFill(&array);
   if (FAILED(hr))
   {
      ATLASSERT(false);
      return hr;
   }

   IndexType size;
   array->get_Count(&size);
   if (size == 0)
   {
      *nextStrands = INVALID_INDEX;
      return S_OK;
   }

   if (bAllowOddStrand)
   {
      IndexType val;
      array->get_Item(0,&val);
      if (val == 2)
      {
         // we can reduce by one, so any number less than max is possible
         StrandIndexType max_harped;
         HRESULT hr = pGridFiller->get_MaxStrandCount(&max_harped);
         if (FAILED(hr))
         {
            ATLASSERT(false);
            return hr;
         }

         if (currNum < max_harped)
         {
            *nextStrands = currNum+1;
            return S_OK;
         }
         else
         {
            *nextStrands = INVALID_INDEX;
            return S_OK;
         }
      }
      // else we have to go through normal strand logic below
   }

   // odd strands didn't help us out

   return GetNextNumStrands(currNum, array, nextStrands);
}

HRESULT CStrandFiller::GetPrevNumberOfHarpedStrands(bool bAllowOddStrand,IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* prevStrands) const
{
   // get the no-brainers out of the way
   if (currNum <= 0)
   {
      ATLASSERT(false);
      return E_INVALIDARG;
   }
   else if (currNum == 1)
   {
      *prevStrands = 0;
      return S_OK;
   }

   CComPtr<IIndexArray> array;
   HRESULT hr = pGridFiller->GetMaxStrandFill(&array);
   if (FAILED(hr))
   {
      ATLASSERT(false);
      return hr;
   }

   // possible that odd strands will help us out
   if (bAllowOddStrand)
   {
      IndexType val;
      array->get_Item(0,&val);
      if (val == 2)
      {
         // we can reduce by one, so any number less than max is possible
         StrandIndexType max_harped;
         HRESULT hr = pGridFiller->get_MaxStrandCount(&max_harped);
         if (FAILED(hr))
         {
            ATLASSERT(false);
            return hr;
         }

         if (currNum < max_harped+1)
         {
            *prevStrands = currNum-1;
            return S_OK;
         }
         else
         {
            ATLASSERT(false);
            return hr;
         }
      }
      // else we have to go through normal strand logic below
   }

   return GetPrevNumStrands(currNum, array, prevStrands);
}

HRESULT CStrandFiller::GetNextNumberOfTemporaryStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* nextStrands) const
{
   return GetNextNumberOfStrands(pGridFiller,currNum,nextStrands);
}

HRESULT CStrandFiller::GetPrevNumberOfTemporaryStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* prevStrands) const
{
   return GetPrevNumberOfStrands(pGridFiller,currNum,prevStrands);
}









HRESULT CStrandFiller::GetNextNumberOfStraightStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* nextStrands) const
{
   CComPtr<IIndexArray> array;
   HRESULT hr = pStrandGridModel->GetMaxStrandFill(Straight,&array);
   if (FAILED(hr))
   {
      ATLASSERT(false);
      return hr;
   }

   return GetNextNumStrands(currNum, array, nextStrands);
}

HRESULT CStrandFiller::GetNextNumberOfHarpedStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* nextStrands) const
{
   CComPtr<IIndexArray> array;
   HRESULT hr = pStrandGridModel->GetMaxStrandFill(Harped,&array);
   if (FAILED(hr))
   {
      ATLASSERT(false);
      return hr;
   }

   IndexType size;
   array->get_Count(&size);
   if (size == 0)
   {
      *nextStrands = INVALID_INDEX;
      return S_OK;
   }

   VARIANT_BOOL allow;
   pStrandGridModel->get_AllowOddNumberOfHarpedStrands(&allow);
   if (allow == VARIANT_TRUE)
   {
      IndexType val;
      array->get_Item(0,&val);
      if (val == 2)
      {
         // we can reduce by one, so any number less than max is possible
         StrandIndexType max_harped;
         HRESULT hr = pStrandGridModel->GetMaxStrands(Harped,&max_harped);
         if (FAILED(hr))
         {
            ATLASSERT(false);
            return hr;
         }

         if (currNum < max_harped)
         {
            *nextStrands = currNum+1;
            return S_OK;
         }
         else
         {
            *nextStrands = INVALID_INDEX;
            return S_OK;
         }
      }
      // else we have to go through normal strand logic below
   }

   // odd strands didn't help us out

   return GetNextNumStrands(currNum, array, nextStrands);
}

HRESULT CStrandFiller::GetNextNumberOfTemporaryStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* nextStrands) const
{
   CComPtr<IIndexArray> array;
   HRESULT hr = pStrandGridModel->GetMaxStrandFill(Temporary,&array);
   if (FAILED(hr))
   {
      ATLASSERT(false);
      return hr;
   }

   return GetNextNumStrands(currNum, array, nextStrands);
}

HRESULT CStrandFiller::GetPreviousNumberOfStraightStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* prevStrands) const
{
   CComPtr<IIndexArray> array;
   HRESULT hr = pStrandGridModel->GetMaxStrandFill(Straight,&array);
   if (FAILED(hr))
   {
      ATLASSERT(false);
      return hr;
   }

   return GetPrevNumStrands(currNum, array, prevStrands);
}

HRESULT CStrandFiller::GetPreviousNumberOfHarpedStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* prevStrands) const
{
   // get the no-brainers out of the way
   if (currNum <= 0)
   {
      ATLASSERT(false);
      return E_INVALIDARG;
   }
   else if (currNum == 1)
   {
      *prevStrands = 0;
      return S_OK;
   }

   CComPtr<IIndexArray> array;
   HRESULT hr = pStrandGridModel->GetMaxStrandFill(Harped,&array);
   if (FAILED(hr))
   {
      ATLASSERT(false);
      return hr;
   }

   // possible that odd strands will help us out
   VARIANT_BOOL allow;
   pStrandGridModel->get_AllowOddNumberOfHarpedStrands(&allow);
   if (allow == VARIANT_TRUE)
   {
      IndexType val;
      array->get_Item(0,&val);
      if (val == 2)
      {
         // we can reduce by one, so any number less than max is possible
         StrandIndexType max_harped;
         HRESULT hr = pStrandGridModel->GetMaxStrands(Harped,&max_harped);
         if (FAILED(hr))
         {
            ATLASSERT(false);
            return hr;
         }

         if (currNum < max_harped+1)
         {
            *prevStrands = currNum-1;
            return S_OK;
         }
         else
         {
            ATLASSERT(false);
            return hr;
         }
      }
      // else we have to go through normal strand logic below
   }

   return GetPrevNumStrands(currNum, array, prevStrands);
}

HRESULT CStrandFiller::GetPreviousNumberOfTemporaryStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* prevStrands) const
{
   CComPtr<IIndexArray> array;
   HRESULT hr = pStrandGridModel->GetMaxStrandFill(Temporary,&array);
   if (FAILED(hr))
   {
      ATLASSERT(false);
      return hr;
   }

   return GetPrevNumStrands(currNum, array, prevStrands);
}

HRESULT CStrandFiller::GetMaxNumPermanentStrands(IStrandGridModel* pStrandGridModel, StrandIndexType* numStrands) const
{
   ValidatePermanent();

   *numStrands = m_PermStrands.size()-1;

   return S_OK;
}

HRESULT CStrandFiller::ComputeNumPermanentStrands(IStrandGridModel* pStrandGridModel, StrandIndexType totalPermanent, StrandIndexType* numStraight, StrandIndexType* numHarped) const
{
   return m_pGdrEntry->GetPermStrandDistribution(totalPermanent, numStraight, numHarped) ? S_OK : S_FALSE;
}

HRESULT CStrandFiller::GetNextNumberOfPermanentStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* nextNum) const
{
   ValidatePermanent();

   if (currNum == INVALID_INDEX)
   {
      *nextNum = 0;
      return S_FALSE;
   }
   else if ( (StrandIndexType)m_PermStrands.size()-1 <= currNum)
   {
      *nextNum = INVALID_INDEX;

      if ( (StrandIndexType)m_PermStrands.size()-1 <= currNum)
      {
         return m_PermStrands[currNum]!=GirderLibraryEntry::ptNone ? S_OK : S_FALSE;
      }
      else
      {
         return S_FALSE;
      }
   }
   else
   {
      HRESULT hr = m_PermStrands[currNum]!=GirderLibraryEntry::ptNone ? S_OK : S_FALSE;

      // should never have to look more than two locations forward
      if (m_PermStrands[++currNum]!=GirderLibraryEntry::ptNone)
      {
         *nextNum = currNum;
      }
      else if (m_PermStrands[++currNum]!=GirderLibraryEntry::ptNone)
      {
         *nextNum = currNum;
      }
      else
      {
         ATLASSERT(false);   // this should never happen if m_PermStrands is build correctly
         *nextNum = INVALID_INDEX;
      }


      return hr;
   }
}

HRESULT CStrandFiller::GetPreviousNumberOfPermanentStrands(IStrandGridModel* pStrandGridModel, StrandIndexType currNum,  StrandIndexType* nextNum) const
{
   ValidatePermanent();

   StrandIndexType s_size = m_PermStrands.size();

   if (currNum<=0)
   {
      *nextNum = INVALID_INDEX;

      if (currNum==0 && m_PermStrands[0]!=GirderLibraryEntry::ptNone)
      {
         return S_OK;
      }
      else
      {
         return S_FALSE;
      }

   }
   else if (s_size <= currNum)
   {
      *nextNum = s_size-1;

      ATLASSERT( m_PermStrands[s_size-1]!=GirderLibraryEntry::ptNone );
      return S_FALSE;
   }
   else
   {
      HRESULT hr = m_PermStrands[currNum]!=GirderLibraryEntry::ptNone ? S_OK : S_FALSE;

      // should never have to look more than two locations back
      if (m_PermStrands[--currNum]!=GirderLibraryEntry::ptNone)
      {
         *nextNum = currNum;
      }
      else
      {
         if (currNum==0)
         {
            *nextNum = INVALID_INDEX;
         }
         else if (m_PermStrands[--currNum]!=GirderLibraryEntry::ptNone)
         {
            *nextNum = currNum;
         }
         else
         {
            ATLASSERT(false);   // this should never happen if m_PermStrands is build correctly
            *nextNum = -1;
         }
      }


      return hr;
   }
}

HRESULT CStrandFiller::ComputeStraightStrandFill(IStrandGridModel* pStrandGridModel, const CDirectStrandFillCollection* pCollection, IIndexArray** strandFill) const
{
   CComPtr<IIndexArray> maxarray;
   HRESULT hr = pStrandGridModel->GetMaxStrandFill(Straight,&maxarray);
   if (FAILED(hr))
   {
      ATLASSERT(false);
      return hr;
   }

   m_TempArray->Clear();

   // Max strand fill array has same indexing as girder library entry
   IndexType cnt;
   maxarray->get_Count(&cnt);
   ATLASSERT(cnt==m_pGdrEntry->GetNumStraightStrandCoordinates());

   // First assign all locations in fill array to zero
   m_TempArray->Assign(cnt, 0);

   // Next, fill strands
   CDirectStrandFillCollection::const_iterator  it = pCollection->begin();
   CDirectStrandFillCollection::const_iterator itend = pCollection->end();
   while(it!=itend)
   {
      if (it->permStrandGridIdx < cnt)
      {
         IndexType maxf;
         maxarray->get_Item(it->permStrandGridIdx, &maxf);
         if (it->numFilled <= maxf)
         {
            // normal fill
            m_TempArray->put_Item(it->permStrandGridIdx, it->numFilled);
         }
         else
         {
            // Somehow fill value is greater than max - something is wrong
            ATLASSERT(false);
            m_TempArray->put_Item(it->permStrandGridIdx, maxf);
         }
      }
      else
      {
         ATLASSERT(false); // should never happen
      }

      it++;
   }

   return m_TempArray.CopyTo(strandFill);
}

HRESULT CStrandFiller::ComputeHarpedStrandFill(IStrandGridModel* pStrandGridModel, const CDirectStrandFillCollection* pCollection, IIndexArray** strandFill) const
{
   CComPtr<IIndexArray> maxarray;
   HRESULT hr = pStrandGridModel->GetMaxStrandFill(Harped,&maxarray);
   if (FAILED(hr))
   {
      ATLASSERT(false);
      return hr;
   }

   m_TempArray->Clear();

   // Max strand fill array has same indexing as girder library entry
   IndexType cnt;
   maxarray->get_Count(&cnt);
   ATLASSERT(cnt==m_pGdrEntry->GetNumHarpedStrandCoordinates());

   // First assign all locations in fill array to zero
   m_TempArray->Assign(cnt, 0);

   // Next, fill strands
   CDirectStrandFillCollection::const_iterator it = pCollection->begin();
   CDirectStrandFillCollection::const_iterator itend = pCollection->end();
   while(it!=itend)
   {
      if (it->permStrandGridIdx < cnt)
      {
         IndexType maxf;
         maxarray->get_Item(it->permStrandGridIdx, &maxf);
         if (it->numFilled <= maxf)
         {
            // normal fill
            m_TempArray->put_Item(it->permStrandGridIdx, it->numFilled);
         }
         else
         {
            // Somehow fill value is greater than max - something is wrong
            ATLASSERT(false);
            m_TempArray->put_Item(it->permStrandGridIdx, maxf);
         }
      }
      else
      {
         ATLASSERT(false); // should never happen
      }

      it++;
   }

   return m_TempArray.CopyTo(strandFill);
}

HRESULT CStrandFiller::ComputeTemporaryStrandFill(IStrandGridModel* pStrandGridModel, const CDirectStrandFillCollection* pCollection, IIndexArray** strandFill) const
{
   CComPtr<IIndexArray> maxarray;
   HRESULT hr = pStrandGridModel->GetMaxStrandFill(Temporary,&maxarray);
   if (FAILED(hr))
   {
      ATLASSERT(false);
      return hr;
   }

   m_TempArray->Clear();

   // Max strand fill array has same indexing as girder library entry
   IndexType cnt;
   maxarray->get_Count(&cnt);
   ATLASSERT(cnt==m_pGdrEntry->GetNumTemporaryStrandCoordinates());

   // First assign all locations in fill array to zero
   m_TempArray->Assign(cnt, 0);

   // Next, fill strands
   CDirectStrandFillCollection::const_iterator it = pCollection->begin();
   CDirectStrandFillCollection::const_iterator itend = pCollection->end();
   while(it!=itend)
   {
      if (it->permStrandGridIdx < cnt)
      {
         IndexType maxf;
         maxarray->get_Item(it->permStrandGridIdx, &maxf);
         if (it->numFilled <= maxf)
         {
            // normal fill
            m_TempArray->put_Item(it->permStrandGridIdx, it->numFilled);
         }
         else
         {
            // Somehow fill value is greater than max - something is wrong
            ATLASSERT(false);
            m_TempArray->put_Item(it->permStrandGridIdx, maxf);
         }
      }
      else
      {
         ATLASSERT(false); // should never happen
      }

      it++;
   }

   return m_TempArray.CopyTo(strandFill);
}

HRESULT CStrandFiller::SetStraightDirectStrandFill(IStrandGridModel* pStrandGridModel,  const CDirectStrandFillCollection* pCollection)
{
   CComPtr<IIndexArray> fillArray;
   this->ComputeStraightStrandFill(pStrandGridModel, pCollection, &fillArray);

   return pStrandGridModel->putref_StrandFill(Straight,fillArray);
}

HRESULT CStrandFiller::SetHarpedDirectStrandFill(IStrandGridModel* pStrandGridModel,  const CDirectStrandFillCollection* pCollection)
{
   CComPtr<IIndexArray> fillArray;
   this->ComputeHarpedStrandFill(pStrandGridModel, pCollection, &fillArray);

   return pStrandGridModel->putref_StrandFill(Harped,fillArray);
}

HRESULT CStrandFiller::SetTemporaryDirectStrandFill(IStrandGridModel* pStrandGridModel,  const CDirectStrandFillCollection* pCollection)
{
   CComPtr<IIndexArray> fillArray;
   this->ComputeTemporaryStrandFill(pStrandGridModel, pCollection, &fillArray);

   return pStrandGridModel->putref_StrandFill(Temporary,fillArray);
}

