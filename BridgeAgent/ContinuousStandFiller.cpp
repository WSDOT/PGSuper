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

// ContinuousStandFiller.cpp: implementation of the CContinuousStandFiller class.
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
static HRESULT GetNextNumStrands(StrandIndexType currNum, ILongArray* array, StrandIndexType* nextNum)
{
   if (currNum == INVALID_INDEX)
   {
      return E_INVALIDARG;
   }

   StrandIndexType running_cnt=0;
   CollectionIndexType size;
   array->get_Count(&size);
   for (CollectionIndexType ip = 0; ip < size && running_cnt <= currNum; ip++)
   {
      IDType val;
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

static HRESULT GetPrevNumStrands(StrandIndexType currNum, ILongArray* array, StrandIndexType* prevNum)
{
   if (currNum <= 0 || currNum == INVALID_INDEX)
   {
      ATLASSERT(0);
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
      CollectionIndexType size;
      array->get_Count(&size);
      for (CollectionIndexType ip = 0; ip < size; ip++)
      {
         StrandIndexType prev_cnt = running_cnt;

         IDType val;
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

CContinuousStandFiller::CContinuousStandFiller():
m_pGdrEntry(NULL),
m_NeedToCompute(true)
{
   m_TempArray.CoCreateInstance(CLSID_LongArray);

   // typical girders will not have this many strands
   m_TempArray->Reserve(128);

   m_StrandFillTool.CoCreateInstance(CLSID_StrandFillTool);
}

CContinuousStandFiller::~CContinuousStandFiller()
{
}

void CContinuousStandFiller::Init(const GirderLibraryEntry* pGdrEntry)
{
   m_pGdrEntry = pGdrEntry;
   m_NeedToCompute = true;
}

void CContinuousStandFiller::ValidatePermanent()
{
   // permanent strand ordering is nasty. compute once and reuse.
   if(m_NeedToCompute)
   {
      m_PermStrands = m_pGdrEntry->GetPermanentStrands();
      m_NeedToCompute = false;
   }
}


// place strands using continuous fill order
HRESULT CContinuousStandFiller::SetStraightStrandCount(IPrecastGirder* girder, StrandIndexType nStrands)
{
   CComPtr<ILongArray> array;
   HRESULT hr = ComputeStraightStrandFill(girder,nStrands, &array);
   if (FAILED(hr))
   {
      ATLASSERT(0);
      return hr;
   }

   hr = girder->put_StraightStrandFill(array);
   if (FAILED(hr))
   {
      ATLASSERT(0);
      return hr;
   }

   return S_OK;
}

HRESULT CContinuousStandFiller::SetHarpedStrandCount(IPrecastGirder* girder,  StrandIndexType nStrands)
{
   CComPtr<ILongArray> array;
   HRESULT hr = ComputeHarpedStrandFill(girder,nStrands, &array);
   if (FAILED(hr))
   {
      ATLASSERT(0);
      return hr;
   }

   hr = girder->put_HarpedStrandFill(array);
   if (FAILED(hr))
   {
      ATLASSERT(0);
      return hr;
   }

   return S_OK;
}

HRESULT CContinuousStandFiller::SetTemporaryStrandCount(IPrecastGirder* girder, StrandIndexType nStrands)
{
   CComPtr<ILongArray> array;
   HRESULT hr = ComputeTemporaryStrandFill(girder,nStrands, &array);
   if (FAILED(hr))
   {
      ATLASSERT(0);
      return hr;
   }

   hr = girder->put_TemporaryStrandFill(array);
   if (FAILED(hr))
   {
      ATLASSERT(0);
      return hr;
   }

   return S_OK;
}

HRESULT CContinuousStandFiller::IsValidNumStraightStrands(IPrecastGirder* girder, StrandIndexType nStrands,VARIANT_BOOL* bIsValid)
{
   CComPtr<ILongArray> array;
   HRESULT hr = ComputeStraightStrandFill(girder,nStrands,&array);
   *bIsValid = SUCCEEDED(hr) ? VARIANT_TRUE : VARIANT_FALSE;
   return S_OK;
}

HRESULT CContinuousStandFiller::IsValidNumHarpedStrands(IPrecastGirder* girder, StrandIndexType nStrands,VARIANT_BOOL* bIsValid)
{
   CComPtr<ILongArray> array;
   HRESULT hr = ComputeHarpedStrandFill(girder,nStrands,&array);
   *bIsValid = SUCCEEDED(hr) ? VARIANT_TRUE : VARIANT_FALSE;
   return S_OK;
}

HRESULT CContinuousStandFiller::IsValidNumTemporaryStrands(IPrecastGirder* girder, StrandIndexType nStrands,VARIANT_BOOL* bIsValid)
{
   CComPtr<ILongArray> array;
   HRESULT hr = ComputeTemporaryStrandFill(girder,nStrands,&array);
   *bIsValid = SUCCEEDED(hr) ? VARIANT_TRUE : VARIANT_FALSE;
   return S_OK;
}

HRESULT CContinuousStandFiller::ComputeStraightStrandFill(IPrecastGirder* girder, StrandIndexType nStrands, ILongArray** strandFill)
{
   CComPtr<IStrandGrid> grid;
   girder->get_StraightStrandGrid(etStart,&grid);

   CComQIPtr<IStrandGridFiller> gridFiller(grid);
   return ComputeStraightStrandFill(gridFiller,nStrands,strandFill);
}

HRESULT CContinuousStandFiller::ComputeStraightStrandFill(IStrandGridFiller* pGridFiller, StrandIndexType nStrands, ILongArray** strandFill)
{
   CComPtr<ILongArray> array;
   HRESULT hr = pGridFiller->GetMaxStrandFill(&array);
   if (FAILED(hr))
   {
      ATLASSERT(0);
      return hr;
   }

   m_TempArray->Clear();

   if (0 < nStrands)
   {
      CollectionIndexType cnt=0;
      CollectionIndexType size;
      array->get_Count(&size);
      for (CollectionIndexType ip = 0; ip < size; ip++)
      {
         IDType val;
         array->get_Item(ip, &val);

         m_TempArray->Add(val);
         cnt += val;
      
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
//               ATLASSERT(0);
               return E_INVALIDARG;
            }
         }
      }

      // not enough strand locations available in girder to fit
//      ATLASSERT(0);
      return E_INVALIDARG;
   }
   else
   {
      // no strands, return empty array
      return m_TempArray.CopyTo(strandFill);
   }
}

HRESULT CContinuousStandFiller::ComputeHarpedStrandFill(IPrecastGirder* girder, StrandIndexType nStrands, ILongArray** strandFill)
{
   CComPtr<IStrandGrid> startGrid, hpGrid;
   girder->get_HarpedStrandGridEnd(etStart,&startGrid);
   girder->get_HarpedStrandGridHP(etStart,&hpGrid);

   VARIANT_BOOL vb;
   girder->get_AllowOddNumberOfHarpedStrands(&vb);

   CComQIPtr<IStrandGridFiller> startGridFiller(startGrid);
   CComQIPtr<IStrandGridFiller> hpGridFiller(hpGrid);
   return ComputeHarpedStrandFill(vb == VARIANT_TRUE ? true : false,startGridFiller,hpGridFiller,nStrands,strandFill);
}

HRESULT CContinuousStandFiller::ComputeHarpedStrandFill(bool bAllowOddNumberOfHarpedStrands,IStrandGridFiller* pEndGridFiller, IStrandGridFiller* pHPGridFiller, StrandIndexType nStrands, ILongArray** strandFill)
{
   StrandIndexType nMaxStrands;
   CComPtr<ILongArray> array;
   m_StrandFillTool->ComputeHarpedStrandMaxFill(pEndGridFiller,pHPGridFiller,&nMaxStrands,&array);

   m_TempArray->Clear();

   if (0 < nStrands)
   {
      CollectionIndexType cnt=0;
      CollectionIndexType size;
      array->get_Count(&size);
      for (CollectionIndexType ip = 0; ip < size; ip++)
      {
         IDType val;
         array->get_Item(ip, &val);

         m_TempArray->Add(val);
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
               ATLASSERT(0);
               return E_INVALIDARG;
            }
         }
      }

      // not enough strand locations available in girder to fit
      //ATLASSERT(0);
      return E_INVALIDARG;
   }
   else
   {
      // no strands - return empty array
      return m_TempArray.CopyTo(strandFill);
   }
}

HRESULT CContinuousStandFiller::ComputeTemporaryStrandFill(IPrecastGirder* girder, StrandIndexType nStrands, ILongArray** strandFill)
{
   CComPtr<IStrandGrid> grid;
   girder->get_TemporaryStrandGrid(etStart,&grid);

   CComQIPtr<IStrandGridFiller> gridFiller(grid);
   return ComputeTemporaryStrandFill(gridFiller,nStrands,strandFill);
}

HRESULT CContinuousStandFiller::ComputeTemporaryStrandFill(IStrandGridFiller* pGridFiller, StrandIndexType nStrands, ILongArray** strandFill)
{
   CComPtr<ILongArray> array;
   HRESULT hr = pGridFiller->GetMaxStrandFill(&array);
   if (FAILED(hr))
   {
      ATLASSERT(0);
      return hr;
   }

   m_TempArray->Clear();

   if (0 < nStrands)
   {

      CollectionIndexType cnt=0;
      CollectionIndexType size;
      array->get_Count(&size);
      for (CollectionIndexType ip = 0; ip < size; ip++)
      {
         IDType val;
         array->get_Item(ip, &val);

         m_TempArray->Add(val);
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
               //ATLASSERT(0);
               return E_INVALIDARG;
            }
         }
      }

      // not enough strand locations available in girder to fit
//      ATLASSERT(0);
      return E_INVALIDARG;
   }
   else
   {
      // no strands, return empty
      return m_TempArray.CopyTo(strandFill);
   }
}

HRESULT CContinuousStandFiller::GetNextNumberOfStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* nextStrands)
{
   CComPtr<ILongArray> array;
   HRESULT hr = pGridFiller->GetMaxStrandFill(&array);
   if (FAILED(hr))
   {
      ATLASSERT(0);
      return hr;
   }

   return GetNextNumStrands(currNum, array, nextStrands);
}

HRESULT CContinuousStandFiller::GetPrevNumberOfStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* prevStrands)
{
   CComPtr<ILongArray> array;
   HRESULT hr = pGridFiller->GetMaxStrandFill(&array);
   if (FAILED(hr))
   {
      ATLASSERT(0);
      return hr;
   }

   return GetPrevNumStrands(currNum, array, prevStrands);
}

HRESULT CContinuousStandFiller::GetNextNumberOfStraightStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* nextStrands)
{
   return GetNextNumberOfStrands(pGridFiller,currNum,nextStrands);
}

HRESULT CContinuousStandFiller::GetPrevNumberOfStraightStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* prevStrands)
{
   return GetPrevNumberOfStrands(pGridFiller,currNum,prevStrands);
}

HRESULT CContinuousStandFiller::GetNextNumberOfHarpedStrands(bool bAllowOddStrand,IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* nextStrands)
{
   CComPtr<ILongArray> array;
   HRESULT hr = pGridFiller->GetMaxStrandFill(&array);
   if (FAILED(hr))
   {
      ATLASSERT(0);
      return hr;
   }

   CollectionIndexType size;
   array->get_Count(&size);
   if (size == 0)
   {
      *nextStrands = INVALID_INDEX;
      return S_OK;
   }

   if (bAllowOddStrand)
   {
      IDType val;
      array->get_Item(0,&val);
      if (val == 2)
      {
         // we can reduce by one, so any number less than max is possible
         StrandIndexType max_harped;
         HRESULT hr = pGridFiller->get_MaxStrandCount(&max_harped);
         if (FAILED(hr))
         {
            ATLASSERT(0);
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

HRESULT CContinuousStandFiller::GetPrevNumberOfHarpedStrands(bool bAllowOddStrand,IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* prevStrands)
{
   // get the no-brainers out of the way
   if (currNum <= 0)
   {
      ATLASSERT(0);
      return E_INVALIDARG;
   }
   else if (currNum == 1)
   {
      *prevStrands = 0;
      return S_OK;
   }

   CComPtr<ILongArray> array;
   HRESULT hr = pGridFiller->GetMaxStrandFill(&array);
   if (FAILED(hr))
   {
      ATLASSERT(0);
      return hr;
   }

   // possible that odd strands will help us out
   if (bAllowOddStrand)
   {
      IDType val;
      array->get_Item(0,&val);
      if (val == 2)
      {
         // we can reduce by one, so any number less than max is possible
         StrandIndexType max_harped;
         HRESULT hr = pGridFiller->get_MaxStrandCount(&max_harped);
         if (FAILED(hr))
         {
            ATLASSERT(0);
            return hr;
         }

         if (currNum < max_harped+1)
         {
            *prevStrands = currNum-1;
            return S_OK;
         }
         else
         {
            ATLASSERT(0);
            return hr;
         }
      }
      // else we have to go through normal strand logic below
   }

   return GetPrevNumStrands(currNum, array, prevStrands);
}

HRESULT CContinuousStandFiller::GetNextNumberOfTemporaryStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* nextStrands)
{
   return GetNextNumberOfStrands(pGridFiller,currNum,nextStrands);
}

HRESULT CContinuousStandFiller::GetPrevNumberOfTemporaryStrands(IStrandGridFiller* pGridFiller, StrandIndexType currNum,  StrandIndexType* prevStrands)
{
   return GetPrevNumberOfStrands(pGridFiller,currNum,prevStrands);
}

HRESULT CContinuousStandFiller::GetNextNumberOfStraightStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* nextStrands)
{
   CComPtr<ILongArray> array;
   HRESULT hr = girder->get_StraightMaxStrandFill(&array);
   if (FAILED(hr))
   {
      ATLASSERT(0);
      return hr;
   }

   return GetNextNumStrands(currNum, array, nextStrands);
}

HRESULT CContinuousStandFiller::GetNextNumberOfHarpedStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* nextStrands)
{
   CComPtr<ILongArray> array;
   HRESULT hr = girder->get_HarpedMaxStrandFill(&array);
   if (FAILED(hr))
   {
      ATLASSERT(0);
      return hr;
   }

   CollectionIndexType size;
   array->get_Count(&size);
   if (size == 0)
   {
      *nextStrands = INVALID_INDEX;
      return S_OK;
   }

   VARIANT_BOOL allow;
   girder->get_AllowOddNumberOfHarpedStrands(&allow);
   if (allow == VARIANT_TRUE)
   {
      IDType val;
      array->get_Item(0,&val);
      if (val == 2)
      {
         // we can reduce by one, so any number less than max is possible
         StrandIndexType max_harped;
         HRESULT hr = girder->get_MaxHarpedStrands(&max_harped);
         if (FAILED(hr))
         {
            ATLASSERT(0);
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

HRESULT CContinuousStandFiller::GetNextNumberOfTemporaryStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* nextStrands)
{
   CComPtr<ILongArray> array;
   HRESULT hr = girder->get_TemporaryMaxStrandFill(&array);
   if (FAILED(hr))
   {
      ATLASSERT(0);
      return hr;
   }

   return GetNextNumStrands(currNum, array, nextStrands);
}

HRESULT CContinuousStandFiller::GetPreviousNumberOfStraightStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* prevStrands)
{
   CComPtr<ILongArray> array;
   HRESULT hr = girder->get_StraightMaxStrandFill(&array);
   if (FAILED(hr))
   {
      ATLASSERT(0);
      return hr;
   }

   return GetPrevNumStrands(currNum, array, prevStrands);
}

HRESULT CContinuousStandFiller::GetPreviousNumberOfHarpedStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* prevStrands)
{
   // get the no-brainers out of the way
   if (currNum <= 0)
   {
      ATLASSERT(0);
      return E_INVALIDARG;
   }
   else if (currNum == 1)
   {
      *prevStrands = 0;
      return S_OK;
   }

   CComPtr<ILongArray> array;
   HRESULT hr = girder->get_HarpedMaxStrandFill(&array);
   if (FAILED(hr))
   {
      ATLASSERT(0);
      return hr;
   }

   // possible that odd strands will help us out
   VARIANT_BOOL allow;
   girder->get_AllowOddNumberOfHarpedStrands(&allow);
   if (allow == VARIANT_TRUE)
   {
      IDType val;
      array->get_Item(0,&val);
      if (val == 2)
      {
         // we can reduce by one, so any number less than max is possible
         StrandIndexType max_harped;
         HRESULT hr = girder->get_MaxHarpedStrands(&max_harped);
         if (FAILED(hr))
         {
            ATLASSERT(0);
            return hr;
         }

         if (currNum < max_harped+1)
         {
            *prevStrands = currNum-1;
            return S_OK;
         }
         else
         {
            ATLASSERT(0);
            return hr;
         }
      }
      // else we have to go through normal strand logic below
   }

   return GetPrevNumStrands(currNum, array, prevStrands);
}

HRESULT CContinuousStandFiller::GetPreviousNumberOfTemporaryStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* prevStrands)
{
   CComPtr<ILongArray> array;
   HRESULT hr = girder->get_TemporaryMaxStrandFill(&array);
   if (FAILED(hr))
   {
      ATLASSERT(0);
      return hr;
   }

   return GetPrevNumStrands(currNum, array, prevStrands);
}

HRESULT CContinuousStandFiller::GetMaxNumPermanentStrands(IPrecastGirder* girder, StrandIndexType* numStrands)
{
   ValidatePermanent();

   *numStrands = m_PermStrands.size()-1;

   return S_OK;
}

HRESULT CContinuousStandFiller::ComputeNumPermanentStrands(IPrecastGirder* girder, StrandIndexType totalPermanent, StrandIndexType* numStraight, StrandIndexType* numHarped)
{
   return m_pGdrEntry->ComputeGlobalStrands(totalPermanent, numStraight, numHarped) ? S_OK : S_FALSE;
}

HRESULT CContinuousStandFiller::GetNextNumberOfPermanentStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* nextNum)
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
         ATLASSERT(0);   // this should never happen if m_PermStrands is build correctly
         *nextNum = INVALID_INDEX;
      }


      return hr;
   }
}

HRESULT CContinuousStandFiller::GetPreviousNumberOfPermanentStrands(IPrecastGirder* girder, StrandIndexType currNum,  StrandIndexType* nextNum)
{
   ValidatePermanent();

   StrandIndexType s_size = m_PermStrands.size();

   if (currNum<=0)
   {
      *nextNum = INVALID_INDEX;

      if (currNum==0 && m_PermStrands[0]!=GirderLibraryEntry::ptNone)
         return S_OK;
      else
         return S_FALSE;

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
            ATLASSERT(0);   // this should never happen if m_PermStrands is build correctly
            *nextNum = -1;
         }
      }


      return hr;
   }
}
