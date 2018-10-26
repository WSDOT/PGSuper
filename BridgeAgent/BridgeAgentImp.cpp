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

// BridgeAgentImp.cpp : Implementation of CBridgeAgentImp
#include "stdafx.h"
#include "BridgeAgent.h"
#include "BridgeAgent_i.h"
#include "BridgeAgentImp.h"
#include "BridgeHelpers.h"
#include "..\PGSuperException.h"
#include "DeckEdgeBuilder.h"

#include "StatusItems.h"
#include "PGSuperUnits.h"

#include <PsgLib\LibraryManager.h>
#include <PsgLib\GirderLibraryEntry.h>
#include <PsgLib\ConnectionLibraryEntry.h>
#include <PsgLib\UnitServer.h>

#include <PgsExt\BridgeDescription.h>
#include <PgsExt\DeckDescription.h>
#include <PgsExt\PierData.h>
#include <PgsExt\SpanData.h>

#include <PgsExt\GirderData.h>
#include <PgsExt\PointOfInterest.h>
#include <PsgLib\ShearData.h>
#include <PgsExt\LongitudinalRebarData.h>
#include <PgsExt\PointLoadData.h>
#include <PgsExt\DistributedLoadData.h>
#include <PgsExt\MomentLoadData.h>
#include <PgsExt\StatusItem.h>
#include <PgsExt\GirderLabel.h>
#include <PgsExt\StageCompare.h>

#include <MathEx.h>
#include <System\Flags.h>
#include <Material\ConcreteEx.h>
#include <Material\PsStrand.h>
#include <Material\Rebar.h>
#include <GeomModel\ShapeUtils.h>

#include <IFace\DrawBridgeSettings.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\ShearCapacity.h>
#include <IFace\GirderHandling.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\StatusCenter.h>

#include <BridgeModeling\DrawSettings.h>

#include <BridgeModeling\LrLayout.h>
#include <BridgeModeling\LrFlexiZone.h>
#include <BridgeModeling\LrRowPattern.h>

#include <DesignConfigUtil.h>

#include <algorithm>
#include <cctype>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


DECLARE_LOGFILE;


#define CLEAR_ALL       0
#define COGO_MODEL      1
#define CONCRETE        2
#define BRIDGE          3
#define GIRDER          4

// Capacity problem validation states
#define CAPPROB_NC        0x0001
#define CAPPROB_CMP_INT   0x0002
#define CAPPROB_CMP_EXT   0x0004

#define USERLOADPOI_ATTR POI_FLEXURESTRESS | POI_SHEAR | POI_FLEXURECAPACITY | POI_TABULAR | POI_GRAPHICAL | POI_CONCLOAD


#define VALIDATE(x) {if ( !Validate((x)) ) THROW_SHUTDOWN(_T("Fatal Error in Bridge Agent"),XREASON_AGENTVALIDATIONFAILURE,true);}
#define INVALIDATE(x) Invalidate((x))

#define STARTCONNECTION _T("Left Connection")  // must be this value for the PrecastGirder model
#define ENDCONNECTION   _T("Right Connection") // must be this value for the PrecastGirder model

//function to translate project loads to bridge loads

IUserDefinedLoads::UserDefinedLoadCase Project2BridgeLoads(UserLoads::LoadCase plc)
{
   if (plc==UserLoads::DC)
      return IUserDefinedLoads::userDC;
   else if (plc==UserLoads::DW)
      return IUserDefinedLoads::userDW;
   else if (plc==UserLoads::LL_IM)
      return IUserDefinedLoads::userLL_IM;
   else
   {
      ATLASSERT(0);
      return  IUserDefinedLoads::userDC;
   }
}

// function for computing debond factor from development length
inline Float64 GetDevLengthAdjustment(Float64 bonded_length, Float64 dev_length, Float64 xfer_length, Float64 fps, Float64 fpe)
{
   if (bonded_length <= 0.0)
   {
      // strand is unbonded at location, no more to do
      return 0.0;
   }
   else
   {
      Float64 adjust = -999; // dummy value, helps with debugging

      if ( bonded_length <= xfer_length)
      {
         adjust = (fpe < fps ? (bonded_length*fpe) / (xfer_length*fps) : 1.0);
      }
      else if ( bonded_length < dev_length )
      {
         adjust = (fpe + (bonded_length - xfer_length)*(fps-fpe)/(dev_length - xfer_length))/fps;
      }
      else
      {
         adjust = 1.0;
      }

      adjust = IsZero(adjust) ? 0 : adjust;
      adjust = ::ForceIntoRange(0.0,adjust,1.0);
      return adjust;
   }
}



// an arbitrary large length
static const Float64 BIG_LENGTH = 1.0e12;
// floating tolerance
static const Float64 TOL=1.0e-04;


inline Float64 GetGirderEci(IGirderData* pGirderData,SpanIndexType span,GirderIndexType gdr, Float64 trialFci, bool* pChanged)
{
   const CGirderMaterial* pGdrMaterial = pGirderData->GetGirderMaterial(span,gdr);

   Float64 modE;
   // release strength
   if ( pGdrMaterial->bUserEci )
   {
      modE = pGdrMaterial->Eci;

      *pChanged = false;
   }
   else
   {
      modE = lrfdConcreteUtil::ModE( trialFci, 
                                     pGdrMaterial->StrengthDensity, 
                                     false /* ignore LRFD range checks */ );

      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         modE *= (pGdrMaterial->EcK1*pGdrMaterial->EcK2);
      }

      *pChanged = !IsEqual(modE,pGdrMaterial->Eci);
   }

   return modE;
}

inline Float64 GetGirderEc(IGirderData* pGirderData,SpanIndexType span,GirderIndexType gdr, Float64 trialFc, bool* pChanged)
{
   const CGirderMaterial* pGdrMaterial = pGirderData->GetGirderMaterial(span,gdr);

   Float64 modE;

   if ( pGdrMaterial->bUserEc )
   {
      modE = pGdrMaterial->Ec;
      *pChanged = false;
   }
   else
   {
      modE = lrfdConcreteUtil::ModE( trialFc, 
                                     pGdrMaterial->StrengthDensity, 
                                     false /* ignore LRFD range checks */ );

      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         modE *= (pGdrMaterial->EcK1*pGdrMaterial->EcK2);
      }

      *pChanged = !IsEqual(modE,pGdrMaterial->Ec);
   }

   return modE;
}

// Function to choose confinement bars from primary and additional
static void ChooseConfinementBars(Float64 requiredZoneLength, 
                                  Float64 primSpc, Float64 primZonL, matRebar::Size primSize,
                                  Float64 addlSpc, Float64 addlZonL, matRebar::Size addlSize,
                                  matRebar::Size* pSize, Float64* pProvidedZoneLength, Float64* pSpacing)
{
   // Start assuming the worst
   *pSize = matRebar::bsNone;
   *pProvidedZoneLength = 0.0;
   *pSpacing = 0.0;

   // methods must have bars and non-zero spacing just to be in the running
   bool is_prim = primSize!=matRebar::bsNone && primSpc>0.0;
   bool is_addl = addlSize!=matRebar::bsNone && addlSpc>0.0;

   // Both must meet required zone length to qualify for a run-off
   if (is_prim && is_addl && primZonL+TOL > requiredZoneLength && addlZonL+TOL > requiredZoneLength)
   {
      // both meet zone length req. choose smallest spacing
      if (primSpc < addlSpc)
      {
         *pSize = primSize;
         *pProvidedZoneLength = primZonL;
         *pSpacing = primSpc;
      }
      else
      {
         *pSize = addlSize;
         *pProvidedZoneLength = addlZonL;
         *pSpacing = addlSpc;
      }
   }
   else if (is_prim && primZonL+TOL > requiredZoneLength)
   {
      *pSize = primSize;
      *pProvidedZoneLength = primZonL;
      *pSpacing = primSpc;
   }
   else if (is_addl && addlZonL+TOL > requiredZoneLength)
   {
      *pSize = addlSize;
      *pProvidedZoneLength = addlZonL;
      *pSpacing = addlSpc;
   }
   else if (is_addl)
   {
      *pSize = addlSize;
      *pProvidedZoneLength = addlZonL;
      *pSpacing = addlSpc;
   }
   else if (is_prim)
   {
      *pSize = primSize;
      *pProvidedZoneLength = primZonL;
      *pSpacing = primSpc;
   }
}

/////////////////////////////////////////////////////////
// Utilities for Dealing with strand array conversions
inline void IndexArray2ConfigStrandFillVec(IIndexArray* pArray, ConfigStrandFillVector& rVec)
{
   rVec.clear();
   if (pArray!=NULL)
   {
      CollectionIndexType cnt;
      pArray->get_Count(&cnt);
      rVec.reserve(cnt);

      CComPtr<IEnumIndexArray> enum_array;
      pArray->get__EnumElements(&enum_array);
      StrandIndexType value;
      while ( enum_array->Next(1,&value,NULL) != S_FALSE )
      {
         rVec.push_back(value);
      }
   }
}

bool AreStrandsInConfigFillVec(const ConfigStrandFillVector& rHarpedFillArray)
{
   ConfigStrandFillConstIterator it =    rHarpedFillArray.begin();
   ConfigStrandFillConstIterator itend = rHarpedFillArray.end();
   while(it!=itend)
   {
      if(*it>0)
         return true;

      it++;
   }

   return false;
}

StrandIndexType CountStrandsInConfigFillVec(const ConfigStrandFillVector& rHarpedFillArray)
{
   StrandIndexType cnt(0);

   ConfigStrandFillConstIterator it =    rHarpedFillArray.begin();
   ConfigStrandFillConstIterator itend = rHarpedFillArray.end();
   while(it!=itend)
   {
      cnt += *it;
      it++;
   }

   return cnt;
}


// Wrapper class to turn a ConfigStrandFillVector into a IIndexArray
// THIS IS A VERY MINIMAL WRAPPER USED TO PASS STRAND DATA INTO THE WBFL
//   -Read only
//   -Does not reference count
//   -Does not clone
//   -Does not support many functions of IIndexArray
class CIndexArrayWrapper : public IIndexArray
{
   // we are wrapping this container
   const ConfigStrandFillVector& m_Values;

   CIndexArrayWrapper(); // no default constuct
public:
   CIndexArrayWrapper(const ConfigStrandFillVector& vec):
      m_Values(vec)
   {;}

   // IUnknown
   virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, __RPC__deref_out void __RPC_FAR *__RPC_FAR *ppvObject)
   { ATLASSERT(0); return E_FAIL;}
   virtual ULONG STDMETHODCALLTYPE AddRef( void)
   { return 1;}
   virtual ULONG STDMETHODCALLTYPE Release( void)
   {return 1;}

   // IIndexArray
	STDMETHOD(Find)(/*[in]*/CollectionIndexType value, /*[out,retval]*/CollectionIndexType* fndIndex)
   {
      ATLASSERT(0);
      return E_FAIL;
   }
	STDMETHOD(ReDim)(/*[in]*/CollectionIndexType size)
   {
      ATLASSERT(0);
      return E_FAIL;
   }

	STDMETHOD(Clone)(/*[out,retval]*/IIndexArray* *clone)
   {
      ATLASSERT(0);
      return E_FAIL;
   }
	STDMETHOD(get_Count)(/*[out, retval]*/ CollectionIndexType *pVal)
   {
	   *pVal = m_Values.size();
	   return S_OK;
   }
	STDMETHOD(Clear)()
   {
      ATLASSERT(0);
      return E_FAIL;
   }
	STDMETHOD(Reserve)(/*[in]*/CollectionIndexType count)
   {
      ATLASSERT(0);
      return E_FAIL;
   }
	STDMETHOD(Insert)(/*[in]*/CollectionIndexType relPosition, /*[in]*/CollectionIndexType item)
   {
      ATLASSERT(0);
      return E_FAIL;
   }
	STDMETHOD(Remove)(/*[in]*/CollectionIndexType relPosition)
   {
      ATLASSERT(0);
      return E_FAIL;
   }
	STDMETHOD(Add)(/*[in]*/CollectionIndexType item)
   {
      ATLASSERT(0);
      return E_FAIL;
   }
	STDMETHOD(get_Item)(/*[in]*/CollectionIndexType relPosition, /*[out, retval]*/ CollectionIndexType *pVal)
   {
      try
      {
         *pVal = m_Values[relPosition];
      }
      catch(...)
      {
         ATLASSERT(0);
         return E_INVALIDARG;
      }
	   return S_OK;
   }
	STDMETHOD(put_Item)(/*[in]*/CollectionIndexType relPosition, /*[in]*/ CollectionIndexType newVal)
   {
      ATLASSERT(0);
      return E_FAIL;
   }
	STDMETHOD(get__NewEnum)(struct IUnknown ** )
   {
      ATLASSERT(0);
      return E_FAIL;
   }
	STDMETHOD(get__EnumElements)(struct IEnumIndexArray ** )
   {
      ATLASSERT(0);
      return E_FAIL;
   }
   STDMETHOD(Assign)(/*[in]*/CollectionIndexType numElements, /*[in]*/CollectionIndexType value)
   {
      ATLASSERT(0);
      return E_FAIL;
   }
};

///////////////////////////////////////////////////////////
// Exception-safe class for blocking infinite recursion
class SimpleMutex
{
public:

   SimpleMutex(Uint16& flag, Uint16 value):
   m_MutexValue(flag)
   {
      flag = value;
   }

   ~SimpleMutex()
   {
      m_MutexValue = m_Default;
   }

   static const Uint16 m_Default; // mutex is not blocking if set to Default

private:
   Uint16& m_MutexValue;
};

const Uint16 SimpleMutex::m_Default=666; // evil number to search for


static Uint16 st_MutexValue = SimpleMutex::m_Default; // store m_level

/////////////////////////////////////////////////////////////////////////////
// CBridgeAgentImp

HRESULT CBridgeAgentImp::FinalConstruct()
{
   HRESULT hr = m_BridgeGeometryTool.CoCreateInstance(CLSID_BridgeGeometryTool);
   if ( FAILED(hr) )
      return hr;

   hr = m_CogoEngine.CoCreateInstance(CLSID_CogoEngine);
   if ( FAILED(hr) )
      return hr;


   return S_OK;
}

void CBridgeAgentImp::FinalRelease()
{
}

void CBridgeAgentImp::Invalidate( Uint16 level )
{
//   LOG(_T("Invalidating"));

   if ( level <= BRIDGE )
   {
//      LOG(_T("Invalidating Bridge Model"));

      m_Bridge.Release();
      m_HorzCurveKeys.clear();
      m_VertCurveKeys.clear();


      // Must be valided at least past COGO_MODEL
      if ( COGO_MODEL < level )
         level = COGO_MODEL;

      InvalidateConcrete();

      // If bridge is invalid, so are points of interest
      m_PoiValidated.clear();
      m_PoiMgr.RemoveAll();

      m_CriticalSectionState[0].clear();
      m_CriticalSectionState[1].clear();
//      m_GirderConnections.clear();
      m_SectProps.clear();
      InvalidateUserLoads();


      // clear the cached shapes
      m_DeckShapes.clear();
      m_LeftBarrierShapes.clear();
      m_RightBarrierShapes.clear();

      m_LeftSlabEdgeOffset.clear();
      m_RightSlabEdgeOffset.clear();

      // cached sheardata
      InvalidateStirrupData();

      // remove our items from the status center
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      pStatusCenter->RemoveByStatusGroupID(m_StatusGroupID);
   }

   m_Level = level;
   if ( m_Level != 0 )
      m_Level -= 1;

//   LOG(_T("Invalidate complete - BridgeAgent at level ") << m_Level );
}

Uint16 CBridgeAgentImp::Validate( Uint16 level )
{
//   LOG(_T("Validating"));

   if (st_MutexValue != SimpleMutex::m_Default)
   {
      // mutex is blocking recursion
//      LOG(_T("Mutex is blocking recursion at level")<<st_MutexValue);
      ATLASSERT(level<=st_MutexValue); // this is bad. A call down the stack is requesting a higher level Validation
   }
   else
   {
      // use SimpleMutex class to block recursion
      SimpleMutex mutex(st_MutexValue, level);

      if ( level > m_Level )
      {
         VALIDATE_TO_LEVEL( COGO_MODEL,   BuildCogoModel );
         VALIDATE_TO_LEVEL( CONCRETE,     ValidateConcrete );
         VALIDATE_TO_LEVEL( BRIDGE,       BuildBridgeModel );
         VALIDATE_AND_CHECK_TO_LEVEL( GIRDER,       BuildGirder,    ValidateGirder );
      }

  
      if (level>=BRIDGE)
         ValidateUserLoads();


//      LOG(_T("Validation complete - BridgeAgent at level ") << m_Level );
   }

      return m_Level;
}

void CBridgeAgentImp::ValidatePointsOfInterest(SpanIndexType span,GirderIndexType gdr)
{
   // Bridge model, up to and including girders, must be valid before we can layout the poi
   VALIDATE(GIRDER);

   SpanIndexType startSpan = span;
   SpanIndexType nSpans = startSpan + 1;
   if ( span == ALL_SPANS )
   {
      startSpan = 0;
      nSpans = GetSpanCount_Private();
   }

   for ( SpanIndexType spanIdx = startSpan; spanIdx < nSpans; spanIdx++ )
   {
      // deal with girder index when there are different number of girders in each span
      GirderIndexType nGirders = GetGirderCount(spanIdx);
      if ( nGirders <= gdr )
         gdr = nGirders-1;

      SpanGirderHashType key = HashSpanGirder(spanIdx,gdr);
      std::set<SpanGirderHashType>::iterator found = m_PoiValidated.find( key );
      if ( found == m_PoiValidated.end() )
      {
         LayoutPointsOfInterest( spanIdx, gdr );
         m_PoiValidated.insert(key);
      }
   }
}

void CBridgeAgentImp::InvalidateConcrete()
{
   m_pSlabConc = std::auto_ptr<matConcreteEx>(0);
   m_pRailingConc[pgsTypes::tboLeft] = std::auto_ptr<matConcreteEx>(0);
   m_pRailingConc[pgsTypes::tboRight] = std::auto_ptr<matConcreteEx>(0);
   m_pGdrConc.clear();
   m_pGdrReleaseConc.clear();
}

bool CBridgeAgentImp::ValidateConcrete()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();

   // Create slab concrete
   Float64 modE;
   if ( pDeck->SlabUserEc )
   {
      modE = pDeck->SlabEc;
   }
   else
   {
      modE = lrfdConcreteUtil::ModE(pDeck->SlabFc, 
                                    pDeck->SlabStrengthDensity, 
                                    false /* ignore LRFD range checks */ );

      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         modE *= (pDeck->SlabEcK1*pDeck->SlabEcK2);
      }
   }
   m_pSlabConc = std::auto_ptr<matConcreteEx>(new matConcreteEx( _T("Slab Concrete"),
                                                                 pDeck->SlabFc,
                                                                 pDeck->SlabStrengthDensity,
                                                                 pDeck->SlabWeightDensity,
                                                                 modE) );
   m_pSlabConc->SetMaxAggregateSize(pDeck->SlabMaxAggregateSize);
   m_pSlabConc->SetType((matConcrete::Type)pDeck->SlabConcreteType);
   m_pSlabConc->HasAggSplittingStrength(pDeck->SlabHasFct);
   m_pSlabConc->SetAggSplittingStrength(pDeck->SlabFct);

   m_SlabEcK1        = pDeck->SlabEcK1;
   m_SlabEcK2        = pDeck->SlabEcK2;
   m_SlabCreepK1     = pDeck->SlabCreepK1;
   m_SlabCreepK2     = pDeck->SlabCreepK2;
   m_SlabShrinkageK1 = pDeck->SlabShrinkageK1;
   m_SlabShrinkageK2 = pDeck->SlabShrinkageK2;


   // Create railing concrete
   const CRailingSystem* pLeftRailingSystem = pBridgeDesc->GetLeftRailingSystem();
   if ( pLeftRailingSystem->bUserEc )
   {
      modE = pLeftRailingSystem->Ec;
   }
   else
   {
      modE = lrfdConcreteUtil::ModE(pLeftRailingSystem->fc, 
                                    pLeftRailingSystem->StrengthDensity, 
                                    false /* ignore LRFD range checks */ );

      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         modE *= pLeftRailingSystem->EcK1*pLeftRailingSystem->EcK2;
      }
   }

   m_pRailingConc[pgsTypes::tboLeft] = std::auto_ptr<matConcreteEx>(new matConcreteEx( _T("Left Railing Concrete"),
                                                                 pLeftRailingSystem->fc,
                                                                 pLeftRailingSystem->StrengthDensity,
                                                                 pLeftRailingSystem->WeightDensity,
                                                                 modE) );
   m_pRailingConc[pgsTypes::tboLeft]->SetType((matConcrete::Type)pLeftRailingSystem->ConcreteType);
   m_pRailingConc[pgsTypes::tboLeft]->HasAggSplittingStrength(pLeftRailingSystem->bHasFct);
   m_pRailingConc[pgsTypes::tboLeft]->SetAggSplittingStrength(pLeftRailingSystem->Fct);

   const CRailingSystem* pRightRailingSystem = pBridgeDesc->GetRightRailingSystem();
   if ( pRightRailingSystem->bUserEc )
   {
      modE = pRightRailingSystem->Ec;
   }
   else
   {
      modE = lrfdConcreteUtil::ModE(pRightRailingSystem->fc, 
                                    pRightRailingSystem->StrengthDensity, 
                                    false /* ignore LRFD range checks */ );

      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         modE *= pRightRailingSystem->EcK1*pRightRailingSystem->EcK2;
      }
   }

   m_pRailingConc[pgsTypes::tboRight] = std::auto_ptr<matConcreteEx>(new matConcreteEx( _T("Right Railing Concrete"),
                                                                 pRightRailingSystem->fc,
                                                                 pRightRailingSystem->StrengthDensity,
                                                                 pRightRailingSystem->WeightDensity,
                                                                 modE) );
   
   m_pRailingConc[pgsTypes::tboRight]->SetType((matConcrete::Type)pLeftRailingSystem->ConcreteType);
   m_pRailingConc[pgsTypes::tboRight]->HasAggSplittingStrength(pLeftRailingSystem->bHasFct);
   m_pRailingConc[pgsTypes::tboRight]->SetAggSplittingStrength(pLeftRailingSystem->Fct);

   // Girder Concrete
   GET_IFACE(IGirderData,pGirderData);
   const CSpanData* pSpan = pBridgeDesc->GetSpan(0);
   while ( pSpan != NULL )
   {
      SpanIndexType spanIdx = pSpan->GetSpanIndex();

      GirderIndexType nGirders = pSpan->GetGirderCount();

      for ( GirderIndexType girderIdx = 0; girderIdx < nGirders; girderIdx++ )
      {
         // Girder
         const CGirderMaterial* pGdrMaterial = pGirderData->GetGirderMaterial(spanIdx,girderIdx);

         SpanGirderHashType key = HashSpanGirder(spanIdx,girderIdx);

         // release strength
         if ( pGdrMaterial->bUserEci )
         {
            modE = pGdrMaterial->Eci;
         }
         else
         {
            modE = lrfdConcreteUtil::ModE( pGdrMaterial->Fci, 
                                           pGdrMaterial->StrengthDensity, 
                                           false /* ignore LRFD range checks */ );

            if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
            {
               modE *= pGdrMaterial->EcK1*pGdrMaterial->EcK2;
            }
         }
         matConcreteEx* pGdrReleaseConc = new matConcreteEx( _T("Girder Concrete at Release"),
                                                     pGdrMaterial->Fci,
                                                     pGdrMaterial->StrengthDensity,
                                                     pGdrMaterial->WeightDensity,
                                                     modE);

         pGdrReleaseConc->SetMaxAggregateSize(pGdrMaterial->MaxAggregateSize);
         pGdrReleaseConc->SetType((matConcrete::Type)pGdrMaterial->Type);
         pGdrReleaseConc->HasAggSplittingStrength(pGdrMaterial->bHasFct);
         pGdrReleaseConc->SetAggSplittingStrength(pGdrMaterial->Fct);

         m_pGdrReleaseConc.insert( std::make_pair(key,boost::shared_ptr<matConcreteEx>(pGdrReleaseConc)) );

         // 28 day strength
         if ( pGdrMaterial->bUserEc )
         {
            modE = pGdrMaterial->Ec;
         }
         else
         {
         modE = lrfdConcreteUtil::ModE( pGdrMaterial->Fc, 
                                        pGdrMaterial->StrengthDensity, 
                                        false /* ignore LRFD range checks */ );

            if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
            {
               modE *= pGdrMaterial->EcK1*pGdrMaterial->EcK2;
            }
         }
         matConcreteEx* pGdrConc = new matConcreteEx( _T("Girder Concrete"),
                                                     pGdrMaterial->Fc,
                                                     pGdrMaterial->StrengthDensity,
                                                     pGdrMaterial->WeightDensity,
                                                     modE);

         pGdrConc->SetMaxAggregateSize(pGdrMaterial->MaxAggregateSize);
         pGdrConc->SetType((matConcrete::Type)pGdrMaterial->Type);
         pGdrConc->HasAggSplittingStrength(pGdrMaterial->bHasFct);
         pGdrConc->SetAggSplittingStrength(pGdrMaterial->Fct);

         m_pGdrConc.insert( std::make_pair(key,boost::shared_ptr<matConcreteEx>(pGdrConc)) );
      }

      pSpan = pSpan->GetNextPier()->GetNextSpan();
   }

   // Validate values   
   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   GET_IFACE(ILimits2,pLimits);

   // per 5.4.2.1 f'c must exceed 28 MPa (4 ksi)
   bool bSI = lrfdVersionMgr::GetUnits() == lrfdVersionMgr::SI ? true : false;
   Float64 fcMin = bSI ? ::ConvertToSysUnits(28, unitMeasure::MPa) : ::ConvertToSysUnits(4, unitMeasure::KSI);

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   // check railing system
   if ( !IsConcreteDensityInRange(m_pRailingConc[pgsTypes::tboLeft]->GetDensity(),(pgsTypes::ConcreteType)m_pRailingConc[pgsTypes::tboLeft]->GetType()) )
   {
      std::_tostringstream os;
      if ( m_pRailingConc[pgsTypes::tboLeft]->GetType() == pgsTypes::Normal )
         os << _T("Left railing system concrete density is out of range for Normal Weight Concrete per LRFD 5.2.");
      else
         os << _T("Left railing system concrete density is out of range for Lightweight Concrete per LRFD 5.2.");

      std::_tstring strMsg = os.str();

      pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::RailingSystem,pgsConcreteStrengthStatusItem::Density,0,0,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
      pStatusCenter->Add(pStatusItem);
   }

   if ( !IsConcreteDensityInRange(m_pRailingConc[pgsTypes::tboRight]->GetDensity(),(pgsTypes::ConcreteType)m_pRailingConc[pgsTypes::tboRight]->GetType()) )
   {
      std::_tostringstream os;
      if ( m_pRailingConc[pgsTypes::tboRight]->GetType() == pgsTypes::Normal )
         os << _T("Right railing system concrete density is out of range for Normal Weight Concrete per LRFD 5.2.");
      else
         os << _T("Right railing system concrete density is out of range for Lightweight Concrete per LRFD 5.2.");

      std::_tstring strMsg = os.str();

      pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::RailingSystem,pgsConcreteStrengthStatusItem::Density,0,0,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
      pStatusCenter->Add(pStatusItem);
   }

   // Check slab concrete
   if ( m_pSlabConc->GetFc() < fcMin && !IsEqual(m_pSlabConc->GetFc(),fcMin) )
   {
      std::_tstring strMsg;
      strMsg = bSI ? _T("Slab concrete cannot be less than 28 MPa per LRFD 5.4.2.1") 
                   : _T("Slab concrete cannot be less than 4 KSI per LRFD 5.4.2.1");
      pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Slab,pgsConcreteStrengthStatusItem::FinalStrength,0,0,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
      pStatusCenter->Add(pStatusItem);
      //strMsg += std::_tstring(_T("\nSee Status Center for Details"));
      //THROW_UNWIND(strMsg.c_str(),-1);
   }

   pgsTypes::ConcreteType slabConcreteType = (pgsTypes::ConcreteType)m_pSlabConc->GetType();
   Float64 max_slab_fc = pLimits->GetMaxSlabFc(slabConcreteType);
   if (  max_slab_fc < m_pSlabConc->GetFc() && !IsEqual(max_slab_fc,m_pSlabConc->GetFc()) )
   {
      Float64 fcMax = ::ConvertFromSysUnits(max_slab_fc,pDisplayUnits->GetStressUnit().UnitOfMeasure);
      std::_tostringstream os;
      os << _T("Slab concrete strength exceeds the normal value of ") << fcMax << _T(" ") << pDisplayUnits->GetStressUnit().UnitOfMeasure.UnitTag();

         std::_tstring strMsg = os.str();

      pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Slab,pgsConcreteStrengthStatusItem::FinalStrength,0,0,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
      pStatusCenter->Add(pStatusItem);
   }

   Float64 max_wc = pLimits->GetMaxConcreteUnitWeight(slabConcreteType);
   Float64 MaxWc = ::ConvertFromSysUnits(max_wc,pDisplayUnits->GetDensityUnit().UnitOfMeasure);

   if ( max_wc < m_pSlabConc->GetDensity() && !IsEqual(max_wc,m_pSlabConc->GetDensity(),0.0001) )
   {
      std::_tostringstream os;
      os << _T("Slab concrete density for strength calculations exceeds the normal value of ") << MaxWc << _T(" ") << pDisplayUnits->GetDensityUnit().UnitOfMeasure.UnitTag();

      std::_tstring strMsg = os.str();

      pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Slab,pgsConcreteStrengthStatusItem::Density,0,0,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
      pStatusCenter->Add(pStatusItem);
   }

   if ( max_wc < m_pSlabConc->GetDensityForWeight() && !IsEqual(max_wc,m_pSlabConc->GetDensityForWeight(),0.0001) )
   {
      std::_tostringstream os;
      os << _T("Slab concrete density for weight calculations exceeds the normal value of ") << MaxWc << _T(" ") << pDisplayUnits->GetDensityUnit().UnitOfMeasure.UnitTag();

      std::_tstring strMsg = os.str();

      pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Slab,pgsConcreteStrengthStatusItem::DensityForWeight,0,0,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
      pStatusCenter->Add(pStatusItem);
   }

   if ( !IsConcreteDensityInRange(m_pSlabConc->GetDensity(), (pgsTypes::ConcreteType)m_pSlabConc->GetType()) )
   {
      std::_tostringstream os;
      if ( m_pSlabConc->GetType() == pgsTypes::Normal )
         os << _T("Slab concrete density is out of range for Normal Weight Concrete per LRFD 5.2.");
      else
         os << _T("Slab concrete density is out of range for Lightweight Concrete per LRFD 5.2.");

      std::_tstring strMsg = os.str();

      pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Slab,pgsConcreteStrengthStatusItem::Density,0,0,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
      pStatusCenter->Add(pStatusItem);
   }

   Float64 max_agg_size = pLimits->GetMaxConcreteAggSize(slabConcreteType);
   Float64 MaxAggSize = ::ConvertFromSysUnits(max_agg_size,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

   if ( max_agg_size < m_pSlabConc->GetMaxAggregateSize() && !IsEqual(max_agg_size,m_pSlabConc->GetMaxAggregateSize()))
   {
      std::_tostringstream os;
      os << _T("Slab concrete aggregate size exceeds the normal value of ") << MaxAggSize << _T(" ") << pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag();

      std::_tstring strMsg = os.str();

      pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Slab,pgsConcreteStrengthStatusItem::AggSize,0,0,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
      pStatusCenter->Add(pStatusItem);
   }


   // Check girder concrete
   bool bThrow = false;
   std::_tstring strMsg;

   pSpan = pBridgeDesc->GetSpan(0);
   while ( pSpan != NULL )
   {
      GirderIndexType nGirders = pSpan->GetGirderCount();
      if ( pBridgeDesc->UseSameNumberOfGirdersInAllSpans() )
         nGirders = pBridgeDesc->GetGirderCount();

      SpanIndexType spanIdx = pSpan->GetSpanIndex();

      for ( GirderIndexType girderIdx = 0; girderIdx < nGirders; girderIdx++ )
      {
         SpanGirderHashType key = HashSpanGirder(spanIdx,girderIdx);

         pgsTypes::ConcreteType gdrConcreteType = (pgsTypes::ConcreteType)m_pGdrConc[key]->GetType();

         Float64 max_wc = pLimits->GetMaxConcreteUnitWeight(gdrConcreteType);

         Float64 max_girder_fci = pLimits->GetMaxGirderFci(gdrConcreteType);
         Float64 fciGirderMax = ::ConvertFromSysUnits(max_girder_fci,pDisplayUnits->GetStressUnit().UnitOfMeasure);
         Float64 max_girder_fc = pLimits->GetMaxGirderFc(gdrConcreteType);
         Float64 fcGirderMax = ::ConvertFromSysUnits(max_girder_fc,pDisplayUnits->GetStressUnit().UnitOfMeasure);

         Float64 max_agg_size = pLimits->GetMaxConcreteAggSize(gdrConcreteType);
         Float64 MaxAggSize = ::ConvertFromSysUnits(max_agg_size,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

         if ( m_pGdrConc[key]->GetFc() < fcMin )
         {
            bThrow = false;

            std::_tostringstream os;
            os << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(girderIdx) << _T(": Girder concrete strength is less that permitted by LRFD 5.4.2.1");
            strMsg = os.str();
            pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Girder,pgsConcreteStrengthStatusItem::FinalStrength,spanIdx,girderIdx,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
            pStatusCenter->Add(pStatusItem);
         }

         if (  max_girder_fci < m_pGdrReleaseConc[key]->GetFc() )
         {
            std::_tostringstream os;
            os << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(girderIdx) << _T(": Girder concrete release strength exceeds the normal value of ") << fciGirderMax << _T(" ") << pDisplayUnits->GetStressUnit().UnitOfMeasure.UnitTag();

            strMsg = os.str();

            pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Girder,pgsConcreteStrengthStatusItem::ReleaseStrength,spanIdx,girderIdx,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
            pStatusCenter->Add(pStatusItem);

            bThrow = false;
         }

         if (  max_girder_fc < m_pGdrConc[key]->GetFc() )
         {
            std::_tostringstream os;
            os << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(girderIdx) << _T(": Girder concrete strength exceeds the normal value of ") << fcGirderMax << _T(" ") << pDisplayUnits->GetStressUnit().UnitOfMeasure.UnitTag();

            strMsg = os.str();

            pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Girder,pgsConcreteStrengthStatusItem::FinalStrength,spanIdx,girderIdx,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
            pStatusCenter->Add(pStatusItem);

            bThrow = false;
         }

         Float64 wc = m_pGdrConc[key]->GetDensity();
         if ( max_wc < wc && !IsEqual(max_wc,wc,0.0001))
         {
            std::_tostringstream os;
            os << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(girderIdx) << _T(": Girder concrete density for strength calcuations exceeds the normal value of ") << MaxWc << _T(" ") << pDisplayUnits->GetDensityUnit().UnitOfMeasure.UnitTag();

            strMsg = os.str();

            pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Girder,pgsConcreteStrengthStatusItem::Density,spanIdx,girderIdx,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
            pStatusCenter->Add(pStatusItem);

            bThrow = false;
         }

         wc = m_pGdrConc[key]->GetDensityForWeight();
         if ( max_wc < wc && !IsEqual(max_wc,wc,0.0001) )
         {
            std::_tostringstream os;
            os << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(girderIdx) << _T(": Girder concrete density for weight calcuations exceeds the normal value of ") << MaxWc << _T(" ") << pDisplayUnits->GetDensityUnit().UnitOfMeasure.UnitTag();

            strMsg = os.str();

            pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Girder,pgsConcreteStrengthStatusItem::DensityForWeight,spanIdx,girderIdx,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
            pStatusCenter->Add(pStatusItem);

            bThrow = false;
         }

         if ( !IsConcreteDensityInRange(m_pGdrConc[key]->GetDensity(), (pgsTypes::ConcreteType)m_pGdrConc[key]->GetType()) )
         {
            std::_tostringstream os;
            if ( m_pGdrConc[key]->GetType() == pgsTypes::Normal )
               os << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(girderIdx) << _T(": concrete density is out of range for Normal Weight Concrete per LRFD 5.2.");
            else
               os << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(girderIdx) << _T(": concrete density is out of range for Lightweight Concrete per LRFD 5.2.");

            strMsg = os.str();

            pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Girder,pgsConcreteStrengthStatusItem::DensityForWeight,spanIdx,girderIdx,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
            pStatusCenter->Add(pStatusItem);

            bThrow = false;
         }

         if ( max_agg_size < m_pGdrConc[key]->GetMaxAggregateSize() && !IsEqual(max_agg_size,m_pGdrConc[key]->GetMaxAggregateSize()) )
         {
            std::_tostringstream os;
            os << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(girderIdx) << _T(": Girder concrete aggregate size exceeds the normal value of ") << MaxAggSize << _T(" ") << pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag();

            strMsg = os.str();

            pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Girder,pgsConcreteStrengthStatusItem::AggSize,spanIdx,girderIdx,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
            pStatusCenter->Add(pStatusItem);

            bThrow = false;
         }

         if ( m_pGdrConc[key]->GetE() < m_pGdrReleaseConc[key]->GetE() )
         {
            std::_tostringstream os;
            os << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(girderIdx) << _T(": Ec (") 
               << FormatDimension(m_pGdrConc[key]->GetE(), pDisplayUnits->GetModEUnit()).GetBuffer() << _T(") is less than Eci (")
               << FormatDimension(m_pGdrReleaseConc[key]->GetE(),pDisplayUnits->GetModEUnit()).GetBuffer() << _T(")");

            strMsg = os.str();

            pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Girder,pgsConcreteStrengthStatusItem::Modulus,spanIdx,girderIdx,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
            pStatusCenter->Add(pStatusItem);

            bThrow = false;
         }
      }

      pSpan = pSpan->GetNextPier()->GetNextSpan();
   }


   if ( bThrow )
   {
      strMsg += std::_tstring(_T("\nSee Status Center for Details"));
      THROW_UNWIND(strMsg.c_str(),-1);
   }

   return true;
}

bool CBridgeAgentImp::IsConcreteDensityInRange(Float64 density,pgsTypes::ConcreteType type)
{
   if ( type == pgsTypes::Normal )
   {
      return ( GetNWCDensityLimit() <= density );
   }
   else
   {
      return (density <= GetLWCDensityLimit());
   }
}

void CBridgeAgentImp::InvalidateUserLoads()
{
   m_bUserLoadsValidated = false;

   // remove our items from the status center
   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   pStatusCenter->RemoveByStatusGroupID(m_LoadStatusGroupID);
}

void CBridgeAgentImp::ValidateUserLoads()
{
      // Bridge model must be valid first
         VALIDATE(BRIDGE);

   if (!m_bUserLoadsValidated)
   {
      // first make sure our data is wiped
      for(Int16 i=0; i<3; i++)
      {
         m_PointLoads[i].clear();
         m_DistributedLoads[i].clear();
         m_MomentLoads[i].clear();
      }

      ValidatePointLoads();
      ValidateDistributedLoads();
      ValidateMomentLoads();

      m_bUserLoadsValidated = true;
   }
}

void CBridgeAgentImp::ValidatePointLoads()
{
   SpanIndexType num_spans = this->GetSpanCount();

   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   GET_IFACE( IUserDefinedLoadData, pUdl );

   CollectionIndexType num_pl = pUdl->GetPointLoadCount();
   for(CollectionIndexType ipl=0; ipl<num_pl; ipl++)
   {
      const CPointLoadData& rpl = pUdl->GetPointLoad(ipl);

      // need to loop over all spans if that is what is selected - user a vector to store span numbers
      std::vector<SpanIndexType> spans;
      if (rpl.m_Span==ALL_SPANS)
      {
         for (SpanIndexType i=0; i<num_spans; i++)
            spans.push_back(i);
      }
      else
      {
         if(rpl.m_Span+1>num_spans)
         {
            CString strMsg;
            strMsg.Format(_T("Span %d for point load is out of range. Max span number is %d. This load will be ignored."), LABEL_SPAN(rpl.m_Span),num_spans);
            pgsPointLoadStatusItem* pStatusItem = new pgsPointLoadStatusItem(ipl,m_LoadStatusGroupID,m_scidPointLoadWarning,strMsg,rpl.m_Span,rpl.m_Girder);
            pStatusCenter->Add(pStatusItem);
            continue; // break out of this cycle
         }
         else
         {
            spans.push_back(rpl.m_Span);
         }
      }

      for (std::vector<SpanIndexType>::iterator itspn=spans.begin(); itspn!=spans.end(); itspn++)
      {
         SpanIndexType span = *itspn;

         GirderIndexType num_gdrs = this->GetGirderCount(span);

         std::vector<GirderIndexType> girders;
         if (rpl.m_Girder==ALL_GIRDERS)
         {
            for (GirderIndexType i=0; i<num_gdrs; i++)
               girders.push_back(i);
         }
         else
         {
            if(rpl.m_Girder+1>num_gdrs)
            {
               CString strMsg;
               strMsg.Format(_T("Girder %s for point load is out of range. Max girder number is %s. This load will be ignored."), LABEL_GIRDER(rpl.m_Girder), LABEL_GIRDER(num_gdrs-1));
               pgsPointLoadStatusItem* pStatusItem = new pgsPointLoadStatusItem(ipl,m_LoadStatusGroupID,m_scidPointLoadWarning,strMsg,span,rpl.m_Girder);
               pStatusCenter->Add(pStatusItem);
               continue;
            }
            else
            {
               girders.push_back(rpl.m_Girder);
            }
         }

         for (std::vector<GirderIndexType>::iterator itgdr=girders.begin(); itgdr!=girders.end(); itgdr++)
         {
            GirderIndexType girder = *itgdr;

            UserPointLoad upl;
            upl.m_LoadCase = Project2BridgeLoads(rpl.m_LoadCase);
            upl.m_Description = rpl.m_Description;

            // only a light warning for zero loads - don't bail out
            if (IsZero(rpl.m_Magnitude))
            {
               CString strMsg;
               strMsg.Format(_T("Magnitude of point load is zero"));
               pgsPointLoadStatusItem* pStatusItem = new pgsPointLoadStatusItem(ipl,m_LoadStatusGroupID,m_scidPointLoadWarning,strMsg,span,girder);
               pStatusCenter->Add(pStatusItem);
            }

            upl.m_Magnitude = rpl.m_Magnitude;

            Float64 span_length = GetSpanLength(span,girder);

            if (rpl.m_Fractional)
            {
               if(rpl.m_Location>=0.0 && rpl.m_Location<=1.0)
               {
                  upl.m_Location = rpl.m_Location * span_length;
               }
               else
               {
                  CString strMsg;
                  strMsg.Format(_T("Fractional location value for point load is out of range. Value must range from 0.0 to 1.0. This load will be ignored."));
                  pgsPointLoadStatusItem* pStatusItem = new pgsPointLoadStatusItem(ipl,m_LoadStatusGroupID,m_scidPointLoadWarning,strMsg,span,girder);
                  pStatusCenter->Add(pStatusItem);
                  continue;
               }
            }
            else
            {
               if(rpl.m_Location>=0.0 && rpl.m_Location<=span_length)
               {
                  upl.m_Location = rpl.m_Location;
               }
               else
               {
                  CString strMsg;
                  strMsg.Format(_T("Location value for point load is out of range. Value must range from 0.0 to span length. This load will be ignored."));
                  pgsPointLoadStatusItem* pStatusItem = new pgsPointLoadStatusItem(ipl,m_LoadStatusGroupID,m_scidPointLoadWarning,strMsg,span,girder);
                  pStatusCenter->Add(pStatusItem);
                  continue;
               }
            }

            // add a point of interest at this load location
            Float64 loc = upl.m_Location + GetGirderStartConnectionLength(span,girder);
            pgsPointOfInterest poi(span,girder,loc);
            poi.AddStage(pgsTypes::CastingYard,USERLOADPOI_ATTR);
            poi.AddStage(pgsTypes::BridgeSite1,USERLOADPOI_ATTR);
            poi.AddStage(pgsTypes::BridgeSite2,USERLOADPOI_ATTR);
            poi.AddStage(pgsTypes::BridgeSite3,USERLOADPOI_ATTR);
            m_PoiMgr.AddPointOfInterest( poi );

            // put load into our collection
            SpanGirderHashType hashval = HashSpanGirder(span, girder);
            PointLoadCollection& plc = m_PointLoads[(int)rpl.m_Stage];

            PointLoadCollection::iterator itp = plc.find(hashval);
            if ( itp==plc.end() )
            {
               // not found, must insert
               std::pair<PointLoadCollection::iterator, bool> itbp = plc.insert(PointLoadCollection::value_type(hashval, std::vector<UserPointLoad>() ));
               ATLASSERT(itbp.second);
               itbp.first->second.push_back(upl);
            }
            else
            {
               itp->second.push_back(upl);
            }
         }
      }
   }
}

void CBridgeAgentImp::ValidateDistributedLoads()
{
   SpanIndexType num_spans = this->GetSpanCount();

   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   GET_IFACE( IUserDefinedLoadData, pUdl );

   CollectionIndexType num_pl = pUdl->GetDistributedLoadCount();
   for(CollectionIndexType ipl=0; ipl<num_pl; ipl++)
   {
      const CDistributedLoadData& rpl = pUdl->GetDistributedLoad(ipl);

      // need to loop over all spans if that is what is selected - user a vector to store span numbers
      std::vector<SpanIndexType> spans;
      if (rpl.m_Span==ALL_SPANS)
      {
         for (SpanIndexType i=0; i<num_spans; i++)
            spans.push_back(i);
      }
      else
      {
         if(rpl.m_Span+1>num_spans)
         {
            CString strMsg;
            strMsg.Format(_T("Span %d for Distributed load is out of range. Max span number is %d. This load will be ignored."), LABEL_SPAN(rpl.m_Span),num_spans);
            pgsDistributedLoadStatusItem* pStatusItem = new pgsDistributedLoadStatusItem(ipl,m_LoadStatusGroupID,m_scidDistributedLoadWarning,strMsg,rpl.m_Span,rpl.m_Girder);
            pStatusCenter->Add(pStatusItem);
            continue; // break out of this cycle
         }
         else
         {
            spans.push_back(rpl.m_Span);
         }
      }

      for (std::vector<SpanIndexType>::iterator itspn=spans.begin(); itspn!=spans.end(); itspn++)
      {
         SpanIndexType span = *itspn;

         GirderIndexType num_gdrs = this->GetGirderCount(span);

         std::vector<GirderIndexType> girders;
         if (rpl.m_Girder==ALL_GIRDERS)
         {
            for (GirderIndexType i=0; i<num_gdrs; i++)
               girders.push_back(i);
         }
         else
         {
            if(rpl.m_Girder+1>num_gdrs)
            {
               CString strMsg;
               strMsg.Format(_T("Girder %s for Distributed load is out of range. Max girder number is %s. This load will be ignored."), LABEL_GIRDER(rpl.m_Girder), LABEL_GIRDER(num_gdrs-1));
               pgsDistributedLoadStatusItem* pStatusItem = new pgsDistributedLoadStatusItem(ipl,m_LoadStatusGroupID,m_scidDistributedLoadWarning,strMsg,span,rpl.m_Girder);
               pStatusCenter->Add(pStatusItem);
               continue;
            }
            else
            {
               girders.push_back(rpl.m_Girder);
            }
         }

         for (std::vector<GirderIndexType>::iterator itgdr=girders.begin(); itgdr!=girders.end(); itgdr++)
         {
            GirderIndexType girder = *itgdr;

            Float64 span_length = GetSpanLength(span,girder);

            UserDistributedLoad upl;
            upl.m_LoadCase = Project2BridgeLoads(rpl.m_LoadCase);
            upl.m_Description = rpl.m_Description;

            if (rpl.m_Type==UserLoads::Uniform)
            {
               if (IsZero(rpl.m_WStart) && IsZero(rpl.m_WEnd))
               {
                  CString strMsg;
                  strMsg.Format(_T("Magnitude of Distributed load is zero"));
                  pgsDistributedLoadStatusItem* pStatusItem = new pgsDistributedLoadStatusItem(ipl,m_LoadStatusGroupID,m_scidDistributedLoadWarning,strMsg,span,girder);
                  pStatusCenter->Add(pStatusItem);
               }

                  // uniform load
                  upl.m_WStart = rpl.m_WStart;
                  upl.m_WEnd   = rpl.m_WStart;

                  upl.m_StartLocation = 0.0;
                  upl.m_EndLocation   = span_length;
            }
            else
            {
               // only a light warning for zero loads - don't bail out
               if (IsZero(rpl.m_WStart) && IsZero(rpl.m_WEnd))
               {
                  CString strMsg;
                  strMsg.Format(_T("Magnitude of Distributed load is zero"));
                  pgsDistributedLoadStatusItem* pStatusItem = new pgsDistributedLoadStatusItem(ipl,m_LoadStatusGroupID,m_scidDistributedLoadWarning,strMsg,span,girder);
                  pStatusCenter->Add(pStatusItem);
               }

               upl.m_WStart = rpl.m_WStart;
               upl.m_WEnd   = rpl.m_WEnd;

               // location
               if(rpl.m_StartLocation >= rpl.m_EndLocation)
               {
                  CString strMsg;
                  strMsg.Format(_T("Start locaton of distributed load is greater than end location. This load will be ignored."));
                  pgsDistributedLoadStatusItem* pStatusItem = new pgsDistributedLoadStatusItem(ipl,m_LoadStatusGroupID,103,strMsg,span,girder);
                  pStatusCenter->Add(pStatusItem);
                  continue;
               }

               if (rpl.m_Fractional)
               {
                  if(rpl.m_StartLocation>=0.0 && rpl.m_StartLocation<=1.0 &&
                     rpl.m_EndLocation>=0.0   && rpl.m_EndLocation<=1.0)
                  {
                     upl.m_StartLocation = rpl.m_StartLocation * span_length;
                     upl.m_EndLocation   = rpl.m_EndLocation * span_length;
                  }
                  else
                  {
                     CString strMsg;
                     strMsg.Format(_T("Fractional location value for Distributed load is out of range. Value must range from 0.0 to 1.0. This load will be ignored."));
                     pgsDistributedLoadStatusItem* pStatusItem = new pgsDistributedLoadStatusItem(ipl,m_LoadStatusGroupID,m_scidDistributedLoadWarning,strMsg,span,girder);
                     pStatusCenter->Add(pStatusItem);
                     continue;
                  }
               }
               else
               {
                  if(rpl.m_StartLocation>=0.0 && rpl.m_StartLocation<=span_length &&
                     rpl.m_EndLocation>=0.0   && rpl.m_EndLocation<=span_length+TOL)
                  {
                     upl.m_StartLocation = rpl.m_StartLocation;

                     // fudge a bit if user entered a slightly high value
                     if (rpl.m_EndLocation<span_length)
                        upl.m_EndLocation   = rpl.m_EndLocation;
                     else
                        upl.m_EndLocation   = span_length;

                  }
                  else
                  {
                     CString strMsg;
                     strMsg.Format(_T("Location value for Distributed load is out of range. Value must range from 0.0 to span length. This load will be ignored."));
                     pgsDistributedLoadStatusItem* pStatusItem = new pgsDistributedLoadStatusItem(ipl,m_LoadStatusGroupID,m_scidDistributedLoadWarning,strMsg,span,girder);
                     pStatusCenter->Add(pStatusItem);
                     continue;
                  }
               }
            }

            SpanGirderHashType hashval = HashSpanGirder(span, girder);
            DistributedLoadCollection& plc = m_DistributedLoads[(int)rpl.m_Stage];

            DistributedLoadCollection::iterator itp = plc.find(hashval);
            if ( itp==plc.end() )
            {
               // not found, must insert
               std::pair<DistributedLoadCollection::iterator, bool> itbp = plc.insert(DistributedLoadCollection::value_type(hashval, std::vector<UserDistributedLoad>() ));
               ATLASSERT(itbp.second);
               itbp.first->second.push_back(upl);
            }
            else
            {
               itp->second.push_back(upl);
            }
         }
      }
   }
}

void CBridgeAgentImp::ValidateMomentLoads()
{
   SpanIndexType num_spans = this->GetSpanCount();

   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   GET_IFACE( IUserDefinedLoadData, pUdl );

   CollectionIndexType num_pl = pUdl->GetMomentLoadCount();
   for(CollectionIndexType ipl=0; ipl<num_pl; ipl++)
   {
      const CMomentLoadData& rpl = pUdl->GetMomentLoad(ipl);

      // need to loop over all spans if that is what is selected - user a vector to store span numbers
      std::vector<SpanIndexType> spans;
      if (rpl.m_Span==ALL_SPANS)
      {
         for (SpanIndexType i=0; i<num_spans; i++)
            spans.push_back(i);
      }
      else
      {
         if(rpl.m_Span+1>num_spans)
         {
            CString strMsg;
            strMsg.Format(_T("Span %d for moment load is out of range. Max span number is %d. This load will be ignored."), LABEL_SPAN(rpl.m_Span),num_spans);
            pgsMomentLoadStatusItem* pStatusItem = new pgsMomentLoadStatusItem(ipl,m_LoadStatusGroupID,m_scidMomentLoadWarning,strMsg,rpl.m_Span,rpl.m_Girder);
            pStatusCenter->Add(pStatusItem);
            continue; // break out of this cycle
         }
         else
         {
            spans.push_back(rpl.m_Span);
         }
      }

      for (std::vector<SpanIndexType>::iterator itspn=spans.begin(); itspn!=spans.end(); itspn++)
      {
         SpanIndexType span = *itspn;

         GirderIndexType num_gdrs = this->GetGirderCount(span);

         std::vector<GirderIndexType> girders;
         if (rpl.m_Girder==ALL_GIRDERS)
         {
            for (GirderIndexType i=0; i<num_gdrs; i++)
               girders.push_back(i);
         }
         else
         {
            if(rpl.m_Girder+1>num_gdrs)
            {
               CString strMsg;
               strMsg.Format(_T("Girder %s for moment load is out of range. Max girder number is %s. This load will be ignored."), LABEL_GIRDER(rpl.m_Girder), LABEL_GIRDER(num_gdrs-1));
               pgsMomentLoadStatusItem* pStatusItem = new pgsMomentLoadStatusItem(ipl,m_LoadStatusGroupID,m_scidMomentLoadWarning,strMsg,span,rpl.m_Girder);
               pStatusCenter->Add(pStatusItem);
               continue;
            }
            else
            {
               girders.push_back(rpl.m_Girder);
            }
         }

         for (std::vector<GirderIndexType>::iterator itgdr=girders.begin(); itgdr!=girders.end(); itgdr++)
         {
            GirderIndexType girder = *itgdr;

            UserMomentLoad upl;
            upl.m_LoadCase = Project2BridgeLoads(rpl.m_LoadCase);
            upl.m_Description = rpl.m_Description;

            // only a light warning for zero loads - don't bail out
            if (IsZero(rpl.m_Magnitude))
            {
               CString strMsg;
               strMsg.Format(_T("Magnitude of moment load is zero"));
               pgsMomentLoadStatusItem* pStatusItem = new pgsMomentLoadStatusItem(ipl,m_LoadStatusGroupID,m_scidMomentLoadWarning,strMsg,span,girder);
               pStatusCenter->Add(pStatusItem);
            }

            upl.m_Magnitude = rpl.m_Magnitude;

            Float64 span_length = GetSpanLength(span,girder);

            if (rpl.m_Fractional)
            {
               if(rpl.m_Location>=0.0 && rpl.m_Location<=1.0)
               {
                  upl.m_Location = rpl.m_Location * span_length;
               }
               else
               {
                  CString strMsg;
                  strMsg.Format(_T("Fractional location value for moment load is out of range. Value must range from 0.0 to 1.0. This load will be ignored."));
                  pgsMomentLoadStatusItem* pStatusItem = new pgsMomentLoadStatusItem(ipl,m_LoadStatusGroupID,m_scidMomentLoadWarning,strMsg,span,girder);
                  pStatusCenter->Add(pStatusItem);
                  continue;
               }
            }
            else
            {
               if(rpl.m_Location>=0.0 && rpl.m_Location<=span_length)
               {
                  upl.m_Location = rpl.m_Location;
               }
               else
               {
                  CString strMsg;
                  strMsg.Format(_T("Location value for moment load is out of range. Value must range from 0.0 to span length. This load will be ignored."));
                  pgsMomentLoadStatusItem* pStatusItem = new pgsMomentLoadStatusItem(ipl,m_LoadStatusGroupID,m_scidMomentLoadWarning,strMsg,span,girder);
                  pStatusCenter->Add(pStatusItem);
                  continue;
               }
            }

            // add a point of interest at this load location
            Float64 loc = upl.m_Location + GetGirderStartConnectionLength(span,girder);
            pgsPointOfInterest poi(span, girder, loc);
            poi.AddStage(pgsTypes::BridgeSite1, USERLOADPOI_ATTR);
            poi.AddStage(pgsTypes::BridgeSite2, USERLOADPOI_ATTR);
            poi.AddStage(pgsTypes::BridgeSite3, USERLOADPOI_ATTR);
            m_PoiMgr.AddPointOfInterest( poi );

            // put load into our collection
            SpanGirderHashType hashval = HashSpanGirder(span, girder);
            MomentLoadCollection& plc = m_MomentLoads[(int)rpl.m_Stage];

            MomentLoadCollection::iterator itp = plc.find(hashval);
            if ( itp==plc.end() )
            {
               // not found, must insert
               std::pair<MomentLoadCollection::iterator, bool> itbp = plc.insert(MomentLoadCollection::value_type(hashval, std::vector<UserMomentLoad>() ));
               ATLASSERT(itbp.second);
               itbp.first->second.push_back(upl);
            }
            else
            {
               itp->second.push_back(upl);
            }
         }
      }
   }
}

void CBridgeAgentImp::ValidateGirderOrientation(SpanIndexType span,GirderIndexType gdr)
{
   ValidatePointsOfInterest(span,gdr); // calls VALIDATE(BRIDGE);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   pgsTypes::GirderOrientationType orientationType = pBridgeDesc->GetGirderOrientation();

   Float64 orientation;
   if ( orientationType == pgsTypes::Plumb )
   {
      orientation = 0.0;
   }
   else
   {
      std::vector<pgsPointOfInterest> vPoi;
      pgsPointOfInterest poi;
      Float64 station, offset;
      switch ( orientationType )
      {
      case pgsTypes::StartNormal:
         vPoi = GetPointsOfInterest(span,gdr,pgsTypes::BridgeSite3,POI_FLEXURESTRESS);
         poi = vPoi.front();
         break;

      case pgsTypes::MidspanNormal:
         vPoi = GetPointsOfInterest(span,gdr,pgsTypes::BridgeSite3,POI_MIDSPAN);
         poi = vPoi.front();
         break;

      case pgsTypes::EndNormal:
         vPoi = GetPointsOfInterest(span,gdr,pgsTypes::BridgeSite3,POI_FLEXURESTRESS);
         poi = vPoi.back();
         break;

      case pgsTypes::Plumb:
      default:
         ATLASSERT(false); // should never get here
      };

      GetStationAndOffset(poi,&station,&offset);

      MatingSurfaceIndexType nMatingSurfaces = GetNumberOfMatingSurfaces(span,gdr);
      ATLASSERT(nMatingSurfaces > 0); // going to subtract 1 below, and this is an unsigned value

      Float64 left_mating_surface_offset  = GetMatingSurfaceLocation(poi,0);
      Float64 right_mating_surface_offset = GetMatingSurfaceLocation(poi,nMatingSurfaces-1);

      Float64 left_mating_surface_width = GetMatingSurfaceWidth(poi,0);
      Float64 right_mating_surface_width = GetMatingSurfaceWidth(poi,nMatingSurfaces-1);

      Float64 left_edge_offset  = left_mating_surface_offset  - left_mating_surface_width/2;
      Float64 right_edge_offset = right_mating_surface_offset + right_mating_surface_width/2;

      Float64 distance = right_edge_offset - left_edge_offset;

      if ( IsZero(distance) )
      {
         orientation = GetCrownSlope(station,offset);
      }
      else
      {   
         Float64 ya_left  = GetElevation(station,offset+left_edge_offset);
         Float64 ya_right = GetElevation(station,offset+right_edge_offset);

         orientation = (ya_left - ya_right)/distance;
      }
   }

   CComPtr<ISuperstructureMember> ssmbr;
   GetSuperstructureMember(span,gdr,&ssmbr);

   CComPtr<ISegment> segment;
   ssmbr->get_Segment(0,&segment);

   segment->put_Orientation(orientation);
}

bool CBridgeAgentImp::BuildCogoModel()
{
   // Need to create bridge so we can get to the cogo model
   ATLASSERT(m_Bridge == NULL);
   m_Bridge.Release(); // release just incase, but it should already be done
   HRESULT hr = m_Bridge.CoCreateInstance(CLSID_GenericBridge);
   ATLASSERT(SUCCEEDED(hr));

   CComPtr<ICogoModel> cogomodel;
   m_Bridge->get_CogoModel(&cogomodel);

   GET_IFACE( IRoadwayData, pIAlignment );
   if ( pIAlignment == NULL )
      return false;

   AlignmentData2 alignment_data   = pIAlignment->GetAlignmentData2();
   ProfileData2   profile_data     = pIAlignment->GetProfileData2();
   RoadwaySectionData section_data = pIAlignment->GetRoadwaySectionData();

   CComPtr<IPathCollection> alignments;
   cogomodel->get_Alignments(&alignments);

   CComPtr<ICogoInfo> cogoinfo;
   m_Bridge->get_CogoInfo(&cogoinfo);
   CogoElementKey alignment_key;
   cogoinfo->get_AlignmentKey(&alignment_key);

   CComPtr<IPath> path;
   alignments->get_Item(alignment_key,&path);
   CComQIPtr<IAlignment> alignment(path);

   CComPtr<IProfile> profile;
   alignment->get_Profile(&profile);
   profile->Clear();

   CComPtr<IPointCollection> points;
   cogomodel->get_Points(&points);

   // Setup the alignment
   if ( alignment_data.HorzCurves.size() == 0 )
   {
      // straight alignment
      Int32 id1 = 20000;
      Int32 id2 = 20001;
      points->Add(id1,0,0,NULL); // start at point 0,0

      CComQIPtr<ILocate> locate(cogomodel);
      locate->ByDistDir(id2,id1,100.00,CComVariant(alignment_data.Direction),0.00);

      CComPtr<IPoint2d> pnt1, pnt2;
      points->get_Item(id1,&pnt1);
      points->get_Item(id2,&pnt2);

      alignment->put_RefStation(CComVariant(0.00));
      alignment->AddEx(pnt1);
      alignment->AddEx(pnt2);
   }
   else
   {
      // there are horizontal curves
      CComQIPtr<ILocate2> locate(m_CogoEngine);
      CComQIPtr<IIntersect2> intersect(m_CogoEngine);

      // start the alignment at coordinate (0,0)... 
      CComPtr<IPoint2d> pbt; // point on back tangent
      pbt.CoCreateInstance(CLSID_Point2d);
      pbt->Move(0,0);

      CComPtr<IHorzCurveCollection> curves;
      cogomodel->get_HorzCurves(&curves);

      CComPtr<IPointCollection> points;
      cogomodel->get_Points(&points);

      Float64 back_tangent = alignment_data.Direction;

      Float64 prev_curve_ST_station; // station of the Spiral-to-Tangent point of the previous curve
                                    // station of the last point on the curve and will be
                                    // compared with the station at the start of the next curve
                                    // to confirm the next curve doesn't start before the previous one ends


      // The station at this first point is the PI station of the first curve less 2 radii
      // and 2 entry spiral lengths. This is completely arbitrary but will ensure that 
      // we are starting on the back tangent
      HorzCurveData& first_curve_data = *(alignment_data.HorzCurves.begin());
      prev_curve_ST_station = first_curve_data.PIStation - 2*(first_curve_data.Radius + first_curve_data.EntrySpiral);

      CogoElementKey curveID = 1;
      CollectionIndexType curveIdx = 0;

      std::vector<HorzCurveData>::iterator iter;
      for ( iter = alignment_data.HorzCurves.begin(); iter != alignment_data.HorzCurves.end(); iter++ )
      {
         HorzCurveData& curve_data = *iter;

         Float64 pi_station = curve_data.PIStation;

         Float64 T = 0;
         if ( IsZero(curve_data.Radius) )
         {
            // this is just a PI point (no curve)
            // create a line
            if ( iter == alignment_data.HorzCurves.begin() )
            {
               // if first curve, add a point on the back tangent
               points->AddEx(curveID++,pbt);
               alignment->AddEx(pbt);
            }

            // locate the PI
            CComPtr<IPoint2d> pi;
            locate->ByDistDir(pbt, pi_station, CComVariant( back_tangent ),0.00,&pi);

            // add the PI
            points->AddEx(curveID,pi);
            alignment->AddEx(pi);

            Float64 fwd_tangent = curve_data.FwdTangent;
            if ( !curve_data.bFwdTangent )
            {
               // FwdTangent data member is the curve delta
               // compute the forward tangent direction by adding delta to the back tangent
               fwd_tangent += back_tangent;
            }

            if ( iter == alignment_data.HorzCurves.end()-1 )
            {
               // this is the last point so add one more to model the last line segment
               CComQIPtr<ILocate2> locate(m_CogoEngine);
               CComPtr<IPoint2d> pnt;
               locate->ByDistDir(pi,100.00,CComVariant(fwd_tangent),0.00,&pnt);
               points->AddEx(++curveID,pnt); // pre-increment is important
               alignment->AddEx(pnt);
            }

            back_tangent = fwd_tangent;
            prev_curve_ST_station = pi_station;
         }
         else
         {
            // a real curve

            // locate the PI
            CComPtr<IPoint2d> pi;
            locate->ByDistDir(pbt, pi_station - prev_curve_ST_station, CComVariant( back_tangent ),0.00,&pi);

            Float64 fwd_tangent = curve_data.FwdTangent;
            if ( !curve_data.bFwdTangent )
            {
               // FwdTangent data member is the curve delta
               // compute the forward tangent direction by adding delta to the back tangent
               fwd_tangent += back_tangent;
            }

            // locate a point on the foward tangent.... at which distance? it doesn't matter... use
            // the curve radius for simplicity
            CComPtr<IPoint2d> pft; // point on forward tangent
            locate->ByDistDir(pi, curve_data.Radius, CComVariant( fwd_tangent ), 0.00, &pft );

            if ( IsZero(fabs(back_tangent - fwd_tangent)) || IsEqual(fabs(back_tangent - fwd_tangent),M_PI) )
            {
               std::_tostringstream os;
               os << _T("The central angle of curve ") << curveID << _T(" is 0 or 180 degrees");
               std::_tstring strMsg = os.str();
               pgsAlignmentDescriptionStatusItem* p_status_item = new pgsAlignmentDescriptionStatusItem(m_StatusGroupID,m_scidAlignmentError,0,strMsg.c_str());
               GET_IFACE(IEAFStatusCenter,pStatusCenter);
               pStatusCenter->Add(p_status_item);
               strMsg += std::_tstring(_T("\nSee Status Center for Details"));
               THROW_UNWIND(strMsg.c_str(),-1);
            }

            CComPtr<IHorzCurve> hc;
            HRESULT hr = curves->Add(curveID,pbt,pi,pft,curve_data.Radius,curve_data.EntrySpiral,curve_data.ExitSpiral,&hc);
            ATLASSERT( SUCCEEDED(hr) );

            m_HorzCurveKeys.insert(std::make_pair(curveIdx,curveID));

            // Make sure this curve and the previous curve don't overlap
            hc->get_BkTangentLength(&T);
            if ( 0 < curveIdx )
            {
               // tangent to spiral station
               Float64 TS_station = pi_station - T;

               if ( TS_station < prev_curve_ST_station )
               {
                  // this curve starts before the previous curve ends
                  if ( IsEqual(TS_station,prev_curve_ST_station, ::ConvertToSysUnits(0.01,unitMeasure::Feet) ) )
                  {
                     // these 2 stations are within a 0.01 ft of each other... tweak this curve so it
                     // starts where the previous curve ends
                     CComPtr<IHorzCurve> prev_curve, this_curve;
                     curves->get_Item(curveID-1,&prev_curve);
                     CComPtr<IPoint2d> pntST;
                     prev_curve->get_ST(&pntST);

                     CComPtr<IPoint2d> pntTS;
                     hc->get_TS(&pntTS);

                     Float64 stX,stY,tsX,tsY;
                     pntST->Location(&stX,&stY);
                     pntTS->Location(&tsX,&tsY);

                     hc->Offset(stX-tsX,stY-tsY);
         
#if defined _DEBUG
                     pntTS.Release();
                     hc->get_TS(&pntTS);
                     pntTS->Location(&tsX,&tsY);
                     ATLASSERT(IsEqual(tsX,stX) && IsEqual(tsY,stY));
#endif
                     std::_tostringstream os;
                     os << _T("Horizontal curve ") << (curveIdx+1) << _T(" begins before curve ") << (curveIdx) << _T(" ends. Curve ") << (curveIdx+1) << _T(" has been adjusted.");
                     std::_tstring strMsg = os.str();
                     
                     pgsAlignmentDescriptionStatusItem* p_status_item = new pgsAlignmentDescriptionStatusItem(m_StatusGroupID,m_scidAlignmentWarning,0,strMsg.c_str());
                     GET_IFACE(IEAFStatusCenter,pStatusCenter);
                     pStatusCenter->Add(p_status_item);

                     strMsg += std::_tstring(_T("\nSee Status Center for Details"));
                  }
                  else
                  {
                     std::_tostringstream os;
                     os << _T("Horizontal curve ") << (curveIdx+1) << _T(" begins before curve ") << (curveIdx) << _T(" ends. Correct the curve data before proceeding");
                     std::_tstring strMsg = os.str();
                     
                     pgsAlignmentDescriptionStatusItem* p_status_item = new pgsAlignmentDescriptionStatusItem(m_StatusGroupID,m_scidAlignmentError,0,strMsg.c_str());
                     GET_IFACE(IEAFStatusCenter,pStatusCenter);
                     pStatusCenter->Add(p_status_item);

                     strMsg += std::_tstring(_T("\nSee Status Center for Details"));
                  }
               }
            }

            alignment->AddEx(hc);

            // determine the station of the ST point because this point will serve
            // as the next point on back tangent
            Float64 L;
            hc->get_TotalLength(&L);

            back_tangent = fwd_tangent;
            pbt.Release();
            hc->get_ST(&pbt);
            prev_curve_ST_station = pi_station - T + L;
         }

         if ( curveIdx == 0 )
         {
            // this is the first curve so set the reference station at the TS 
            alignment->put_RefStation( CComVariant(pi_station - T) );
         }

         curveID++;
         curveIdx++;  
      }
   }

   // move the alignment into the correct position

   // get coordinate at the station the user wants to use as the reference station
   CComPtr<IPoint2d> objRefPoint1;
   alignment->LocatePoint(CComVariant(alignment_data.RefStation),omtAlongDirection, 0.00,CComVariant(0.00),&objRefPoint1);

   // create a point where the reference station should pass through
   CComPtr<IPoint2d> objRefPoint2;
   objRefPoint2.CoCreateInstance(CLSID_Point2d);
   objRefPoint2->Move(alignment_data.xRefPoint,alignment_data.yRefPoint);

   // measure the distance and direction between the point on alignment and the desired ref point
   CComQIPtr<IMeasure2> measure(m_CogoEngine);
   Float64 distance;
   CComPtr<IDirection> objDirection;
   measure->Inverse(objRefPoint1,objRefPoint2,&distance,&objDirection);

   // move the alignment such that it passes through the ref point
   alignment->Move(distance,objDirection);

#if defined _DEBUG
   CComPtr<IPoint2d> objPnt;
   alignment->LocatePoint(CComVariant(alignment_data.RefStation),omtAlongDirection, 0.0,CComVariant(0.0),&objPnt);
   Float64 x1,y1,x2,y2;
   objPnt->Location(&x1,&y1);
   objRefPoint2->Location(&x2,&y2);
   ATLASSERT(IsZero(x1-x2));
   ATLASSERT(IsZero(y1-y2));

   ATLASSERT(IsZero(x1-alignment_data.xRefPoint));
   ATLASSERT(IsZero(y1-alignment_data.yRefPoint));
#endif

   // Setup the profile
   if ( profile_data.VertCurves.size() == 0 )
   {
      CComPtr<IProfilePoint> ppStart, ppEnd;
      ppStart.CoCreateInstance(CLSID_ProfilePoint);
      ppEnd.CoCreateInstance(CLSID_ProfilePoint);

      ppStart->put_Station(CComVariant(profile_data.Station));
      ppStart->put_Elevation(profile_data.Elevation);

      Float64 L = 100;
      ppEnd->put_Station(CComVariant(profile_data.Station + L));
      ppEnd->put_Elevation( profile_data.Elevation + profile_data.Grade*L );

      profile->AddEx(ppStart);
      profile->AddEx(ppEnd);
   }
   else
   {
      // there are vertical curves
      Float64 pbg_station   = profile_data.Station;
      Float64 pbg_elevation = profile_data.Elevation;
      Float64 entry_grade   = profile_data.Grade;

      Float64 prev_EVC = pbg_station;
      
      CComPtr<IVertCurveCollection> vcurves;
      cogomodel->get_VertCurves(&vcurves);

      CComPtr<IProfilePointCollection> profilepoints;
      cogomodel->get_ProfilePoints(&profilepoints);

      CogoElementKey curveID = 1;
      CollectionIndexType curveIdx = 0;

      std::vector<VertCurveData>::iterator iter;
      for ( iter = profile_data.VertCurves.begin(); iter != profile_data.VertCurves.end(); iter++ )
      {
         VertCurveData& curve_data = *iter;

         Float64 L1 = curve_data.L1;
         Float64 L2 = curve_data.L2;

         // if L2 is zero, interpert that as a symmetrical vertical curve with L1
         // being the full curve length.
         // Cut L1 in half and assign half to L2
         if ( IsZero(L2) )
         {
            L1 /= 2;
            L2 = L1;
         }

         CComPtr<IProfilePoint> pbg; // point on back grade
         pbg.CoCreateInstance(CLSID_ProfilePoint);

         if( curveID == 1 && IsEqual(pbg_station,curve_data.PVIStation) )
         {
            // it is a common input _T("mistake") to start with the PVI of the first curve...
            // project the begining point back
            pbg_station   = pbg_station - 2*L1;
            pbg_elevation = pbg_elevation - 2*L1*entry_grade;
            pbg->put_Station(CComVariant(pbg_station));
            pbg->put_Elevation(pbg_elevation);

            prev_EVC = pbg_station;
         }
         else
         {
            pbg->put_Station(CComVariant(pbg_station));
            pbg->put_Elevation(pbg_elevation);
         }

         // locate the PVI
         Float64 pvi_station   = curve_data.PVIStation;
         Float64 pvi_elevation = pbg_elevation + entry_grade*(pvi_station - pbg_station);

         CComPtr<IProfilePoint> pvi;
         pvi.CoCreateInstance(CLSID_ProfilePoint);
         pvi->put_Station(CComVariant(pvi_station));
         pvi->put_Elevation(pvi_elevation);

         if ( IsZero(L1) && IsZero(L2) )
         {
            // add a profile point
            if ( iter == profile_data.VertCurves.begin() )
            {
               // this is the first item so we need a point before this to model the entry grade
               CComPtr<IProfilePoint> pbg;
               pbg.CoCreateInstance(CLSID_ProfilePoint);
               pbg->put_Station(CComVariant(pvi_station - 100.));
               pbg->put_Elevation(pvi_elevation - 100.0*entry_grade);
               profilepoints->AddEx(curveID++,pbg);
               profile->AddEx(pbg);
            }

            profilepoints->AddEx(curveID,pvi);
            profile->AddEx(pvi);

            if ( iter == profile_data.VertCurves.end()-1 )
            {
               // this is the last point ... need to add a Profile point on the exit grade
               CComPtr<IProfilePoint> pfg;
               pfg.CoCreateInstance(CLSID_ProfilePoint);
               pfg->put_Station(CComVariant(pvi_station + 100.));
               pfg->put_Elevation(pvi_elevation + 100.0*curve_data.ExitGrade);
               profilepoints->AddEx(curveID++,pfg);
               profile->AddEx(pfg);
            }

            pbg_station   = pvi_station;
            pbg_elevation = pvi_elevation;
            entry_grade   = curve_data.ExitGrade;

            prev_EVC = pvi_station;
         }
         else
         {
            // add a vertical curve
            Float64 pfg_station = pvi_station + L2;
            Float64 pfg_elevation = pvi_elevation + curve_data.ExitGrade*L2;

            CComPtr<IProfilePoint> pfg; // point on forward grade
            pfg.CoCreateInstance(CLSID_ProfilePoint);
            pfg->put_Station(CComVariant(pfg_station));
            pfg->put_Elevation(pfg_elevation);

            Float64 BVC = pvi_station - L1;
            Float64 EVC = pvi_station + L2;
            Float64 tolerance = ::ConvertToSysUnits(0.006,unitMeasure::Feet); // sometimes users enter the BVC as the start point
                                                                              // and the numbers work out such that it differs by 0.01ft
                                                                              // select a tolerance so that this isn't a problem
            if( IsLT(BVC,prev_EVC,tolerance) || IsLT(pvi_station,prev_EVC,tolerance) || IsLT(EVC,prev_EVC,tolerance) )
            {
               // some part of this curve is before the end of the previous curve
               std::_tostringstream os;

               if ( curveID == 1 )
                  os << _T("Vertical Curve ") << curveID << _T(" begins before the profile reference point.");
               else
                  os << _T("Vertical curve ") << curveID << _T(" begins before curve ") << (curveID-1) << _T(" ends.");

               std::_tstring strMsg = os.str();

               pgsAlignmentDescriptionStatusItem* p_status_item = new pgsAlignmentDescriptionStatusItem(m_StatusGroupID,m_scidAlignmentError,1,strMsg.c_str());
               GET_IFACE(IEAFStatusCenter,pStatusCenter);
               pStatusCenter->Add(p_status_item);

               strMsg += std::_tstring(_T("\nSee Status Center for Details"));
   //            THROW_UNWIND(strMsg.c_str(),-1);
            }

            CComPtr<IVertCurve> vc;
            HRESULT hr = vcurves->Add(curveID,pbg,pvi,pfg,L1,L2,&vc);
            ATLASSERT(SUCCEEDED(hr));

            m_VertCurveKeys.insert(std::make_pair(curveIdx,curveID));
           

            profile->AddEx(vc);

            Float64 g1,g2;
            vc->get_EntryGrade(&g1);
            vc->get_ExitGrade(&g2);

            if ( IsEqual(g1,g2) )
            {
               // entry and exit grades are the same
               std::_tostringstream os;
               os << _T("Entry and exit grades are the same on curve ") << curveID;
               std::_tstring strMsg = os.str();

               pgsAlignmentDescriptionStatusItem* p_status_item = new pgsAlignmentDescriptionStatusItem(m_StatusGroupID,m_scidAlignmentWarning,1,strMsg.c_str());
               GET_IFACE(IEAFStatusCenter,pStatusCenter);
               pStatusCenter->Add(p_status_item);
            }

            pbg_station   = pvi_station;
            pbg_elevation = pvi_elevation;
            entry_grade   = curve_data.ExitGrade;

            prev_EVC = EVC;
         }

         curveID++;
         curveIdx++;
      }
   }

   CComPtr<ICrossSectionCollection> sections;
   profile->get_CrossSections(&sections);

   std::vector<CrownData2>::iterator iter;
   for ( iter = section_data.Superelevations.begin(); iter != section_data.Superelevations.end(); iter++ )
   {
      CrownData2& super = *iter;
      CComPtr<ICrossSection> section;
      sections->Add(CComVariant(super.Station),super.CrownPointOffset,super.Left,super.Right,&section);
   }

   // set the alignment offset
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   Float64 alignment_offset = pBridgeDesc->GetAlignmentOffset();
   m_Bridge->put_AlignmentOffset(alignment_offset);

   return true;
}

bool CBridgeAgentImp::BuildBridgeModel()
{
   WATCH(_T("Validating Bridge Model"));

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CComPtr<IStageCollection> stages;
   m_Bridge->get_Stages(&stages);
   stages->CreateStage(GetStageName(pgsTypes::GirderPlacement),CComBSTR(_T("Girder Placement")));
   stages->CreateStage(GetStageName(pgsTypes::TemporaryStrandRemoval),CComBSTR(_T("Temporary Strand Removal")));
   stages->CreateStage(GetStageName(pgsTypes::BridgeSite1),CComBSTR(_T("Bridge Site 1")));
   stages->CreateStage(GetStageName(pgsTypes::BridgeSite2),CComBSTR(_T("Bridge Site 2")));
   stages->CreateStage(GetStageName(pgsTypes::BridgeSite3),CComBSTR(_T("Bridge Site 3")));

   if ( !LayoutPiersAndSpans(pBridgeDesc) )
      return false;

   if ( !LayoutGirders(pBridgeDesc) )
      return false;

   if ( !LayoutDeck(pBridgeDesc) )
      return false;

   if ( !LayoutTrafficBarriers(pBridgeDesc) )
      return false;

   // Now that the basic input has been generated, updated the bridge model
   // This will get all the geometry correct
   m_Bridge->UpdateBridgeModel();

   // With the geometry correct, layout the superstructure members (need span lengths)
   if ( !LayoutSuperstructureMembers(pBridgeDesc) )
      return false;

   // check bridge for errors - will throw an exception if there are errors
   CheckBridge();

   ///
   // WHY IS THIS DONE HERE?????
   ///
   // Create effective flange width tool
   CComObject<CEffectiveFlangeWidthTool>* pTool;
   HRESULT hr = CComObject<CEffectiveFlangeWidthTool>::CreateInstance(&pTool);
   pTool->Init(m_pBroker,m_StatusGroupID);
   m_EffFlangeWidthTool = pTool;
   if ( FAILED(hr) || m_EffFlangeWidthTool == NULL )
      THROW_UNWIND(_T("Custom Effective Flange Width Tool not created"),-1);

   m_SectCutTool->putref_EffectiveFlangeWidthTool(m_EffFlangeWidthTool);

   return true;
}

bool CBridgeAgentImp::LayoutPiersAndSpans(const CBridgeDescription* pBridgeDesc)
{
   USES_CONVERSION;

   const CSpanData* pSpan = pBridgeDesc->GetSpan(0);

   // layout all the spans... bridge model already has the first pier in it
   while ( pSpan )
   {
      const CPierData* pPrevPier = pSpan->GetPrevPier();
      const CPierData* pNextPier = pSpan->GetNextPier();

      Float64 span_length = pSpan->GetSpanLength();

      m_Bridge->InsertSpanAndPier(ALL_SPANS,span_length,qcbAfter,qcbRight);

      pSpan = pNextPier->GetNextSpan();
   }

   // Move the bridge to the correction station
   const CPierData* pPier = pBridgeDesc->GetPier(0);
   m_Bridge->MoveToStation(0, pPier->GetStation());


   //
   // Configure the piers
   //

   // start by gathering the data that is needed


   // orient the piers
   CComPtr<IPierCollection> piers;
   m_Bridge->get_Piers(&piers);

   CComPtr<IEnumPiers> enum_piers;
   piers->get__EnumPiers(&enum_piers);
   CComPtr<IPier> pier;

   PierIndexType pierIdx = 0;
   while ( enum_piers->Next(1,&pier,NULL) != S_FALSE )
   {
      const CPierData* pPierData = pBridgeDesc->GetPier(pierIdx);

      HRESULT hr = pier->put_Orientation(CComBSTR(pPierData->GetOrientation()));
      ATLASSERT(SUCCEEDED(hr));

      CComPtr<IAngle> skew;
      pier->get_SkewAngle(&skew);
      Float64 value;
      skew->get_Value(&value);

      if ( value < -MAX_SKEW_ANGLE || MAX_SKEW_ANGLE < value )
      {
         GET_IFACE(IEAFStatusCenter,pStatusCenter);
         std::_tostringstream os;
         os << _T("Pier ") << pierIdx+1 << _T(" has excessive Skew.");

         pgsBridgeDescriptionStatusItem* pStatusItem = new pgsBridgeDescriptionStatusItem(m_StatusGroupID,m_scidBridgeDescriptionError,0,os.str().c_str());
         pStatusCenter->Add(pStatusItem);

         os << _T("See Status Center for Details");
         THROW_UNWIND(os.str().c_str(),XREASON_NEGATIVE_GIRDER_LENGTH);
      }

      pier.Release();

      pierIdx++;
   }

   return true;
}

void CBridgeAgentImp::CreateStrandMover(LPCTSTR strGirderName,IStrandMover** ppStrandMover)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGirderEntry = pLib->GetGirderEntry(strGirderName);

   // get harped strand adjustment limits
   pgsTypes::GirderFace endTopFace, endBottomFace;
   Float64 endTopLimit, endBottomLimit;
   pGirderEntry->GetEndAdjustmentLimits(&endTopFace, &endTopLimit, &endBottomFace, &endBottomLimit);

   IBeamFactory::BeamFace etf = endTopFace    == pgsTypes::GirderBottom ? IBeamFactory::BeamBottom : IBeamFactory::BeamTop;
   IBeamFactory::BeamFace ebf = endBottomFace == pgsTypes::GirderBottom ? IBeamFactory::BeamBottom : IBeamFactory::BeamTop;

   pgsTypes::GirderFace hpTopFace, hpBottomFace;
   Float64 hpTopLimit, hpBottomLimit;
   pGirderEntry->GetHPAdjustmentLimits(&hpTopFace, &hpTopLimit, &hpBottomFace, &hpBottomLimit);

   IBeamFactory::BeamFace htf = hpTopFace    == pgsTypes::GirderBottom ? IBeamFactory::BeamBottom : IBeamFactory::BeamTop;
   IBeamFactory::BeamFace hbf = hpBottomFace == pgsTypes::GirderBottom ? IBeamFactory::BeamBottom : IBeamFactory::BeamTop;

   // only allow end adjustents if increment is non-negative
   Float64 end_increment = pGirderEntry->IsVerticalAdjustmentAllowedEnd() ?  pGirderEntry->GetEndStrandIncrement() : -1.0;
   Float64 hp_increment  = pGirderEntry->IsVerticalAdjustmentAllowedHP()  ?  pGirderEntry->GetHPStrandIncrement()  : -1.0;

   // create the strand mover
   CComPtr<IBeamFactory> beamFactory;
   pGirderEntry->GetBeamFactory(&beamFactory);
   beamFactory->CreateStrandMover(pGirderEntry->GetDimensions(), 
                                  etf, endTopLimit, ebf, endBottomLimit, 
                                  htf, hpTopLimit,  hbf, hpBottomLimit, 
                                  end_increment, hp_increment, ppStrandMover);

   ATLASSERT(*ppStrandMover != NULL);
}

bool CBridgeAgentImp::LayoutGirders(const CBridgeDescription* pBridgeDesc)
{
   HRESULT hr = S_OK;

   GET_IFACE(ILibrary,pLib);
   
   CComPtr<ISpanCollection> spans;
   m_Bridge->get_Spans(&spans);

   SpanIndexType nSpans;
   spans->get_Count(&nSpans);

   ATLASSERT(nSpans == pBridgeDesc->GetSpanCount() );

   pgsTypes::SupportedDeckType deckType = pBridgeDesc->GetDeckDescription()->DeckType;
   pgsTypes::SupportedBeamSpacing girderSpacingType = pBridgeDesc->GetGirderSpacingType();

   Float64 gross_slab_depth = pBridgeDesc->GetDeckDescription()->GrossDepth;

   // if this deck has precast panels, GrossDepth is just the cast depth... add the panel depth
   if ( pBridgeDesc->GetDeckDescription()->DeckType == pgsTypes::sdtCompositeSIP )
      gross_slab_depth += pBridgeDesc->GetDeckDescription()->PanelDepth;

   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      const CSpanData* pSpan = pBridgeDesc->GetSpan(spanIdx);
      const CPierData* pPrevPier = pSpan->GetPrevPier();
      const CPierData* pNextPier = pSpan->GetNextPier();

      CComPtr<ISpan> span;
      spans->get_Item(spanIdx,&span);

      CComPtr<IPier> prevPier, nextPier;
      span->get_PrevPier(&prevPier);
      span->get_NextPier(&nextPier);

      CComPtr<IConnection> start_connection, end_connection;
      prevPier->get_Connection(qcbAfter, &start_connection);
      nextPier->get_Connection(qcbBefore,&end_connection);

      const ConnectionLibraryEntry* start_connection_entry = pPrevPier->GetConnectionLibraryEntry(pgsTypes::Ahead);
      const ConnectionLibraryEntry* end_connection_entry   = pNextPier->GetConnectionLibraryEntry(pgsTypes::Back);
      ConfigureConnection(start_connection_entry,start_connection);
      ConfigureConnection(end_connection_entry, end_connection);

      CComPtr<IGirderSpacing> startSpacing, endSpacing;
      span->get_GirderSpacing(etStart, &startSpacing);
      span->get_GirderSpacing(etEnd,   &endSpacing);

      const CGirderSpacing* pStartSpacing = pSpan->GetGirderSpacing(pgsTypes::metStart);
      const CGirderSpacing* pEndSpacing   = pSpan->GetGirderSpacing(pgsTypes::metEnd);

      pgsTypes::MeasurementLocation mlStart = pStartSpacing->GetMeasurementLocation();
      pgsTypes::MeasurementType     mtStart = pStartSpacing->GetMeasurementType();

      GirderIndexType startRefGirderIdx = pStartSpacing->GetRefGirder();
      Float64 startRefGirderOffset = pStartSpacing->GetRefGirderOffset();
      pgsTypes::OffsetMeasurementType startRefGirderOffsetType = pStartSpacing->GetRefGirderOffsetType();

      pgsTypes::MeasurementLocation mlEnd = pEndSpacing->GetMeasurementLocation();
      pgsTypes::MeasurementType     mtEnd = pEndSpacing->GetMeasurementType();

      GirderIndexType endRefGirderIdx = pEndSpacing->GetRefGirder();
      Float64 endRefGirderOffset = pEndSpacing->GetRefGirderOffset();
      pgsTypes::OffsetMeasurementType endRefGirderOffsetType = pEndSpacing->GetRefGirderOffsetType();

      startSpacing->SetMeasurement((MeasurementLocation)mlStart,(MeasurementType)mtStart);
      startSpacing->SetRefGirder(startRefGirderIdx,startRefGirderOffset,(OffsetType)startRefGirderOffsetType);

      endSpacing->SetMeasurement((MeasurementLocation)mlEnd,(MeasurementType)mtEnd);
      endSpacing->SetRefGirder(endRefGirderIdx,endRefGirderOffset,(OffsetType)endRefGirderOffsetType);

      GirderIndexType nGirders = pSpan->GetGirderCount();

      span->put_GirderCount(nGirders);

      const CGirderTypes* pGirderTypes = pSpan->GetGirderTypes();

      // create a girder for each girder line... set the girder spacing

      // store the girder spacing in an array and set it all at once
      CComPtr<IDblArray> startSpaces, endSpaces;
      startSpaces.CoCreateInstance(CLSID_DblArray);
      endSpaces.CoCreateInstance(CLSID_DblArray);
      GirderIndexType gdrIdx = 0;
      for ( gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         // get the girder library entry
         std::_tstring strGirderName = pSpan->GetGirderTypes()->GetGirderName(gdrIdx);

         const GirderLibraryEntry* pGirderEntry = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);

         ATLASSERT( strGirderName == pGirderEntry->GetName() );

         GirderLibraryEntry::Dimensions dimensions = pGirderEntry->GetDimensions();

         CComPtr<IBeamFactory> beamFactory;
         pGirderEntry->GetBeamFactory(&beamFactory);

         // get the girder spacing
         SpacingIndexType spaceIdx = gdrIdx;
         if ( gdrIdx < nGirders-1 )
         {
            // set the spacing after the girder... skip if this is the last girder
            Float64 gdrStartSpacing = pStartSpacing->GetGirderSpacing(spaceIdx);
            Float64 gdrEndSpacing   = pEndSpacing->GetGirderSpacing(spaceIdx);

            if ( IsJointSpacing(girderSpacingType) )
            {
               // spacing is actually a joint width... convert to a CL-CL spacing
               Float64 leftWidth, rightWidth;

               Float64 minSpacing, maxSpacing;
               beamFactory->GetAllowableSpacingRange(dimensions,deckType,girderSpacingType,&minSpacing,&maxSpacing);
               leftWidth = minSpacing;

               const GirderLibraryEntry* pGirderEntry2 = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx+1);
               GirderLibraryEntry::Dimensions dimensions2 = pGirderEntry2->GetDimensions();

               CComPtr<IBeamFactory> beamFactory2;
               pGirderEntry2->GetBeamFactory(&beamFactory2);

               beamFactory2->GetAllowableSpacingRange(dimensions2,deckType,girderSpacingType,&minSpacing,&maxSpacing);
               rightWidth = minSpacing;

               gdrStartSpacing += (leftWidth+rightWidth)/2;
               gdrEndSpacing   += (leftWidth+rightWidth)/2;
            }

            startSpaces->Add(gdrStartSpacing);
            endSpaces->Add(gdrEndSpacing);
         }
                                                                 
         // get the superstructure member
         CComPtr<ISuperstructureMember> ssmbr;
         span->get_SuperstructureMember(gdrIdx,&ssmbr);

         // assign connection data to superstructure member
         CComQIPtr<IItemData> item_data(ssmbr);
         item_data->AddItemData(CComBSTR(STARTCONNECTION),start_connection);
         item_data->AddItemData(CComBSTR(ENDCONNECTION),  end_connection);

         // create the strand mover
         CComPtr<IStrandMover> strand_mover;
         CreateStrandMover(strGirderName.c_str(),&strand_mover);
         ATLASSERT(strand_mover != NULL);
         
         // assign a precast girder model to the superstructure member
         CComPtr<IPrecastGirder> girder;
         HRESULT hr = girder.CoCreateInstance(CLSID_PrecastGirder);
         if ( FAILED(hr) || girder == NULL )
            THROW_UNWIND(_T("Precast girder object not created"),-1);

         girder->Initialize(m_Bridge,strand_mover, spanIdx,gdrIdx);
         item_data->AddItemData(CComBSTR(_T("Precast Girder")),girder);

         // Create strand grid template. Doesn't fill strands, just makes holes
         CComPtr<IStrandGrid> strGrd[2], harpGrdEnd[2], harpGrdHP[2], tempGrd[2];
         girder->get_StraightStrandGrid(etStart,&strGrd[etStart]);
         girder->get_HarpedStrandGridEnd(etStart,&harpGrdEnd[etStart]);
         girder->get_HarpedStrandGridHP(etStart,&harpGrdHP[etStart]);
         girder->get_TemporaryStrandGrid(etStart,&tempGrd[etStart]);

         girder->get_StraightStrandGrid(etEnd,&strGrd[etEnd]);
         girder->get_HarpedStrandGridEnd(etEnd,&harpGrdEnd[etEnd]);
         girder->get_HarpedStrandGridHP(etEnd,&harpGrdHP[etEnd]);
         girder->get_TemporaryStrandGrid(etEnd,&tempGrd[etEnd]);

         pGirderEntry->ConfigureStraightStrandGrid(strGrd[etStart],strGrd[etEnd]);
         pGirderEntry->ConfigureHarpedStrandGrids(harpGrdEnd[etStart], harpGrdHP[etStart], harpGrdHP[etEnd], harpGrdEnd[etEnd]);
         pGirderEntry->ConfigureTemporaryStrandGrid(tempGrd[etStart],tempGrd[etEnd]);

         girder->put_AllowOddNumberOfHarpedStrands(pGirderEntry->OddNumberOfHarpedStrands() ? VARIANT_TRUE : VARIANT_FALSE);

         // Let beam factory build segments as necessary to represent the beam
         beamFactory->LayoutGirderLine(m_pBroker,m_StatusGroupID,spanIdx,gdrIdx,ssmbr);

#pragma Reminder("UPDATE: This is wrong for harping point measured as distance")

         // lay out the harping points
         GirderLibraryEntry::MeasurementLocation hpref = pGirderEntry->GetHarpingPointReference();
         GirderLibraryEntry::MeasurementType hpmeasure = pGirderEntry->GetHarpingPointMeasure();
         Float64 hpLoc = pGirderEntry->GetHarpingPointLocation();

         girder->put_UseMinHarpPointDistance( pGirderEntry->IsMinHarpingPointLocationUsed() ? VARIANT_TRUE : VARIANT_FALSE);
         girder->put_MinHarpPointDistance( pGirderEntry->GetMinHarpingPointLocation() );

         girder->SetHarpingPoints(hpLoc,hpLoc);
         girder->put_HarpingPointReference( HarpPointReference(hpref) );
         girder->put_HarpingPointMeasure( HarpPointMeasure(hpmeasure) );
      

         Float64 startHaunch = 0;
         Float64 endHaunch   = 0;
         if ( deckType != pgsTypes::sdtNone )
         {
            startHaunch = pGirderTypes->GetSlabOffset(gdrIdx,pgsTypes::metStart) - gross_slab_depth;
            endHaunch   = pGirderTypes->GetSlabOffset(gdrIdx,pgsTypes::metEnd)   - gross_slab_depth;
         }

         // if these fire, something is really messed up
         ATLASSERT( 0 <= startHaunch && startHaunch < 1e9 );
         ATLASSERT( 0 <= endHaunch   && endHaunch   < 1e9 );

         startSpacing->put_GirderHaunch(gdrIdx,startHaunch);
         endSpacing->put_GirderHaunch(gdrIdx,endHaunch);
      }

      // set the spacing for all girders in this span now
      startSpacing->put_Spacings(startSpaces);
      endSpacing->put_Spacings(endSpaces);
   }

   return true;
}

bool CBridgeAgentImp::LayoutSuperstructureMembers(const CBridgeDescription* pBridgeDesc)
{
   // now that all of the spaces are defined, set the girder line lengths
   // in the superstructure members

   CComPtr<ISpanCollection> spans;
   m_Bridge->get_Spans(&spans);

   SpanIndexType nSpans;
   spans->get_Count(&nSpans);

   ATLASSERT(nSpans == pBridgeDesc->GetSpanCount() );

   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      CComPtr<ISpan> span;
      spans->get_Item(spanIdx,&span);

      GirderIndexType nGirders;
      span->get_GirderCount(&nGirders);

      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         // get the superstructure member
         CComPtr<ISuperstructureMember> ssmbr;
         span->get_SuperstructureMember(gdrIdx,&ssmbr);

         // set the member length
         Float64 girder_line_length; // this is the pier to pier length
         span->get_GirderLineLength(gdrIdx,&girder_line_length);

         // make the member length equal to the girder length
         Float64 brgOffsetStart = GetGirderStartBearingOffset(spanIdx,gdrIdx);
         Float64 brgOffsetEnd   = GetGirderEndBearingOffset(spanIdx,gdrIdx);
         Float64 endDistStart   = GetGirderStartConnectionLength(spanIdx,gdrIdx);
         Float64 endDistEnd     = GetGirderEndConnectionLength(spanIdx,gdrIdx);
         Float64 gdrLength = girder_line_length - brgOffsetStart - brgOffsetEnd + endDistStart + endDistEnd;

         ssmbr->put_Length(gdrLength);

         // finally, layout the rebar for this girder
         LayoutGirderRebar(spanIdx,gdrIdx);
      }
   }

   return true;
}

bool CBridgeAgentImp::LayoutDeck(const CBridgeDescription* pBridgeDesc)
{
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();
   pgsTypes::SupportedDeckType deckType = pDeck->DeckType;

   CComPtr<IBridgeDeck> deck;

   if ( IsAdjustableWidthDeck(deckType) )
   {
      if ( pDeck->DeckEdgePoints.size() == 1 )
         LayoutSimpleDeck(pBridgeDesc,&deck);
      else
         LayoutFullDeck(pBridgeDesc,&deck);
   }
   else
   {
      LayoutNoDeck(pBridgeDesc,&deck);
   }

   if ( deck )
   {
      deck->put_ConstructionStage(GetStageName(pgsTypes::BridgeSite1));
      deck->put_CompositeStage(GetStageName(pgsTypes::BridgeSite2));

      CComPtr<IMaterial> deck_material;
      deck_material.CoCreateInstance(CLSID_Material);
      deck_material->put_E(GetEcSlab());
      deck_material->put_Density(GetStrDensitySlab());
 
      deck->put_Material(deck_material);
      m_Bridge->putref_Deck(deck);
   }


   return true;
}

void CBridgeAgentImp::NoDeckEdgePoint(SpanIndexType spanIdx,PierIndexType pierIdx,DirectionType side,IPoint2d** ppPoint)
{
   CComPtr<IPoint2d> point_on_girder;
   CComPtr<IPoint2d> point_on_edge;
   CComPtr<IGirderSection> girder_section;
   CComPtr<IDirection> normal;

   CComQIPtr<ILocate2> locate(m_CogoEngine);

   CComPtr<ICogoInfo> cogoInfo;
   m_Bridge->get_CogoInfo(&cogoInfo);
   
   CComPtr<ICogoModel> cogoModel;
   m_Bridge->get_CogoModel(&cogoModel);

   CComPtr<IPointCollection> points;
   cogoModel->get_Points(&points);

   GirderIndexType gdrIdx;
   if ( side == qcbLeft )
   {
      gdrIdx = 0;
   }
   else
   {
      CComPtr<ISpanCollection> spans;
      m_Bridge->get_Spans(&spans);
      
      CComPtr<ISpan> span;
      spans->get_Item(spanIdx,&span);

      span->get_GirderCount(&gdrIdx);
      gdrIdx--;
   }

   Float64 location;
   if ( spanIdx == pierIdx )
   {
      location = 0.0;
   }
   else
   {
      location = GetGirderLength(spanIdx,gdrIdx);
   }

   GetGirderSection(pgsPointOfInterest(spanIdx,gdrIdx,location),&girder_section);
   Float64 width;
   girder_section->get_TopWidth(&width);
   width /= 2;

   m_BridgeGeometryTool->GirderLineBearing(m_Bridge,spanIdx,gdrIdx,&normal);
   normal->IncrementBy(CComVariant(PI_OVER_2));

   PositionType position = (spanIdx == pierIdx ? qcbAfter : qcbBefore);
   CogoElementKey id;
   cogoInfo->get_PierGirderIntersectionPointID(pierIdx,gdrIdx,position,&id);
   HRESULT hr = points->get_Item(id,&point_on_girder);
   ATLASSERT(SUCCEEDED(hr));

   if ( side == qcbRight )
      width *= -1;

   hr = locate->ByDistDir(point_on_girder,width,CComVariant(normal),0.00,&point_on_edge);
   ATLASSERT(SUCCEEDED(hr));

   (*ppPoint) = point_on_edge;
   (*ppPoint)->AddRef();

   return;
}

bool CBridgeAgentImp::LayoutNoDeck(const CBridgeDescription* pBridgeDesc,IBridgeDeck** ppDeck)
{
      // There isn't an explicit deck in this case, so layout of the composite slab must be done
      // based on the girder geometry. Update the bridge model now so that the girder geometry is correct
      m_Bridge->UpdateBridgeModel();

   HRESULT hr = S_OK;
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();
   pgsTypes::SupportedDeckType deckType = pDeck->DeckType;

   ATLASSERT( deckType == pgsTypes::sdtCompositeOverlay || deckType == pgsTypes::sdtNone );

   CComPtr<IOverlaySlab> slab;
   slab.CoCreateInstance(CLSID_OverlaySlab);

   if ( deckType == pgsTypes::sdtCompositeOverlay )
   {
      slab->put_GrossDepth(pDeck->GrossDepth);
      if ( pDeck->WearingSurface == pgsTypes::wstSacrificialDepth || pDeck->WearingSurface == pgsTypes::wstFutureOverlay)
      {
         slab->put_SacrificialDepth(pDeck->SacrificialDepth);
      }
      else
      {
         slab->put_SacrificialDepth(0);
      }
   }
   else
   {
      // use a dummy zero-depth deck because there is no deck
      slab->put_GrossDepth(0);
      slab->put_SacrificialDepth(0);
   }

   // create deck edge strategy
   CComPtr<IPath> left_path, right_path;
   left_path.CoCreateInstance(CLSID_Path);
   right_path.CoCreateInstance(CLSID_Path);

   // Build  path along exterior girders
   CComPtr<ISpanCollection> spans;
   m_Bridge->get_Spans(&spans);
   SpanIndexType nSpans;
   spans->get_Count(&nSpans);

   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      CComPtr<IPoint2d> point_on_edge;

      // left edge - start of span
      PierIndexType pierIdx = spanIdx;
      NoDeckEdgePoint(spanIdx,pierIdx,qcbLeft,&point_on_edge);
      left_path->AddEx(point_on_edge);

      // left edge - end of span
      point_on_edge.Release();
      NoDeckEdgePoint(spanIdx,pierIdx+1,qcbLeft,&point_on_edge);
      left_path->AddEx(point_on_edge);

      // right edge - start of span
      point_on_edge.Release();
      NoDeckEdgePoint(spanIdx,pierIdx,qcbRight,&point_on_edge);
      right_path->AddEx(point_on_edge);

      // right edge - end of span
      point_on_edge.Release();
      NoDeckEdgePoint(spanIdx,pierIdx+1,qcbRight,&point_on_edge);
      right_path->AddEx(point_on_edge);
   }

   CComPtr<IEdgePathStrategy> left_path_strategy,right_path_strategy;
   left_path_strategy.CoCreateInstance(CLSID_EdgePathStrategy);
   left_path_strategy->putref_Path(left_path);

   right_path_strategy.CoCreateInstance(CLSID_EdgePathStrategy);
   right_path_strategy->putref_Path(right_path);

   CComQIPtr<IOverhangPathStrategy> left_strategy(left_path_strategy);
   CComQIPtr<IOverhangPathStrategy> right_strategy(right_path_strategy);

   slab->putref_LeftOverhangPathStrategy(left_strategy);
   slab->putref_RightOverhangPathStrategy(right_strategy);

   slab.QueryInterface(ppDeck);

   (*ppDeck)->put_Composite( deckType == pgsTypes::sdtCompositeOverlay ? VARIANT_TRUE : VARIANT_FALSE );

   CComPtr<ICogoModel> cogo_model;
   m_Bridge->get_CogoModel(&cogo_model);
   CComPtr<IPathCollection> paths;
   cogo_model->get_Paths(&paths);
   paths->AddEx(0,left_path);
   paths->AddEx(1,right_path);

   return true;
}

bool CBridgeAgentImp::LayoutSimpleDeck(const CBridgeDescription* pBridgeDesc,IBridgeDeck** ppDeck)
{
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();
   pgsTypes::SupportedDeckType deckType = pDeck->DeckType;

   ATLASSERT( deckType == pgsTypes::sdtCompositeSIP || deckType == pgsTypes::sdtCompositeCIP );
   ATLASSERT( pDeck->DeckEdgePoints.size() == 1 );

   // use alignment offset of first pier when alignment offset is used to
   // establish the centerline of the bridge
   Float64 alignment_offset = pBridgeDesc->GetAlignmentOffset();

   CComPtr<IPath> left_path, right_path;

   CComQIPtr<IOverhangPathStrategy> left_overhang_strategy;
   CComQIPtr<IOverhangPathStrategy> right_overhang_strategy;

   // this is a simple deck edge that parallels the alignment
   CDeckPoint deckPoint = *(pDeck->DeckEdgePoints.begin());

   Float64 left_offset;
   Float64 right_offset;

   if ( deckPoint.MeasurementType == pgsTypes::omtAlignment )
   {
      // deck edge is measured from the alignment
      left_offset  = -deckPoint.LeftEdge;
      right_offset =  deckPoint.RightEdge;
   }
   else
   {
      // deck edge is measured from the CL bridge. compute the offsets from the alignment
      left_offset  = alignment_offset - deckPoint.LeftEdge;
      right_offset = alignment_offset + deckPoint.RightEdge;
   }

   // use the alignment offset strategy
   CComPtr<IAlignment> alignment;
   m_Bridge->get_Alignment(&alignment);

   CComPtr<IAlignmentOffsetStrategy> left_path_strategy,right_path_strategy;
   left_path_strategy.CoCreateInstance(CLSID_AlignmentOffsetStrategy);
   left_path_strategy->putref_Alignment(alignment);
   left_path_strategy->put_Offset( left_offset );

   right_path_strategy.CoCreateInstance(CLSID_AlignmentOffsetStrategy);
   right_path_strategy->putref_Alignment(alignment);
   right_path_strategy->put_Offset( right_offset );

   left_path_strategy.QueryInterface(&left_overhang_strategy);
   right_path_strategy.QueryInterface(&right_overhang_strategy);

   left_overhang_strategy->get_Path(&left_path);
   right_overhang_strategy->get_Path(&right_path);

   CComPtr<ICogoModel> cogo_model;
   m_Bridge->get_CogoModel(&cogo_model);
   CComPtr<IPathCollection> paths;
   cogo_model->get_Paths(&paths);
   paths->AddEx(0,left_path);
   paths->AddEx(1,right_path);

   if ( deckType == pgsTypes::sdtCompositeCIP )
      LayoutCompositeCIPDeck(pDeck,left_overhang_strategy,right_overhang_strategy,ppDeck);
   else if ( deckType == pgsTypes::sdtCompositeSIP )
      LayoutCompositeSIPDeck(pDeck,left_overhang_strategy,right_overhang_strategy,ppDeck);
   else
      ATLASSERT(false); // shouldn't get here

   return true;
}

bool CBridgeAgentImp::LayoutFullDeck(const CBridgeDescription* pBridgeDesc,IBridgeDeck** ppDeck)
{
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();
   pgsTypes::SupportedDeckType deckType = pDeck->DeckType;

   ATLASSERT( deckType == pgsTypes::sdtCompositeSIP || deckType == pgsTypes::sdtCompositeCIP );
   ATLASSERT( 1 < pDeck->DeckEdgePoints.size() );

   // the deck edge is described by a series of points. the transitions
   // between the points can be parallel to alignment, linear or spline. 
   // for spline transitions a cubic spline will be used. 
   // for parallel transitations, a parallel alignment sub-path will be used.
   CComPtr<IAlignment> alignment;
   m_Bridge->get_Alignment(&alignment);

   // create the edge strategies. use a strategy that consists of a simple cogo path object
   CComPtr<IEdgePathStrategy> left_path_strategy;
   left_path_strategy.CoCreateInstance(CLSID_EdgePathStrategy);

   CComPtr<IEdgePathStrategy> right_path_strategy;
   right_path_strategy.CoCreateInstance(CLSID_EdgePathStrategy);

   // create the path objects
   CComPtr<IPath> left_path, right_path;
   CDeckEdgeBuilder deck_edge_builder;
   deck_edge_builder.BuildDeckEdges(pBridgeDesc,m_CogoEngine,alignment,&left_path,&right_path);

   // associate the paths with the edge strategies
   left_path_strategy->putref_Path(left_path);
   right_path_strategy->putref_Path(right_path);

   CComQIPtr<IOverhangPathStrategy> left_overhang_strategy;
   CComQIPtr<IOverhangPathStrategy> right_overhang_strategy;
   left_path_strategy.QueryInterface(&left_overhang_strategy);
   right_path_strategy.QueryInterface(&right_overhang_strategy);

   // store the deck edge paths in the cogo model
   CComPtr<ICogoModel> cogo_model;
   m_Bridge->get_CogoModel(&cogo_model);
   CComPtr<IPathCollection> paths;
   cogo_model->get_Paths(&paths);
   paths->AddEx(0,left_path);
   paths->AddEx(1,right_path);

   if ( deckType == pgsTypes::sdtCompositeCIP )
      LayoutCompositeCIPDeck(pDeck,left_overhang_strategy,right_overhang_strategy,ppDeck);
   else if ( deckType == pgsTypes::sdtCompositeSIP )
      LayoutCompositeSIPDeck(pDeck,left_overhang_strategy,right_overhang_strategy,ppDeck);
   else
      ATLASSERT(false); // shouldn't get here

   return true;
}

bool CBridgeAgentImp::LayoutCompositeCIPDeck(const CDeckDescription* pDeck,IOverhangPathStrategy* pLeftOverhangStrategy,IOverhangPathStrategy* pRightOverhangStrategy,IBridgeDeck** ppDeck)
{
   CComPtr<ICastSlab> slab;
   slab.CoCreateInstance(CLSID_CastSlab);

   slab->put_Fillet(pDeck->Fillet);
   slab->put_GrossDepth(pDeck->GrossDepth);

   if ( pDeck->WearingSurface == pgsTypes::wstSacrificialDepth || pDeck->WearingSurface == pgsTypes::wstFutureOverlay)
   {
      slab->put_SacrificialDepth(pDeck->SacrificialDepth);
   }
   else
   {
      slab->put_SacrificialDepth(0);
   }

   slab->putref_LeftOverhangPathStrategy(pLeftOverhangStrategy);
   slab->putref_RightOverhangPathStrategy(pRightOverhangStrategy);

   slab->put_OverhangDepth(pDeck->OverhangEdgeDepth);
   slab->put_OverhangTaper((DeckOverhangTaper)pDeck->OverhangTaper);


   slab.QueryInterface(ppDeck);

   (*ppDeck)->put_Composite( VARIANT_TRUE );
   return true;
}

bool CBridgeAgentImp::LayoutCompositeSIPDeck(const CDeckDescription* pDeck,IOverhangPathStrategy* pLeftOverhangStrategy,IOverhangPathStrategy* pRightOverhangStrategy,IBridgeDeck** ppDeck)
{
   CComPtr<IPrecastSlab> slab;
   slab.CoCreateInstance(CLSID_PrecastSlab);

   slab->put_Fillet(pDeck->Fillet);
   slab->put_PanelDepth(pDeck->PanelDepth);
   slab->put_CastDepth(pDeck->GrossDepth); // interpreted as cast depth
   slab->put_OverhangDepth(pDeck->OverhangEdgeDepth);
   slab->put_OverhangTaper((DeckOverhangTaper)pDeck->OverhangTaper);

   if ( pDeck->WearingSurface == pgsTypes::wstSacrificialDepth || pDeck->WearingSurface == pgsTypes::wstFutureOverlay)
   {
      slab->put_SacrificialDepth(pDeck->SacrificialDepth);
   }
   else
   {
      slab->put_SacrificialDepth(0);
   }

   slab->putref_LeftOverhangPathStrategy(pLeftOverhangStrategy);
   slab->putref_RightOverhangPathStrategy(pRightOverhangStrategy);

   slab.QueryInterface(ppDeck);

   (*ppDeck)->put_Composite( VARIANT_TRUE );
   return true;
}

bool CBridgeAgentImp::LayoutTrafficBarriers(const CBridgeDescription* pBridgeDesc)
{
   CComPtr<ISidewalkBarrier> lb;
   const CRailingSystem* pLeftRailingSystem  = pBridgeDesc->GetLeftRailingSystem();
   LayoutTrafficBarrier(pBridgeDesc,pLeftRailingSystem,pgsTypes::tboLeft,&lb);
   m_Bridge->putref_LeftBarrier(lb);

   CComPtr<ISidewalkBarrier> rb;
   const CRailingSystem* pRightRailingSystem = pBridgeDesc->GetRightRailingSystem();
   LayoutTrafficBarrier(pBridgeDesc,pRightRailingSystem,pgsTypes::tboRight,&rb);
   m_Bridge->putref_RightBarrier(rb);

   return true;
}

void ComputeBarrierShapeToeLocations(IShape* pShape, Float64* leftToe, Float64* rightToe)
{
   // barrier toe is at X=0.0 of shape. Clip a very shallow rect at this elevation and use the width
   // of the clipped shape as the toe bounds.
   CComPtr<IRect2d> rect;
   rect.CoCreateInstance(CLSID_Rect2d);
   rect->SetBounds(-1.0e06, 1.0e06, 0.0, 1.0e-04);

   CComPtr<IShape> clip_shape;
   pShape->ClipIn(rect, &clip_shape);
   if (clip_shape)
   {
      CComPtr<IRect2d> bbox;
      clip_shape->get_BoundingBox(&bbox);
      bbox->get_Left(leftToe);
      bbox->get_Right(rightToe);
   }
   else
   {
      *leftToe = 0.0;
      *rightToe = 0.0;
   }
}

void CBridgeAgentImp::CreateBarrierObject(IBarrier** pBarrier, const TrafficBarrierEntry*  pBarrierEntry, pgsTypes::TrafficBarrierOrientation orientation)
{
   CComPtr<IPolyShape> polyshape;
   pBarrierEntry->CreatePolyShape(orientation,&polyshape);

   CComQIPtr<IShape> pShape(polyshape);

   // box containing barrier
   CComPtr<IRect2d> bbox;
   pShape->get_BoundingBox(&bbox);

   Float64 rightEdge, leftEdge;
   bbox->get_Right(&rightEdge);
   bbox->get_Left(&leftEdge);

   // Distance from the barrier curb to the exterior face of barrier for purposes of determining
   // roadway width
   Float64 curbOffset = pBarrierEntry->GetCurbOffset();

   // Toe locations of barrier
   Float64 leftToe, rightToe;
   ComputeBarrierShapeToeLocations(pShape, &leftToe, &rightToe);

   // Dimensions of barrier depend on orientation
   Float64 extToeWid, intToeWid;
   if (orientation == pgsTypes::tboLeft )
   {
      intToeWid = rightEdge - rightToe;
      extToeWid = -(leftEdge - leftToe);
   }
   else
   {
      extToeWid = rightEdge - rightToe;
      intToeWid = -(leftEdge - leftToe);
   }

   // We have all data. Create barrier and initialize
   CComPtr<IGenericBarrier> barrier;
   barrier.CoCreateInstance(CLSID_GenericBarrier);

   barrier->Init(pShape, curbOffset, intToeWid, extToeWid);

   barrier.QueryInterface(pBarrier);

   // set up material
   Float64 E, density;
   E = GetEcRailing(orientation);
   density = GetDensityRailing(orientation);

   CComPtr<IMaterial> material;
   (*pBarrier)->get_Material(&material);
   material->put_E(E);
   material->put_Density(density);
}

bool CBridgeAgentImp::LayoutTrafficBarrier(const CBridgeDescription* pBridgeDesc,const CRailingSystem* pRailingSystem,pgsTypes::TrafficBarrierOrientation orientation,ISidewalkBarrier** ppBarrier)
{
   // Railing system object
   CComPtr<ISidewalkBarrier> railing_system;
   railing_system.CoCreateInstance(CLSID_SidewalkBarrier);

   GET_IFACE(ILibrary,pLib);
   const TrafficBarrierEntry*  pExtRailingEntry = pLib->GetTrafficBarrierEntry( pRailingSystem->strExteriorRailing.c_str() );

   // Exterior Barrier
   CComPtr<IBarrier> extBarrier;
   CreateBarrierObject(&extBarrier, pExtRailingEntry, orientation);

   railing_system->put_IsExteriorStructurallyContinuous(pExtRailingEntry->IsBarrierStructurallyContinuous() ? VARIANT_TRUE : VARIANT_FALSE);

   SidewalkPositionType swPosition = pRailingSystem->bBarriersOnTopOfSidewalk ? swpBeneathBarriers : swpBetweenBarriers;

   int barrierType = 1;
   Float64 h1,h2,w;
   CComPtr<IBarrier> intBarrier;
   if ( pRailingSystem->bUseSidewalk )
   {
      // get the sidewalk dimensions
      barrierType = 2;
      h1 = pRailingSystem->LeftDepth;
      h2 = pRailingSystem->RightDepth;
      w  = pRailingSystem->Width;

      railing_system->put_IsSidewalkStructurallyContinuous(pRailingSystem->bSidewalkStructurallyContinuous ? VARIANT_TRUE : VARIANT_FALSE);

      if ( pRailingSystem->bUseInteriorRailing )
      {
         // there is an interior railing as well
         barrierType = 3;
         const TrafficBarrierEntry* pIntRailingEntry = pLib->GetTrafficBarrierEntry( pRailingSystem->strInteriorRailing.c_str() );

         CreateBarrierObject(&intBarrier, pIntRailingEntry, orientation);

         railing_system->put_IsInteriorStructurallyContinuous(pIntRailingEntry->IsBarrierStructurallyContinuous() ? VARIANT_TRUE : VARIANT_FALSE);
      }
   }

   switch(barrierType)
   {
   case 1:
      railing_system->put_Barrier1(extBarrier,(TrafficBarrierOrientation)orientation);
      break;

   case 2:
      railing_system->put_Barrier2(extBarrier,h1,h2,w,(TrafficBarrierOrientation)orientation, swPosition);
      break;

   case 3:
      railing_system->put_Barrier3(extBarrier,h1,h2,w,(TrafficBarrierOrientation)orientation, swPosition, intBarrier);
      break;

   default:
      ATLASSERT(FALSE); // should never get here
   }


   (*ppBarrier) = railing_system;
   (*ppBarrier)->AddRef();

   return true;
}

bool CBridgeAgentImp::BuildGirder()
{
   UpdatePrestressing(ALL_SPANS,ALL_GIRDERS);
   return true;
}

void CBridgeAgentImp::ValidateGirder()
{
   GET_IFACE(IEAFStatusCenter,pStatusCenter);

   // some data validation checking
   SpanIndexType cSpans = GetSpanCount();
   for (SpanIndexType span = 0; span < cSpans; span++ )
   {
      GirderIndexType cGirders = GetGirderCount( span );
      for ( GirderIndexType gdr = 0; gdr < cGirders; gdr++ )
      {
         Float64 length = GetGirderLength(span, gdr);
         Float64 effStrands;
         Float64 end_ecc = GetHsEccentricity(pgsPointOfInterest(span, gdr, 0.0), &effStrands);
         Float64 hp_ecc  = GetHsEccentricity(pgsPointOfInterest(span, gdr,length/2.0), &effStrands);

         if ((hp_ecc+TOLERANCE) < end_ecc)
         {
            LPCTSTR msg = _T("Harped strand eccentricity at girder ends is larger than at harping points. Drape is upside down");
            pgsGirderDescriptionStatusItem* pStatusItem = new pgsGirderDescriptionStatusItem(span,gdr,0,m_StatusGroupID,m_scidGirderDescriptionWarning,msg);
            pStatusCenter->Add(pStatusItem);
         }
      }
   }
}

void CBridgeAgentImp::UpdatePrestressing(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   GET_IFACE(IGirderData, pGirderData);
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   SpanIndexType cSpans = (spanIdx == ALL_SPANS ? GetSpanCount() : spanIdx+1);
   SpanIndexType span   = (spanIdx == ALL_SPANS ? 0              : spanIdx);
   for ( ; span < cSpans; span++ )
   {
      GirderIndexType cGirders = (gdrIdx == ALL_GIRDERS ? GetGirderCount( span ) : gdrIdx+1);
      GirderIndexType gdr      = (gdrIdx == ALL_GIRDERS ? 0                      : gdrIdx);
      for ( ; gdr < cGirders; gdr++ )
      {
         const CSpanData* pSpan = pBridgeDesc->GetSpan(span);
         std::_tstring strGirderName = pSpan->GetGirderTypes()->GetGirderName(gdr);

         const GirderLibraryEntry* pGirderEntry = pLib->GetGirderEntry( strGirderName.c_str() );

         // Inititalize a strand filler for each girder
         this->InitializeStrandFiller(pGirderEntry, span, gdr);

         CComPtr<IPrecastGirder> girder;
         GetGirder(span,gdr,&girder);

         const CGirderData* pgirderData = pGirderData->GetGirderData(span,gdr);

         girder->ClearStraightStrandDebonding();

         // Fill strands
         int permFillType = pgirderData->PrestressData.GetNumPermStrandsType();
         if (permFillType==NPS_TOTAL_NUMBER || permFillType==NPS_STRAIGHT_HARPED)
         {
            // Continuous fill
            CContinuousStrandFiller* pfiller = this->GetContinuousStrandFiller(span, gdr);

            HRESULT hr;
            hr = pfiller->SetStraightContinuousFill(girder, pgirderData->PrestressData.GetNstrands(pgsTypes::Straight));
            ATLASSERT( SUCCEEDED(hr));
            hr = pfiller->SetHarpedContinuousFill(girder, pgirderData->PrestressData.GetNstrands(pgsTypes::Harped));
            ATLASSERT( SUCCEEDED(hr));
            hr = pfiller->SetTemporaryContinuousFill(girder, pgirderData->PrestressData.GetNstrands(pgsTypes::Temporary));
            ATLASSERT( SUCCEEDED(hr));
         }
         else if (permFillType==NPS_DIRECT_SELECTION)
         {
            // Direct fill
            CDirectStrandFiller* pfiller = this->GetDirectStrandFiller(span, gdr);

            HRESULT hr;
            hr = pfiller->SetStraightDirectStrandFill(girder, pgirderData->PrestressData.GetDirectStrandFillStraight());
            ATLASSERT( SUCCEEDED(hr));
            hr = pfiller->SetHarpedDirectStrandFill(girder, pgirderData->PrestressData.GetDirectStrandFillHarped());
            ATLASSERT( SUCCEEDED(hr));
            hr = pfiller->SetTemporaryDirectStrandFill(girder, pgirderData->PrestressData.GetDirectStrandFillTemporary());
            ATLASSERT( SUCCEEDED(hr));
         }

         // Apply harped strand pattern offsets.
         // Get fill array for harped and convert to ConfigStrandFillVector
         CComPtr<IIndexArray> hFillArray;
         girder->get_HarpedStrandFill(&hFillArray);
         ConfigStrandFillVector hFillVec;
         IndexArray2ConfigStrandFillVec(hFillArray, hFillVec);

         bool force_straight = pGirderEntry->IsForceHarpedStrandsStraight();
         Float64 adjustment(0.0);
         if ( pGirderEntry->IsVerticalAdjustmentAllowedEnd() )
         {
            adjustment = this->ComputeAbsoluteHarpedOffsetEnd(span, gdr, hFillVec, pgirderData->PrestressData.HsoEndMeasurement, pgirderData->PrestressData.HpOffsetAtEnd);
            girder->put_HarpedStrandAdjustmentEnd(adjustment);

            // use same adjustment at harping points if harped strands are forced to straight
            if (force_straight)
            {
               girder->put_HarpedStrandAdjustmentHP(adjustment);
            }
         }

         if ( pGirderEntry->IsVerticalAdjustmentAllowedHP() && !force_straight )
         {
            adjustment = this->ComputeAbsoluteHarpedOffsetHp(span, gdr, hFillVec, 
                                                             pgirderData->PrestressData.HsoHpMeasurement, pgirderData->PrestressData.HpOffsetAtHp);
            girder->put_HarpedStrandAdjustmentHP(adjustment);

            ATLASSERT(!pGirderEntry->IsForceHarpedStrandsStraight()); // should not happen
         }

         // Apply debonding
         std::vector<CDebondInfo>::const_iterator iter;
         for ( iter = pgirderData->PrestressData.Debond[pgsTypes::Straight].begin(); 
               iter != pgirderData->PrestressData.Debond[pgsTypes::Straight].end(); iter++ )
         {
            const CDebondInfo& debond_info = *iter;

            // debond data index is in same order as grid fill
            girder->DebondStraightStrandByGridIndex(debond_info.strandTypeGridIdx,debond_info.Length1,debond_info.Length2);
         }
      
         // lay out POIs based on this prestressing
         LayoutPrestressTransferAndDebondPoi(span,gdr); 
//#pragma Reminder("############# - Not modeling harped or temporary strand debonding - #################")
      }
   }
}

void CBridgeAgentImp::ConfigureConnection( const ConnectionLibraryEntry* pEntry, IConnection* connection )
{
   connection->put_BearingOffset(pEntry->GetGirderBearingOffset());
   connection->put_EndDistance(pEntry->GetGirderEndDistance());
   connection->put_SupportWidth(pEntry->GetSupportWidth());

   ConnectionLibraryEntry::BearingOffsetMeasurementType mtBearingOffset = pEntry->GetBearingOffsetMeasurementType();
   ConnectionLibraryEntry::EndDistanceMeasurementType mtEndDistance   = pEntry->GetEndDistanceMeasurementType();
   connection->put_BearingOffsetMeasurementType(mtBearingOffset == ConnectionLibraryEntry::NormalToPier ? mtNormal : mtAlongItem);
   connection->put_EndDistanceMeasurementType((mtEndDistance==ConnectionLibraryEntry::FromBearingNormalToPier || mtEndDistance==ConnectionLibraryEntry::FromPierNormalToPier) ? mtNormal : mtAlongItem);
   connection->put_EndDistanceMeasurementLocation((mtEndDistance==ConnectionLibraryEntry::FromBearingNormalToPier || mtEndDistance==ConnectionLibraryEntry::FromBearingAlongGirder) ? mlCenterlineBearing : mlCenterlinePier);
}

bool CBridgeAgentImp::AreGirderTopFlangesRoughened(SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE(IShear,pShear);
	CShearData shear_data = pShear->GetShearData(span,gdr);
   return shear_data.bIsRoughenedSurface;
}

void CBridgeAgentImp::LayoutPointsOfInterest(SpanIndexType span,GirderIndexType gdr)
{
   WATCH(_T("Validating Points Of Interest: Span ") << span << _T(" Girder ") << gdr);

   LayoutRegularPoi(span,gdr,10); // 10th points.
   LayoutSpecialPoi(span,gdr);


//#if defined ENABLE_LOGGING
//   // Dump the std::vector<pgsPointOfInterest>'s to the log file
//   LOG(_T("*** Casting Yard POIs"));
//   m_CastingYardPoiMgr.Dump( m_Log );
//   LOG(_T(""));
//   LOG(_T("*** Bridge Site POIs"));
//   m_BridgeSitePoiMgr.Dump( m_Log );
//   LOG(_T(""));
//   LOG(_T("*** Lifting POIs"));
//   m_LiftingPoiMgr.Dump( m_Log );
//   LOG(_T(""));
//   LOG(_T("*** Hauling POIs"));
//   m_HaulingPoiMgr.Dump( m_Log );
//#endif
}

void CBridgeAgentImp::LayoutRegularPoi(SpanIndexType span,GirderIndexType gdr,Uint16 nPnts)
{
   LayoutRegularPoiCastingYard( span, gdr, nPnts, POI_ALLACTIONS | POI_ALLOUTPUT, &m_PoiMgr );
   LayoutRegularPoiBridgeSite(  span, gdr, nPnts, POI_ALLACTIONS | POI_ALLOUTPUT, &m_PoiMgr );
}

void CBridgeAgentImp::LayoutRegularPoiCastingYard(SpanIndexType span,GirderIndexType gdr,Uint16 nPnts,PoiAttributeType attrib,pgsPoiMgr* pPoiMgr)
{
   const Float64 toler = +1.0e-6;

   // need positional overlay for POIs defined for casting yard analysis for
   // when things like losses and camber are computed.
   //
   // This is a list of all stages, except casting yards. These stages have an attribute of 0
   std::vector<pgsTypes::Stage> stages;
   stages.push_back(pgsTypes::Lifting);
   stages.push_back(pgsTypes::Hauling);
   stages.push_back(pgsTypes::GirderPlacement);
   stages.push_back(pgsTypes::TemporaryStrandRemoval);
   stages.push_back(pgsTypes::BridgeSite1);
   stages.push_back(pgsTypes::BridgeSite2);
   stages.push_back(pgsTypes::BridgeSite3);

   Float64 girder_length  = GetGirderLength(span,gdr);
   Float64 left_end_size  = GetGirderStartConnectionLength(span,gdr);
   Float64 right_end_size = GetGirderEndConnectionLength(span,gdr);

   for ( Uint16 i = 0; i < nPnts+1; i++ )
   {
      Float64 dist = girder_length * ((Float64)i / (Float64)nPnts);

      PoiAttributeType attribute = attrib;

      Uint16 tenthPoint = 0; // 0 = not a tenth point

      // Check if poi is on the girder, and force it to be if it is not. poimgr will deal with duplicates
      if (IsZero(dist,toler))
      {
         dist = 0.0;
      }
      else if (IsEqual(dist,girder_length,toler))
      {
         dist = girder_length;
      }

      if (0.0 <= dist && dist <= girder_length)
      {
         // Add a special attribute flag if this poi is at the mid-span
         if ( i == (nPnts-1)/2 + 1)
            attribute |= POI_MIDSPAN;

         // Add a special attribute flag if poi is at a tenth point
         Float64 val = Float64(i)/Float64(nPnts)+toler;
         Float64 modv = fmod(val, 0.1);
         if (IsZero(modv,2*toler) || modv == 0.1)
         {
            tenthPoint = Uint16(10.*Float64(i)/Float64(nPnts) + 1);
         }
      }
      else
      {
         // Poi is a misfit because bearing is off the girder - force it to be on the girder, and
         // do not make it a tenth point
         if (dist<0.0)
         {
            dist = 0.0;
         }
         else
         {
            ATLASSERT(dist>girder_length);
            dist = girder_length;
         }
      }

      pgsPointOfInterest poi(span,gdr,dist);
      if ( dist < left_end_size || girder_length-right_end_size < dist )
      {
         // POI is outside of the bearing locations so it is only a casting yard POI
         poi.AddStage(pgsTypes::CastingYard,attribute);
      }
      else
      {
         // POI is between bearing locations, it is a casting yard POI and a POI in all other stages
         // attributes only apply to casting yard stage
         poi.AddStage(pgsTypes::CastingYard,attribute);
         poi.AddStages(stages,attrib);
      }

      poi.MakeTenthPoint(pgsTypes::CastingYard,tenthPoint);
      pPoiMgr->AddPointOfInterest( poi );
   }

// Extra POIs for debond design
// I used this block of code to layout POIs on 3' increments so I could experiment with debond design by hand
// these POI gave the intermediate values I was looking for
//   if ( stage == pgsTypes::CastingYard )
//   {
//      Float64 debond_inc = ::ConvertToSysUnits(3.0,unitMeasure::Feet);
//      Float64 x = debond_inc;
//      while ( x <= span_length/2 )
//      {
//         pPoiMgr->AddPointOfInterest( pgsPointOfInterest(span,gdr,x,attrib) );
//         x += debond_inc;
//      }
//
//      x = span_length - debond_inc;
//      while ( span_length/2 < x )
//      {
//         pPoiMgr->AddPointOfInterest( pgsPointOfInterest(span,gdr,x,attrib) );
//         x -= debond_inc;
//      }
//   }
}


void CBridgeAgentImp::LayoutRegularPoiBridgeSite(SpanIndexType span,GirderIndexType gdr,Uint16 nPnts,PoiAttributeType attrib,pgsPoiMgr* pPoiMgr)
{
   std::vector<pgsTypes::Stage> stages;
   // need positional overlay for POIs defined for casting yard analysis for
   // when things like losses and camber are computed.
   //
   // This is a list of all stages, except casting yards. Casting Yard stage has attribute of 0
   stages.push_back(pgsTypes::GirderPlacement);
   stages.push_back(pgsTypes::TemporaryStrandRemoval);
   stages.push_back(pgsTypes::BridgeSite1);
   stages.push_back(pgsTypes::BridgeSite2);
   stages.push_back(pgsTypes::BridgeSite3);

   const Float64 toler = +1.0e-6;

   Float64 span_length = GetSpanLength(span,gdr);
   Float64 end_size    = GetGirderStartConnectionLength(span,gdr);

   for ( Uint16 i = 0; i < nPnts+1; i++ )
   {
      Float64 dist = span_length * ((Float64)i / (Float64)nPnts);
      dist += end_size;

      PoiAttributeType attribute = attrib;

      Uint16 tenthPoint = 0;

      // Check if poi is on the girder, and force it to be if it is not. poimgr will deal with duplicates
      if (IsZero(dist,toler))
      {
         dist = 0.0;
      }
      else if (IsEqual(dist,end_size+span_length,toler))
      {
         dist = end_size + span_length;
      }

      if (0.0 <= dist && dist <= end_size+span_length)
      {
         // Add a special attribute flag if this poi is at the mid-span
         if ( i == (nPnts-1)/2 + 1)
            attribute |= POI_MIDSPAN;

         // Add a special attribute flag if poi is at a tenth point
         Float64 val = Float64(i)/Float64(nPnts)+toler;
         Float64 modv = fmod(val, 0.1);
         if (IsZero(modv,2*toler) || modv==0.1)
         {
            tenthPoint = Uint16(10.*Float64(i)/Float64(nPnts) + 1);
         }
      }
      else
      {
         // Poi is a misfit because bearing is off the girder - force it to be on the girder, and
         // do not make it a tenth point
         if (dist<0.0)
         {
            dist = 0.0;
         }
         else
         {
            ATLASSERT(end_size+span_length < dist);
            dist = end_size+span_length;
         }
      }

      pgsPointOfInterest poi(stages,span,gdr,dist,attribute);
      poi.MakeTenthPoint(stages,tenthPoint);
      poi.AddStage(pgsTypes::CastingYard,attrib);
      poi.AddStage(pgsTypes::Lifting,attrib);
      poi.AddStage(pgsTypes::Hauling,attrib);
      pPoiMgr->AddPointOfInterest( poi );
   }
}

void CBridgeAgentImp::LayoutSpecialPoi(SpanIndexType span,GirderIndexType gdr)
{
   LayoutEndSizePoi(span,gdr);
   LayoutHarpingPointPoi(span,gdr);
   //LayoutPrestressTransferAndDebondPoi(span,gdr); // this is done when the prestressing is updated
   LayoutPoiForDiaphragmLoads(span,gdr);
   LayoutPoiForShear(span,gdr);
   LayoutPoiForBarCutoffs(span,gdr);
   LayoutPoiForHandling(span,gdr);     // lifting and hauling
   LayoutPoiForSectionChanges(span,gdr);
}

void CBridgeAgentImp::LayoutEndSizePoi(SpanIndexType span,GirderIndexType gdr)
{
   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   Float64 left_end, right_end, length;
   girder->get_LeftEndSize(&left_end);
   girder->get_RightEndSize(&right_end);
   girder->get_GirderLength(&length);

   std::vector<pgsTypes::Stage> stages;
   stages.push_back(pgsTypes::CastingYard);
   stages.push_back(pgsTypes::GirderPlacement);
   stages.push_back(pgsTypes::TemporaryStrandRemoval);
   stages.push_back(pgsTypes::BridgeSite1);
   stages.push_back(pgsTypes::BridgeSite2);
   stages.push_back(pgsTypes::BridgeSite3);

   pgsPointOfInterest leftPoi( stages, span, gdr, left_end,           POI_ALLACTIONS | POI_ALLOUTPUT);
   pgsPointOfInterest rightPoi(stages, span, gdr, length - right_end, POI_ALLACTIONS | POI_ALLOUTPUT);

   m_PoiMgr.AddPointOfInterest( leftPoi );
   m_PoiMgr.AddPointOfInterest( rightPoi );
}

void CBridgeAgentImp::LayoutHarpingPointPoi(SpanIndexType span,GirderIndexType gdr)
{
   Uint16 maxHarped = GetNumHarpPoints(span,gdr);

   // if there can't be any harped strands, then there is no need to use the harping point attribute
   if ( maxHarped == 0 )
      return; 

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   Float64 hp1, hp2;
   girder->GetHarpingPointLocations( &hp1, &hp2 );

   PoiAttributeType attrib = POI_ALLACTIONS | POI_ALLOUTPUT | POI_HARPINGPOINT;

   pgsPointOfInterest poiHP1(span,gdr,hp1);
   pgsPointOfInterest poiHP2(span,gdr,hp2);

   std::vector<pgsTypes::Stage> stages;
   stages.push_back(pgsTypes::CastingYard);
   stages.push_back(pgsTypes::Lifting);
   stages.push_back(pgsTypes::Hauling);

   // Add hp poi to stages that are at the bridge site if the harp points
   // are between the support locations
   Float64 left_end, right_end, length;
   girder->get_LeftEndSize(&left_end);
   girder->get_RightEndSize(&right_end);
   girder->get_GirderLength(&length);
   if ( left_end < hp1 && hp2 < (length-right_end) )
   {
      stages.push_back(pgsTypes::GirderPlacement);
      stages.push_back(pgsTypes::TemporaryStrandRemoval);
      stages.push_back(pgsTypes::BridgeSite1);
      stages.push_back(pgsTypes::BridgeSite2);
      stages.push_back(pgsTypes::BridgeSite3);
   }
   else
   {
      // harp points are outside of the support location
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      std::_tostringstream os;
      os << _T("The harping points for Girder ") << LABEL_GIRDER(gdr) << _T(" in Span ") << LABEL_SPAN(span) 
         << _T(" are located outside of the bearings. You can fix this by increasing the girder length, or")
         << _T(" by changing the harping point location in the girder library entry.")<<std::endl;

      pgsBridgeDescriptionStatusItem* pStatusItem = 
         new pgsBridgeDescriptionStatusItem(m_StatusGroupID,m_scidBridgeDescriptionError,0,os.str().c_str());

      pStatusCenter->Add(pStatusItem);

      os << _T("See Status Center for Details");
      THROW_UNWIND(os.str().c_str(),XREASON_NEGATIVE_GIRDER_LENGTH);
   }
   
   poiHP1.AddStages(stages,attrib);
   poiHP2.AddStages(stages,attrib);

   m_PoiMgr.AddPointOfInterest( poiHP1 );
   m_PoiMgr.AddPointOfInterest( poiHP2 );


   if ( poiHP1.HasStage(pgsTypes::BridgeSite3) )
   {
      // add POI on either side of the harping point. Because the vertical component of prestress, Vp, is zero
      // on one side of the harp point and Vp on the other side there is a jump in shear capacity. Add these
      // poi to pick up the jump.

      poiHP1.SetDistFromStart( poiHP1.GetDistFromStart() + 0.0015 );
      poiHP1.SetAttributes(stages, POI_SHEAR | POI_GRAPHICAL);

      poiHP2.SetDistFromStart( poiHP2.GetDistFromStart() - 0.0015 );
      poiHP2.SetAttributes(stages, POI_SHEAR | POI_GRAPHICAL);
      
      m_PoiMgr.AddPointOfInterest( poiHP1 );
      m_PoiMgr.AddPointOfInterest( poiHP2 );
   }
}

void CBridgeAgentImp::LayoutPrestressTransferAndDebondPoi(SpanIndexType span,GirderIndexType gdr)
{
   PoiAttributeType attrib_debond = POI_ALLACTIONS | POI_ALLOUTPUT | POI_DEBOND;
   PoiAttributeType attrib_xfer   = POI_ALLACTIONS | POI_ALLOUTPUT | POI_PSXFER;

   // remove any current ps-xfer and debond pois
   std::vector<pgsPointOfInterest> poi;
   m_PoiMgr.GetPointsOfInterest(span,gdr,pgsTypes::CastingYard,POI_DEBOND | POI_PSXFER,POIMGR_OR,&poi);
   std::vector<pgsPointOfInterest>::iterator iter;
   for ( iter = poi.begin(); iter != poi.end(); iter++ )
   {
      m_PoiMgr.RemovePointOfInterest(*iter);
   }

#if defined _DEBUG
   m_PoiMgr.GetPointsOfInterest(span,gdr,pgsTypes::CastingYard,POI_DEBOND | POI_PSXFER,POIMGR_OR,&poi);
   ATLASSERT(poi.size() == 0);
#endif

   // add in the pois
   GET_IFACE(IPrestressForce,pPrestress);
   Float64 xfer_length = pPrestress->GetXferLength(span,gdr,pgsTypes::Permanent);
   
   Float64 length = GetGirderLength(span,gdr);

   Float64 start_offset = GetGirderStartConnectionLength(span,gdr);
   Float64 end_offset   = GetGirderEndConnectionLength(span,gdr);

   Float64 d1 = xfer_length;
   Float64 d2 = length - xfer_length;

   pgsPointOfInterest poiXfer1(span,gdr,d1);
   pgsPointOfInterest poiXfer2(span,gdr,d2);

   poiXfer1.AddStage(pgsTypes::CastingYard,attrib_xfer);
   poiXfer1.AddStage(pgsTypes::Lifting,attrib_xfer);
   poiXfer1.AddStage(pgsTypes::Hauling,attrib_xfer);

   poiXfer2.AddStage(pgsTypes::CastingYard,attrib_xfer);
   poiXfer2.AddStage(pgsTypes::Lifting,attrib_xfer);
   poiXfer2.AddStage(pgsTypes::Hauling,attrib_xfer);

   std::vector<pgsTypes::Stage> stages;
   stages.push_back(pgsTypes::GirderPlacement);
   stages.push_back(pgsTypes::TemporaryStrandRemoval);
   stages.push_back(pgsTypes::BridgeSite1);
   stages.push_back(pgsTypes::BridgeSite2);
   stages.push_back(pgsTypes::BridgeSite3);

   // only add prestress transfer points to the bridge model if they occur betweens bearings
   if ( start_offset < xfer_length )
      poiXfer1.AddStages(stages,attrib_xfer);

   if ( end_offset < xfer_length )
      poiXfer2.AddStages(stages,attrib_xfer);

   m_PoiMgr.AddPointOfInterest( poiXfer1 );
   m_PoiMgr.AddPointOfInterest( poiXfer2 );

   ////////////////////////////////////////////////////////////////
   // debonded strands
   ////////////////////////////////////////////////////////////////

   GET_IFACE(IGirderData,pGirderData);
   const CGirderData* pgirderData = pGirderData->GetGirderData(span,gdr);

   for ( Uint16 i = 0; i < 3; i++ )
   {
      std::vector<CDebondInfo>::const_iterator iter;
      for ( iter = pgirderData->PrestressData.Debond[i].begin(); iter != pgirderData->PrestressData.Debond[i].end(); iter++ )
      {
         const CDebondInfo& debond_info = *iter;

         d1 = debond_info.Length1;
         d2 = d1 + xfer_length;

         // only add POI if debond and transfer point are on the girder
         if ( d1 < length && d2 < length )
         {
            pgsPointOfInterest poiDBD(span,gdr,d1);
            poiDBD.AddStage(pgsTypes::CastingYard,attrib_debond);
            poiDBD.AddStage(pgsTypes::Lifting,    attrib_debond);
            poiDBD.AddStage(pgsTypes::Hauling,    attrib_debond);

            pgsPointOfInterest poiXFR(span,gdr,d2);
            poiXFR.AddStage(pgsTypes::CastingYard,attrib_xfer);
            poiXFR.AddStage(pgsTypes::Lifting,    attrib_xfer);
            poiXFR.AddStage(pgsTypes::Hauling,    attrib_xfer);

            // only add POI if debond and transfer point are betwen bearing points
            if ( start_offset < d1 && start_offset < d2 )
            {
               poiDBD.AddStages(stages,attrib_debond);
               poiXFR.AddStages(stages,attrib_xfer);
            }

            m_PoiMgr.AddPointOfInterest( poiDBD );
            m_PoiMgr.AddPointOfInterest( poiXFR );
         }

         d1 = length - debond_info.Length2;
         d2 = d1 - xfer_length;
         // only add POI if debond and transfer point are on the girder
         if ( 0 < d1 && 0 < d2 )
         {
            pgsPointOfInterest poiDBD(span,gdr,d1);
            poiDBD.AddStage(pgsTypes::CastingYard,attrib_debond);
            poiDBD.AddStage(pgsTypes::Lifting,    attrib_debond);
            poiDBD.AddStage(pgsTypes::Hauling,    attrib_debond);

            pgsPointOfInterest poiXFR(span,gdr,d2);
            poiXFR.AddStage(pgsTypes::CastingYard,attrib_xfer);
            poiXFR.AddStage(pgsTypes::Lifting,    attrib_xfer);
            poiXFR.AddStage(pgsTypes::Hauling,    attrib_xfer);

            // only add POI if debond and transfer point are betwen bearing points
            if ( d1 < length-end_offset && d2 < length-end_offset )
            {
               poiDBD.AddStages(stages,attrib_debond);
               poiXFR.AddStages(stages,attrib_xfer);
            }

            m_PoiMgr.AddPointOfInterest( poiDBD );
            m_PoiMgr.AddPointOfInterest( poiXFR );
         }
      }
   }
}

void CBridgeAgentImp::LayoutPoiForDiaphragmLoads(SpanIndexType span,GirderIndexType gdr)
{
   // we want to capture "jumps" do to diaphragm loads in the graphical displays
   std::vector<pgsTypes::Stage> stages;
   stages.push_back(pgsTypes::CastingYard);
   stages.push_back(pgsTypes::Lifting);
   stages.push_back(pgsTypes::Hauling);
   stages.push_back(pgsTypes::GirderPlacement);
   stages.push_back(pgsTypes::TemporaryStrandRemoval);
   stages.push_back(pgsTypes::BridgeSite1);
   stages.push_back(pgsTypes::BridgeSite2);
   stages.push_back(pgsTypes::BridgeSite3);

   // layout for diaphragms that are built in the casting yard
   std::vector<IntermedateDiaphragm> diaphragms   = GetIntermediateDiaphragms(pgsTypes::CastingYard,span,gdr);
   std::vector<IntermedateDiaphragm>::iterator iter;
   for ( iter = diaphragms.begin(); iter != diaphragms.end(); iter++ )
   {
      IntermedateDiaphragm& diaphragm = *iter;

      pgsPointOfInterest poi( stages, span, gdr, diaphragm.Location, POI_FLEXURESTRESS | POI_TABULAR | POI_GRAPHICAL );
      m_PoiMgr.AddPointOfInterest( poi );
   }

   // get loads for bridge site and combine with casting yard
   std::vector<IntermedateDiaphragm> bsdiaphragms = GetIntermediateDiaphragms(pgsTypes::BridgeSite1,span,gdr);

   diaphragms.insert(diaphragms.end(),bsdiaphragms.begin(),bsdiaphragms.end());

   Float64 end_size    = GetGirderStartConnectionLength(span,gdr);
   Float64 span_length = GetSpanLength(span,gdr);

   // remove lifting and hauling stages for cast in place diaphragms
   // keep casting yard stage because we have to summ losses over casting yard stage
   stages.erase( std::find(stages.begin(),stages.end(), pgsTypes::Lifting) );
   stages.erase( std::find(stages.begin(),stages.end(), pgsTypes::Hauling) );

   for ( iter = diaphragms.begin(); iter != diaphragms.end(); iter++ )
   {
      IntermedateDiaphragm& diaphragm = *iter;

      Float64 location = ForceIntoRange(end_size,diaphragm.Location,end_size+span_length);
      pgsPointOfInterest poi( stages, span, gdr, location, POI_TABULAR | POI_GRAPHICAL );
      m_PoiMgr.AddPointOfInterest( poi );
   }
}

void CBridgeAgentImp::LayoutPoiForShear(SpanIndexType span,GirderIndexType gdr)
{
   // Need points of interest at h and 1.5h for shear reporting
   // Need NPOI points of interest between the ends of the girder and 2h
   //    that will be used for locating the critical section for shear
   // Need points of interest at the critical section for shear
   Float64 length = GetGirderLength(span,gdr);

   Float64 hgLeft, hgRight; // height of girder at left and right ends
   hgLeft  = GetHeight(pgsPointOfInterest(span,gdr,0.00));
   hgRight = GetHeight(pgsPointOfInterest(span,gdr,length));

   Float64 left_end_size  = GetGirderStartConnectionLength(span,gdr);
   Float64 right_end_size = GetGirderEndConnectionLength(span,gdr);

   // If "H" from the end of the girder is at the point of bearing
   // make sure there isn't any "noise" in the data
   if ( IsEqual(hgLeft,left_end_size) )
      hgLeft = left_end_size;

   if ( IsEqual(hgRight,right_end_size) )
      hgRight = right_end_size;

   Float64 left_support_width   = GetGirderStartSupportWidth(span,gdr);
   Float64 right_support_width  = GetGirderEndSupportWidth(span,gdr);

   //
   // Poi's at h from ends of girder
   //
   std::vector<pgsTypes::Stage> stages;
   stages.push_back(pgsTypes::GirderPlacement);
   stages.push_back(pgsTypes::TemporaryStrandRemoval);
   stages.push_back(pgsTypes::BridgeSite1);
   stages.push_back(pgsTypes::BridgeSite2);
   stages.push_back(pgsTypes::BridgeSite3);

   PoiAttributeType attribute = POI_FLEXURESTRESS | POI_SHEAR | POI_ALLOUTPUT;
   pgsPointOfInterest poi_hl_gdr( pgsTypes::CastingYard, span, gdr, hgLeft,        attribute | POI_H);
   pgsPointOfInterest poi_hr_gdr( pgsTypes::CastingYard, span, gdr, length-hgRight,attribute | POI_H);

   if ( left_end_size <= hgLeft )
      poi_hl_gdr.AddStages(stages,attribute);

   if ( right_end_size <= hgRight )
      poi_hr_gdr.AddStages(stages,attribute);

   m_PoiMgr.AddPointOfInterest(poi_hl_gdr);
   m_PoiMgr.AddPointOfInterest(poi_hr_gdr);

   // h is from face of support in bridge site stages.
   if ( left_end_size + left_support_width/2 <= hgLeft )
   {
      pgsPointOfInterest poi_hl_brg( stages, span, gdr, left_end_size + left_support_width/2 + hgLeft, attribute | POI_H);
      poi_hl_brg.AddStage(pgsTypes::CastingYard, attribute);
      m_PoiMgr.AddPointOfInterest(poi_hl_brg);
   }

   if ( right_end_size + right_support_width/2 <= hgRight )
   {
      pgsPointOfInterest poi_hr_brg( stages, span, gdr, length - right_end_size - right_support_width/2 - hgRight, attribute | POI_H);
      poi_hr_brg.AddStage(pgsTypes::CastingYard, attribute);
      m_PoiMgr.AddPointOfInterest(poi_hr_brg);
   }

   //
   // poi at face of support. For shear only.
   //
   pgsPointOfInterest poi_left_fos( stages,span,gdr,left_end_size + left_support_width/2,            POI_SHEAR | POI_ALLOUTPUT | POI_FACEOFSUPPORT);
   pgsPointOfInterest poi_right_fos(stages,span,gdr,length - right_end_size - right_support_width/2, POI_SHEAR | POI_ALLOUTPUT | POI_FACEOFSUPPORT);
   
   // need to have this in casting yard stage for negative moment capacity.
   // need to have camber at this point to build model... camber starts in casting yard
   poi_left_fos.AddStage(pgsTypes::CastingYard, POI_FLEXURECAPACITY | POI_ALLOUTPUT);
   poi_right_fos.AddStage(pgsTypes::CastingYard,POI_FLEXURECAPACITY | POI_ALLOUTPUT);

   m_PoiMgr.AddPointOfInterest(poi_left_fos);
   m_PoiMgr.AddPointOfInterest(poi_right_fos);

   //
   // poi at 1.5h from face of support. For shear only.
   //
   poi_left_fos.SetAttributes( stages, POI_SHEAR | POI_ALLOUTPUT | POI_15H);
   poi_left_fos.SetDistFromStart(left_end_size + left_support_width/2 + 1.5*hgLeft);
   m_PoiMgr.AddPointOfInterest(poi_left_fos);

   poi_right_fos.SetAttributes( stages, POI_SHEAR | POI_ALLOUTPUT | POI_15H );
   poi_right_fos.SetDistFromStart(length - right_end_size - right_support_width/2 - 1.5*hgRight);
   m_PoiMgr.AddPointOfInterest(poi_right_fos);


   //
   // NPOI std::vector<pgsPointOfInterest>'s between ends of girder and 1.5h
   // this is used to determine critical section for shear
   //
   const Int16 NPOI = 2;
   PoiAttributeType attrib = POI_FLEXURECAPACITY | POI_SHEAR | POI_GRAPHICAL;

   stages.push_back(pgsTypes::CastingYard);
   std::sort(stages.begin(),stages.end());
   for ( Int16 i = 0; i < NPOI; i++ )
   {
      Float64 x_left  = left_end_size + (i+1)*1.5*hgLeft/NPOI;
      Float64 x_right = length - right_end_size - (i+1)*1.5*hgRight/NPOI;

      pgsPointOfInterest poi_l(stages,span,gdr,x_left,attrib);
      pgsPointOfInterest poi_r(stages,span,gdr,x_right,attrib);
      m_PoiMgr.AddPointOfInterest(poi_l);
      m_PoiMgr.AddPointOfInterest(poi_r);
   }
}

void CBridgeAgentImp::LayoutPoiForBarCutoffs(SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckRebarData& rebarData = pBridgeDesc->GetDeckDescription()->DeckRebarData;

   PierIndexType prev_pier = (PierIndexType)span;
   PierIndexType next_pier = prev_pier + 1;

   Float64 gdr_length = GetGirderLength(span,gdr);

   Float64 left_end_size  = GetGirderStartConnectionLength(span,gdr);
   Float64 right_end_size = GetGirderEndConnectionLength(span,gdr);

   Float64 left_brg_offset  = GetGirderStartBearingOffset(span,gdr);
   Float64 right_brg_offset = GetGirderEndBearingOffset(span,gdr);

   std::vector<pgsTypes::Stage> stages;
   stages.push_back(pgsTypes::CastingYard); // need camber for neg moment capacity model (camber starts in casting yard)
   stages.push_back(pgsTypes::BridgeSite3);

   std::vector<CDeckRebarData::NegMomentRebarData>::const_iterator iter;
   for ( iter = rebarData.NegMomentRebar.begin(); iter != rebarData.NegMomentRebar.end(); iter++ )
   {
      const CDeckRebarData::NegMomentRebarData& nmRebarData = *iter;

      PoiAttributeType attribute = POI_FLEXURECAPACITY | POI_SHEAR | POI_GRAPHICAL | POI_TABULAR | POI_BARCUTOFF;

      if ( nmRebarData.PierIdx == prev_pier )
      {
         Float64 dist_from_start = nmRebarData.RightCutoff - left_brg_offset + left_end_size;
         if ( gdr_length < dist_from_start )
            dist_from_start = gdr_length - 0.0015;

         m_PoiMgr.AddPointOfInterest( pgsPointOfInterest(stages,span,gdr,dist_from_start,attribute) );
         m_PoiMgr.AddPointOfInterest( pgsPointOfInterest(stages,span,gdr,dist_from_start+0.0015,POI_FLEXURECAPACITY | POI_GRAPHICAL) );
      }
      else if ( nmRebarData.PierIdx == next_pier )
      {
         Float64 dist_from_start = gdr_length - (nmRebarData.LeftCutoff - right_brg_offset + right_end_size);
         if ( dist_from_start < 0 )
            dist_from_start = 0.0015;

         m_PoiMgr.AddPointOfInterest( pgsPointOfInterest(stages,span,gdr,dist_from_start-0.0015,POI_FLEXURECAPACITY | POI_GRAPHICAL) );
         m_PoiMgr.AddPointOfInterest( pgsPointOfInterest(stages,span,gdr,dist_from_start,attribute) );
      }
   }
}

void CBridgeAgentImp::LayoutPoiForHandling(SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE(IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   if (pGirderLiftingSpecCriteria->IsLiftingCheckEnabled())
   {
      LayoutLiftingPoi(span,gdr,10); // puts poi at 10th points
      UpdateLiftingPoi(span,gdr);    // updates poi at pick point location
   }

   GET_IFACE(IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   if ( pGirderHaulingSpecCriteria->IsHaulingCheckEnabled() )
   {
      LayoutHaulingPoi(span,gdr,10);
      UpdateHaulingPoi(span,gdr);
   }
}

void CBridgeAgentImp::LayoutPoiForSectionChanges(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(spanIdx);
   const GirderLibraryEntry* pGirderEntry = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);
   CComPtr<IBeamFactory> beamFactory;
   pGirderEntry->GetBeamFactory(&beamFactory);

   beamFactory->LayoutSectionChangePointsOfInterest(m_pBroker,spanIdx,gdrIdx,&m_PoiMgr);
}

void CBridgeAgentImp::UpdateLiftingPoi(SpanIndexType span,GirderIndexType gdr)
{
   Float64 Lg = GetGirderLength(span,gdr);

   Float64 leftPickPoint, rightPickPoint;

   GET_IFACE(IGirderLifting, pGirderLifting);

   leftPickPoint = pGirderLifting->GetLeftLiftingLoopLocation(span,gdr);
   rightPickPoint = Lg - pGirderLifting->GetRightLiftingLoopLocation(span,gdr);

   std::vector<pgsPointOfInterest> vPoi;
   m_PoiMgr.GetPointsOfInterest(span,gdr,pgsTypes::Lifting,POI_PICKPOINT,POIMGR_AND,&vPoi);

   std::vector<pgsPointOfInterest>::iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      pgsPointOfInterest& poi = *iter;
      PoiAttributeType attributes = poi.GetAttributes(pgsTypes::Lifting);
      if ( attributes == POI_PICKPOINT )
      {
         // if the only flag that is set is pick point, remove it
         m_PoiMgr.RemovePointOfInterest(*iter);
      }
      else
      {
         // otherwise, just clear the POI_PICKPOINT bit
         // note that we are operating on a copy of the poi, so replace the
         // actual poi in the poi manager with the modified poi we create here
         sysFlags<PoiAttributeType>::Clear(&attributes,POI_PICKPOINT);
         poi.SetAttributes(pgsTypes::Lifting,attributes);
         m_PoiMgr.ReplacePointOfInterest(poi.GetID(),poi);
      }
   }

#ifdef _DEBUG
   vPoi.clear();
   m_PoiMgr.GetPointsOfInterest(span,gdr,pgsTypes::Lifting,POI_PICKPOINT,POIMGR_AND,&vPoi);
   ATLASSERT(vPoi.size() == 0); // there should not be any pick point POI at this point
#endif

   pgsPointOfInterest left_pick_point(pgsTypes::Lifting,span,gdr,leftPickPoint,POI_PICKPOINT);
   pgsPointOfInterest right_pick_point(pgsTypes::Lifting,span,gdr,rightPickPoint,POI_PICKPOINT);

   m_PoiMgr.AddPointOfInterest(left_pick_point);
   m_PoiMgr.AddPointOfInterest(right_pick_point);
}

void CBridgeAgentImp::UpdateHaulingPoi(SpanIndexType span,GirderIndexType gdr)
{
   Float64 Lg = GetGirderLength(span,gdr);

   Float64 leftBunkPoint, rightBunkPoint;

   GET_IFACE(IGirderHauling, pGirderHauling);

   leftBunkPoint = pGirderHauling->GetTrailingOverhang(span,gdr);
   rightBunkPoint = Lg - pGirderHauling->GetLeadingOverhang(span,gdr);

   std::vector<pgsPointOfInterest> vPoi;
   m_PoiMgr.GetPointsOfInterest(span,gdr,pgsTypes::Hauling,POI_BUNKPOINT,POIMGR_AND,&vPoi);

   std::vector<pgsPointOfInterest>::iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      pgsPointOfInterest& poi = *iter;
      if ( poi.GetAttributes(pgsTypes::Hauling) == POI_BUNKPOINT )
      {
         // if the only flag that is set is bunk point, remove it
         m_PoiMgr.RemovePointOfInterest(*iter);
      }
      else
      {
         // otherwise, just clear the POI_BUNKPOINT bit
         PoiAttributeType attributes = poi.GetAttributes(pgsTypes::Hauling);
         sysFlags<PoiAttributeType>::Clear(&attributes,POI_BUNKPOINT);
         poi.SetAttributes(pgsTypes::Hauling,attributes);
         m_PoiMgr.ReplacePointOfInterest(poi.GetID(),poi);
      }
   }

#ifdef _DEBUG
   vPoi.clear();
   m_PoiMgr.GetPointsOfInterest(span,gdr,pgsTypes::Hauling,POI_BUNKPOINT,POIMGR_AND,&vPoi);
   ATLASSERT(vPoi.size() == 0); // there should not be any bunk point POI at this point
#endif

   pgsPointOfInterest left_bunk_point( pgsTypes::Hauling,span,gdr,leftBunkPoint, POI_BUNKPOINT);
   pgsPointOfInterest right_bunk_point(pgsTypes::Hauling,span,gdr,rightBunkPoint,POI_BUNKPOINT);

   m_PoiMgr.AddPointOfInterest(left_bunk_point);
   m_PoiMgr.AddPointOfInterest(right_bunk_point);
}

//-----------------------------------------------------------------------------
void CBridgeAgentImp::LayoutLiftingPoi(SpanIndexType span, 
                                       GirderIndexType gdr, 
                                       Uint16 nPnts)
{
   // lifting
   LayoutHandlingPoi(pgsTypes::Lifting,
                     span, 
                     gdr, 
                     nPnts, 
                     POI_FLEXURESTRESS | POI_ALLOUTPUT,
                     &m_PoiMgr); 
                     
}

//-----------------------------------------------------------------------------
void CBridgeAgentImp::LayoutHaulingPoi(SpanIndexType span, 
                                       GirderIndexType gdr, 
                                       Uint16 nPnts)
{
   // hauling
   LayoutHandlingPoi(pgsTypes::Hauling,
                     span, 
                     gdr, 
                     nPnts, 
                     POI_FLEXURESTRESS | POI_ALLOUTPUT,
                     &m_PoiMgr);
}

//-----------------------------------------------------------------------------
void CBridgeAgentImp::LayoutHandlingPoi(pgsTypes::Stage stage,
                                        SpanIndexType span, 
                                        GirderIndexType gdr, 
                                        Uint16 nPnts, 
                                        PoiAttributeType attrib,
                                        pgsPoiMgr* pPoiMgr)
{
   PoiAttributeType supportAttribute;
   Float64 leftOverhang, rightOverhang;
   if (stage == pgsTypes::Lifting)
   {
      GET_IFACE(IGirderLifting, pGirderLifting);
      leftOverhang  = pGirderLifting->GetLeftLiftingLoopLocation(span,gdr);
      rightOverhang = pGirderLifting->GetRightLiftingLoopLocation(span,gdr);

      supportAttribute = POI_PICKPOINT;
   }
   else if (stage == pgsTypes::Hauling)
   {
      GET_IFACE(IGirderHauling, pGirderHauling);
      leftOverhang  = pGirderHauling->GetTrailingOverhang(span,gdr);
      rightOverhang = pGirderHauling->GetLeadingOverhang(span,gdr);

      supportAttribute = POI_BUNKPOINT;
   }
   else
   {
      ATLASSERT(false); // you sent in the wrong stage type
   }

   LayoutHandlingPoi(stage,span,gdr,nPnts,leftOverhang,rightOverhang,attrib,supportAttribute,pPoiMgr);
}

//-----------------------------------------------------------------------------
void CBridgeAgentImp::LayoutHandlingPoi(pgsTypes::Stage stage,
                                        SpanIndexType span,
                                        GirderIndexType gdr, 
                                        Uint16 nPnts, 
                                        Float64 leftOverhang, 
                                        Float64 rightOverhang, 
                                        PoiAttributeType attrib, 
                                        PoiAttributeType supportAttribute,
                                        pgsPoiMgr* pPoiMgr)
{
   Float64 girder_length = GetGirderLength(span,gdr);
   Float64 span_length = girder_length - leftOverhang - rightOverhang;
   CHECK(span_length>0.0); // should be checked in input

   // add poi at support locations
   pPoiMgr->AddPointOfInterest( pgsPointOfInterest(stage,span,gdr,leftOverhang,attrib | supportAttribute) );
   pPoiMgr->AddPointOfInterest( pgsPointOfInterest(stage,span,gdr,girder_length - rightOverhang,attrib | supportAttribute) );

   // add poi at ends of girder
   pPoiMgr->AddPointOfInterest( pgsPointOfInterest(stage,span,gdr,0,attrib) );
   pPoiMgr->AddPointOfInterest( pgsPointOfInterest(stage,span,gdr,girder_length,attrib) );

   // nth point POI between overhang support points
   const Float64 toler = +1.0e-6;
   for ( Uint16 i = 0; i < nPnts+1; i++ )
   {
      Float64 dist = leftOverhang + span_length * ((Float64)i / (Float64)nPnts);

      PoiAttributeType attribute = attrib;

      // Add a special attribute flag if this poi is at the mid-span
      if ( i == (nPnts-1)/2 + 1)
         attribute |= POI_MIDSPAN;

      // Add a special attribute flag if poi is at a tenth point
      Uint16 tenthPoint = 0;
      Float64 val = Float64(i)/Float64(nPnts)+toler;
      Float64 modv = fmod(val, 0.1);
      if (IsZero(modv,2*toler) || modv==0.1)
      {
         tenthPoint = Uint16(10.*Float64(i)/Float64(nPnts) + 1);
      }
      
      pgsPointOfInterest poi(stage,span,gdr,dist,attribute);
      poi.MakeTenthPoint(stage,tenthPoint);
      pPoiMgr->AddPointOfInterest( poi );
   }

}

/////////////////////////////////////////////////////////////////////////
// IAgent
//
STDMETHODIMP CBridgeAgentImp::SetBroker(IBroker* pBroker)
{
   AGENT_SET_BROKER(pBroker);
   return S_OK;
}

STDMETHODIMP CBridgeAgentImp::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);

   pBrokerInit->RegInterface( IID_IRoadway,                       this );
   pBrokerInit->RegInterface( IID_IBridge,                        this );
   pBrokerInit->RegInterface( IID_IBridgeMaterial,                this );
   pBrokerInit->RegInterface( IID_IBridgeMaterialEx,              this );
   pBrokerInit->RegInterface( IID_IStrandGeometry,                this );
   pBrokerInit->RegInterface( IID_IStageMap,                      this );
   pBrokerInit->RegInterface( IID_ILongRebarGeometry,             this );
   pBrokerInit->RegInterface( IID_IStirrupGeometry,               this );
   pBrokerInit->RegInterface( IID_IPointOfInterest,               this );
   pBrokerInit->RegInterface( IID_ISectProp2,                     this );
   pBrokerInit->RegInterface( IID_IBarriers,                      this );
   pBrokerInit->RegInterface( IID_IGirder,                        this );
   pBrokerInit->RegInterface( IID_IGirderLiftingPointsOfInterest, this );
   pBrokerInit->RegInterface( IID_IGirderHaulingPointsOfInterest, this );
   pBrokerInit->RegInterface( IID_IUserDefinedLoads,              this );

   return S_OK;
}

STDMETHODIMP CBridgeAgentImp::Init()
{
   CREATE_LOGFILE("BridgeAgent");
   AGENT_INIT; // this macro defines pStatusCenter
   m_LoadStatusGroupID = pStatusCenter->CreateStatusGroupID();

   // Setup cogo model and roadway alignment
   // Alignment goes between two dummy cogo points

   //
   // Attach to connection points
   //
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   CComPtr<IConnectionPoint> pCP;
   HRESULT hr = S_OK;

   // Connection point for the bridge description
   hr = pBrokerInit->FindConnectionPoint( IID_IBridgeDescriptionEventSink, &pCP );
   CHECK( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwBridgeDescCookie );
   CHECK( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   // Connection point for the specifications
   hr = pBrokerInit->FindConnectionPoint( IID_ISpecificationEventSink, &pCP );
   CHECK( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwSpecificationCookie );
   CHECK( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = m_SectCutTool.CoCreateInstance(CLSID_SectionCutTool);
   if ( FAILED(hr) || m_SectCutTool == NULL )
      THROW_UNWIND(_T("GenericBridgeTools::SectionPropertyTool not created"),-1);

   // Register status callbacks that we want to use
   m_scidInformationalError       = pStatusCenter->RegisterCallback(new pgsInformationalStatusCallback(eafTypes::statusError)); 
   m_scidInformationalWarning     = pStatusCenter->RegisterCallback(new pgsInformationalStatusCallback(eafTypes::statusWarning)); 
   m_scidBridgeDescriptionError   = pStatusCenter->RegisterCallback(new pgsBridgeDescriptionStatusCallback(m_pBroker,eafTypes::statusError));
   m_scidBridgeDescriptionWarning = pStatusCenter->RegisterCallback(new pgsBridgeDescriptionStatusCallback(m_pBroker,eafTypes::statusWarning));
   m_scidAlignmentWarning         = pStatusCenter->RegisterCallback(new pgsAlignmentDescriptionStatusCallback(m_pBroker,eafTypes::statusWarning));
   m_scidAlignmentError           = pStatusCenter->RegisterCallback(new pgsAlignmentDescriptionStatusCallback(m_pBroker,eafTypes::statusError));
   m_scidGirderDescriptionWarning = pStatusCenter->RegisterCallback(new pgsGirderDescriptionStatusCallback(m_pBroker,eafTypes::statusWarning));
   m_scidConcreteStrengthWarning  = pStatusCenter->RegisterCallback(new pgsConcreteStrengthStatusCallback(m_pBroker,eafTypes::statusWarning));
   m_scidConcreteStrengthError    = pStatusCenter->RegisterCallback(new pgsConcreteStrengthStatusCallback(m_pBroker,eafTypes::statusError));
   m_scidPointLoadWarning         = pStatusCenter->RegisterCallback(new pgsPointLoadStatusCallback(m_pBroker,eafTypes::statusWarning));
   m_scidDistributedLoadWarning   = pStatusCenter->RegisterCallback(new pgsDistributedLoadStatusCallback(m_pBroker,eafTypes::statusWarning));
   m_scidMomentLoadWarning        = pStatusCenter->RegisterCallback(new pgsMomentLoadStatusCallback(m_pBroker,eafTypes::statusWarning));

   return S_OK;
}

STDMETHODIMP CBridgeAgentImp::Init2()
{
   return S_OK;
}

STDMETHODIMP CBridgeAgentImp::GetClassID(CLSID* pCLSID)
{
   *pCLSID = CLSID_BridgeAgent;
   return S_OK;
}

STDMETHODIMP CBridgeAgentImp::Reset()
{
   Invalidate( CLEAR_ALL );
   m_SectCutTool.Release();

   return S_OK;
}

STDMETHODIMP CBridgeAgentImp::ShutDown()
{
   //
   // Detach to connection points
   //
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   CComPtr<IConnectionPoint> pCP;
   HRESULT hr = S_OK;

   hr = pBrokerInit->FindConnectionPoint(IID_IBridgeDescriptionEventSink, &pCP );
   CHECK( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwBridgeDescCookie );
   CHECK( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_ISpecificationEventSink, &pCP );
   CHECK( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwSpecificationCookie );
   CHECK( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   CLOSE_LOGFILE;

   AGENT_CLEAR_INTERFACE_CACHE;

   return S_OK;
}

/////////////////////////////////////////////////////////////////////////
// IRoadway
//
Float64 CBridgeAgentImp::GetCrownPointOffset(Float64 station)
{
   VALIDATE( COGO_MODEL );

   CComPtr<IProfile> profile;
   GetProfile(&profile);

   Float64 offset;
   profile->CrownPointOffset(CComVariant(station),&offset);

   return offset;
}

Float64 CBridgeAgentImp::GetCrownSlope(Float64 station,Float64 offset)
{
   VALIDATE( COGO_MODEL );

   CComPtr<IProfile> profile;
   GetProfile(&profile);

   Float64 slope;
   profile->CrownSlope(CComVariant(station),offset,&slope);

   return slope;
}

void CBridgeAgentImp::GetCrownSlope(Float64 station,Float64* pLeftSlope,Float64* pRightSlope)
{
   VALIDATE( COGO_MODEL );

   CComPtr<IProfile> profile;
   GetProfile(&profile);

   profile->LeftCrownSlope( CComVariant(station),pLeftSlope);
   profile->RightCrownSlope(CComVariant(station),pRightSlope);
}

Float64 CBridgeAgentImp::GetProfileGrade(Float64 station)
{
   VALIDATE( COGO_MODEL );

   CComPtr<IProfile> profile;
   GetProfile(&profile);

   Float64 grade;
   profile->Grade(CComVariant(station),&grade);
   return grade;
}

Float64 CBridgeAgentImp::GetElevation(Float64 station,Float64 offset)
{
   VALIDATE( COGO_MODEL );

   CComPtr<IProfile> profile;
   GetProfile(&profile);

   Float64 elev;
   profile->Elevation(CComVariant(station),offset,&elev);
   return elev;
}

void CBridgeAgentImp::GetBearing(Float64 station,IDirection** ppBearing)
{
   VALIDATE( COGO_MODEL );

   CComPtr<IAlignment> alignment;
   GetAlignment(&alignment);

   alignment->Bearing(CComVariant(station),ppBearing);
}

void CBridgeAgentImp::GetBearingNormal(Float64 station,IDirection** ppNormal)
{
   VALIDATE( COGO_MODEL );

   CComPtr<IAlignment> alignment;
   GetAlignment(&alignment);

   alignment->Normal(CComVariant(station),ppNormal);
}

CollectionIndexType CBridgeAgentImp::GetCurveCount()
{
   VALIDATE( COGO_MODEL );

   CComPtr<ICogoModel> cogomodel;
   m_Bridge->get_CogoModel(&cogomodel);

   CComPtr<IHorzCurveCollection> curves;
   cogomodel->get_HorzCurves(&curves);

   CollectionIndexType nCurves;
   curves->get_Count(&nCurves);
   return nCurves;
}

void CBridgeAgentImp::GetCurve(CollectionIndexType idx,IHorzCurve** ppCurve)
{
   VALIDATE( COGO_MODEL );

   CComPtr<ICogoModel> cogomodel;
   m_Bridge->get_CogoModel(&cogomodel);

   CComPtr<IHorzCurveCollection> curves;
   cogomodel->get_HorzCurves(&curves);

   CogoElementKey key = m_HorzCurveKeys[idx];

   HRESULT hr = curves->get_Item(key,ppCurve);
   ATLASSERT(SUCCEEDED(hr));
}

CollectionIndexType CBridgeAgentImp::GetVertCurveCount()
{
   VALIDATE( COGO_MODEL );

   CComPtr<ICogoModel> cogomodel;
   m_Bridge->get_CogoModel(&cogomodel);

   CComPtr<IVertCurveCollection> curves;
   cogomodel->get_VertCurves(&curves);

   CollectionIndexType nCurves;
   curves->get_Count(&nCurves);
   return nCurves;
}

void CBridgeAgentImp::GetVertCurve(CollectionIndexType idx,IVertCurve** ppCurve)
{
   VALIDATE( COGO_MODEL );

   CComPtr<ICogoModel> cogomodel;
   m_Bridge->get_CogoModel(&cogomodel);

   CComPtr<IVertCurveCollection> curves;
   cogomodel->get_VertCurves(&curves);

   CogoElementKey key = m_VertCurveKeys[idx];

   HRESULT hr = curves->get_Item(key,ppCurve);
   ATLASSERT(SUCCEEDED(hr));
}

void CBridgeAgentImp::GetCrownPoint(Float64 station,IDirection* pDirection,IPoint2d** ppPoint)
{
   VALIDATE( COGO_MODEL );

   CComPtr<IAlignment> alignment;
   GetAlignment(&alignment);

   alignment->LocateCrownPoint2D(CComVariant(station),CComVariant(pDirection),ppPoint);
}

void CBridgeAgentImp::GetCrownPoint(Float64 station,IDirection* pDirection,IPoint3d** ppPoint)
{
   VALIDATE( COGO_MODEL );

   CComPtr<IAlignment> alignment;
   GetAlignment(&alignment);

   alignment->LocateCrownPoint3D(CComVariant(station),CComVariant(pDirection),ppPoint);
}

void CBridgeAgentImp::GetPoint(Float64 station,Float64 offset,IDirection* pBearing,IPoint2d** ppPoint)

{
   VALIDATE( COGO_MODEL );

   CComPtr<IAlignment> alignment;
   GetAlignment(&alignment);

   alignment->LocatePoint(CComVariant(station),omtAlongDirection, offset,CComVariant(pBearing),ppPoint);
}

void CBridgeAgentImp::GetStationAndOffset(IPoint2d* point,Float64* pStation,Float64* pOffset)
{
   VALIDATE( COGO_MODEL );

   CComPtr<IAlignment> alignment;
   GetAlignment(&alignment);

   CComPtr<IStation> station;
   Float64 offset;
   alignment->Offset(point,&station,&offset);

   station->get_Value(pStation);
   *pOffset = offset;
}

/////////////////////////////////////////////////////////////////////////
// IBridge
//
Float64 CBridgeAgentImp::GetLength()
{
   VALIDATE( BRIDGE );
   Float64 length;
   m_Bridge->get_Length(&length);
   return length;
}

Float64 CBridgeAgentImp::GetSpanLength(SpanIndexType spanIdx)
{
   VALIDATE( BRIDGE );
   CComPtr<ISpanCollection> spans;
   m_Bridge->get_Spans(&spans);

   CComPtr<ISpan> span;
   spans->get_Item(spanIdx,&span);

   Float64 length;
   span->get_Length(&length);

   return length;
}

Float64 CBridgeAgentImp::GetAlignmentOffset()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   return pBridgeDesc->GetAlignmentOffset();
}

Float64 CBridgeAgentImp::GetDistanceFromStartOfBridge(Float64 station)
{
   VALIDATE( BRIDGE );
   CComPtr<IPierCollection> piers;
   m_Bridge->get_Piers(&piers);

   CComPtr<IPier> pier;
   piers->get_Item(0,&pier);

   CComPtr<IStation> objStation;
   pier->get_Station(&objStation);

   Float64 sta_value;
   objStation->get_Value(&sta_value);

   Float64 dist = station - sta_value;

   return dist;
}

SpanIndexType CBridgeAgentImp::GetSpanCount()
{
   VALIDATE( BRIDGE );

   CComPtr<ISpanCollection> spans;
   m_Bridge->get_Spans(&spans);
   SpanIndexType count;
   spans->get_Count(&count);
   return count;
}

PierIndexType CBridgeAgentImp::GetPierCount()
{
   VALIDATE( BRIDGE );

   CComPtr<IPierCollection> piers;
   m_Bridge->get_Piers(&piers);

   PierIndexType count;
   piers->get_Count(&count);

   return count;
}

GirderIndexType CBridgeAgentImp::GetGirderCount(SpanIndexType spanIdx)
{
   VALIDATE( BRIDGE );

   ATLASSERT( spanIdx != ALL_SPANS );

   GirderIndexType count;
   CComPtr<ISpanCollection> spans;
   m_Bridge->get_Spans(&spans);
   CComPtr<ISpan> span;
   spans->get_Item(spanIdx,&span);
   span->get_GirderCount(&count);
   return count;
}

void CBridgeAgentImp::GetDistanceBetweenGirders(const pgsPointOfInterest& poi,Float64 *pLeft,Float64* pRight)
{
   VALIDATE( BRIDGE );

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   SpanIndexType spanIdx = poi.GetSpan();
   GirderIndexType gdrIdx = poi.GetGirder();

   const CSpanData* pSpan = pBridgeDesc->GetSpan(poi.GetSpan());
   const CGirderSpacing* pStartSpacing = pSpan->GetGirderSpacing(pgsTypes::metStart);
   const CGirderSpacing* pEndSpacing   = pSpan->GetGirderSpacing(pgsTypes::metEnd);

   // girder spacing is on the right hand side of the girder

   // Get spacing on the left side of the girder at start and end
   Float64 left_start = 0;
   Float64 left_end   = 0;

   if ( 0 != gdrIdx )
   {
      SpacingIndexType spaceIdx = gdrIdx-1;
      left_start = pStartSpacing->GetGirderSpacing(spaceIdx);
      left_end   = pEndSpacing->GetGirderSpacing(spaceIdx);
   }

   // Get spacing on the right side of the girder at start and end
   GirderIndexType nGirders = GetGirderCount(spanIdx);
   Float64 right_start = 0;
   Float64 right_end   = 0;
   if ( gdrIdx < nGirders-1 )
   {
      SpacingIndexType spaceIdx = gdrIdx;
      right_start = pStartSpacing->GetGirderSpacing(spaceIdx);
      right_end   = pEndSpacing->GetGirderSpacing(spaceIdx);
   }

   Float64 gdrLength = GetGirderLength(spanIdx,gdrIdx);

   Float64 left  = ::LinInterp( poi.GetDistFromStart(), left_start,  left_end,  gdrLength );
   Float64 right = ::LinInterp( poi.GetDistFromStart(), right_start, right_end, gdrLength );

   // if the spacing is a joint spacing, we have what we are after
   if ( IsJointSpacing(pBridgeDesc->GetGirderSpacingType()) )
   {
      *pLeft = left;
      *pRight = right;
      return;
   }

   // not a joint spacing so the spacing is between CL girders.... deduct the width
   // of the adjacent girders and this girder
   Float64 left_width = 0;
   Float64 width = 0;
   Float64 right_width = 0;
   if ( 0 != gdrIdx )
   {
      pgsPointOfInterest leftPoi(poi);
      leftPoi.SetGirder(gdrIdx-1);
      left_width = max(GetTopWidth(leftPoi),GetBottomWidth(leftPoi));
   }

   width = max(GetTopWidth(poi),GetBottomWidth(poi));

   if ( gdrIdx < nGirders-1 )
   {
      pgsPointOfInterest rightPoi(poi);
      rightPoi.SetGirder(gdrIdx);
      right_width = max(GetTopWidth(rightPoi),GetBottomWidth(rightPoi));
   }

   // clear spacing is C-C spacing minus half the width of the adjacent girder minus the width of this girder
   *pLeft = left - left_width/2 - width/2;
   *pRight = right - right_width/2 - width/2;

   if ( *pLeft < 0 )
      *pLeft = 0;

   if ( *pRight < 0 )
      *pRight = 0;
}

std::vector<Float64> CBridgeAgentImp::GetGirderSpacing(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementLocation measureLocation,pgsTypes::MeasurementType measureType)
{
   VALIDATE( BRIDGE );
   SpanIndexType spanIdx = (pierFace == pgsTypes::Back ? pierIdx-1 : pierIdx);
   ATLASSERT( 0 <= spanIdx && spanIdx < GetSpanCount() );

   GirderIndexType nGirders = GetGirderCount(spanIdx);
   SpacingIndexType nSpaces = nGirders-1;

   CComPtr<IPierCollection> piers;
   m_Bridge->get_Piers(&piers);

   CComPtr<IPier> pier;
   piers->get_Item(pierIdx,&pier);

   CComPtr<IGirderSpacing> gdrSpacing;
   pier->get_GirderSpacing(pierFace == pgsTypes::Ahead ? qcbAfter : qcbBefore,&gdrSpacing);

   std::vector<Float64> spaces;
   for ( SpacingIndexType spaceIdx = 0; spaceIdx < nSpaces; spaceIdx++ )
   {
      Float64 spa;
      MeasurementType mt = (measureType == pgsTypes::NormalToItem ? mtNormal : mtAlongItem);
      MeasurementLocation ml = (measureLocation == pgsTypes::AtCenterlinePier ? mlCenterlinePier : mlCenterlineBearing);
      gdrSpacing->get_GirderSpacing(spaceIdx,ml,mt,&spa);
      spaces.push_back(spa);
   }

   return spaces;
}

std::vector<SpaceBetweenGirder> CBridgeAgentImp::GetGirderSpacing(SpanIndexType spanIdx,Float64 distFromStartOfSpan)
{
   VALIDATE( BRIDGE );
   std::vector<SpaceBetweenGirder> vSpacing;

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   SpacingIndexType nSpaces = pBridgeDesc->GetSpan(spanIdx)->GetGirderCount() - 1;

   if ( nSpaces <= 0 )
      return vSpacing;

   CComPtr<ISpanCollection> spans;
   m_Bridge->get_Spans(&spans);

   CComPtr<ISpan> span;
   spans->get_Item(spanIdx,&span);

   CComPtr<IGirderSpacing> spacing;
   span->get_GirderSpacing(etStart,&spacing);

   SpaceBetweenGirder gdrSpacing;
   gdrSpacing.firstGdrIdx = 0;
   gdrSpacing.lastGdrIdx  = 0;
   gdrSpacing.spacing = -1;

   for ( SpacingIndexType spaceIdx = 0; spaceIdx < nSpaces; spaceIdx++ )
   {
      Float64 space;
      spacing->get_SpaceWidth(spaceIdx,distFromStartOfSpan,&space);

      gdrSpacing.lastGdrIdx++;

      if ( !IsEqual(gdrSpacing.spacing,space) )
      {
         // this is a new spacing
         gdrSpacing.spacing = space; // save it
         vSpacing.push_back(gdrSpacing);
      }
      else
      {
         // this space is the same as the previous one... change the lastGdrIdx
         vSpacing.back().lastGdrIdx = gdrSpacing.lastGdrIdx;
      }

      gdrSpacing.firstGdrIdx = gdrSpacing.lastGdrIdx;
   }

   return vSpacing;
}

void CBridgeAgentImp::GetSpacingAlongGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx,Float64 distFromStartOfGirder,Float64* leftSpacing,Float64* rightSpacing)
{
   VALIDATE(BRIDGE);

   CComPtr<ISpanCollection> spans;
   m_Bridge->get_Spans(&spans);
   CComPtr<ISpan> span;
   spans->get_Item(spanIdx,&span);

   CComPtr<IGirderSpacing> gdrSpacing;
   span->get_GirderSpacing(etStart,&gdrSpacing);

   gdrSpacing->get_SpacingAlongGirder(gdrIdx,distFromStartOfGirder, qcbLeft,  leftSpacing);
   gdrSpacing->get_SpacingAlongGirder(gdrIdx,distFromStartOfGirder, qcbRight, rightSpacing);
}

Float64 CBridgeAgentImp::GetGirderLength(SpanIndexType span,GirderIndexType gdr)
{
   VALIDATE(BRIDGE);
   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);
   Float64 length;
   girder->get_GirderLength(&length);
   return length;
}

Float64 CBridgeAgentImp::GetSpanLength(SpanIndexType span,GirderIndexType gdr)
{
   VALIDATE(BRIDGE);
   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);
   Float64 length;
   girder->get_SpanLength(&length);
   return length;
}

Float64 CBridgeAgentImp::GetGirderPlanLength(SpanIndexType span,GirderIndexType gdr)
{
   Float64 length = GetGirderLength(span,gdr);
   Float64 slope  = GetGirderSlope(span,gdr);

   Float64 plan_length = length*sqrt(1.0 + slope*slope);

   return plan_length;
}

Float64 CBridgeAgentImp::GetGirderSlope(SpanIndexType span,GirderIndexType gdr)
{
   Float64 length = GetSpanLength(span,gdr);

   Float64 end1 = GetGirderStartConnectionLength(span,gdr);

   Float64 sta1,offset1;
   Float64 sta2,offset2;
   GetStationAndOffset(pgsPointOfInterest(span,gdr,end1),       &sta1,&offset1);
   GetStationAndOffset(pgsPointOfInterest(span,gdr,length+end1),&sta2,&offset2);

   Float64 elev1,elev2;
   elev1 = GetElevation(sta1,offset1);
   elev2 = GetElevation(sta2,offset2);

   Float64 slabOffset1,slabOffset2;
   slabOffset1 = GetSlabOffset(span,gdr,pgsTypes::metStart);
   slabOffset2 = GetSlabOffset(span,gdr,pgsTypes::metEnd);

   Float64 dy = (elev2-slabOffset2) - (elev1-slabOffset1);

   Float64 slope = dy/length;

   return slope;
}

Float64 CBridgeAgentImp::GetCCPierLength(SpanIndexType span,GirderIndexType gdr)
{
   CComPtr<ICogoInfo> cogoInfo;
   m_Bridge->get_CogoInfo(&cogoInfo);
   
   CogoElementKey id1,id2;

   cogoInfo->get_PierGirderIntersectionPointID(span,  gdr,qcbAfter,&id1);
   cogoInfo->get_PierGirderIntersectionPointID(span+1,gdr,qcbBefore,&id2);

   CComPtr<ICogoModel> cogoModel;
   m_Bridge->get_CogoModel(&cogoModel);
   
   CComPtr<IPointCollection> points;
   cogoModel->get_Points(&points);

   // Get the points where the girder line intersect the CL pier
   CComPtr<IPoint2d> pntPier1, pntPier2;
   points->get_Item(id1,&pntPier1);
   points->get_Item(id2,&pntPier2);

   CComQIPtr<IMeasure2> measure(m_CogoEngine);
   Float64 length;
   measure->Distance(pntPier1,pntPier2,&length);

   return length;
}

Float64 CBridgeAgentImp::GetGirderStartConnectionLength(SpanIndexType span,GirderIndexType gdr)
{
   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);
   Float64 length;
   girder->get_LeftEndSize(&length);
   return length;
}

Float64 CBridgeAgentImp::GetGirderEndConnectionLength(SpanIndexType span,GirderIndexType gdr)
{
   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);
   Float64 length;
   girder->get_RightEndSize(&length);
   return length;
}

Float64 CBridgeAgentImp::GetGirderOffset(SpanIndexType span,GirderIndexType gdr,Float64 station)
{
   VALIDATE( BRIDGE );
   Float64 offset;
   HRESULT hr = m_BridgeGeometryTool->GirderPathOffset(m_Bridge,span,gdr,CComVariant(station),&offset);
   ATLASSERT( SUCCEEDED(hr) );
   return offset;
}

void CBridgeAgentImp::GetPoint(SpanIndexType span,GirderIndexType gdr,Float64 distFromStartOfSpan,IPoint2d** ppPoint)
{
   VALIDATE( BRIDGE );
   HRESULT hr = m_BridgeGeometryTool->Point(m_Bridge,span,gdr,distFromStartOfSpan,ppPoint);
   ATLASSERT( SUCCEEDED(hr) );
}

void CBridgeAgentImp::GetPoint(const pgsPointOfInterest& poi,IPoint2d** ppPoint)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   Float64 gdr_end_dist = GetGirderStartConnectionLength(span,gdr);
   Float64 brg_offset = GetGirderStartBearingOffset(span,gdr);
   GetPoint(span,gdr,poi.GetDistFromStart()+brg_offset - gdr_end_dist,ppPoint);
}

void CBridgeAgentImp::GetStationAndOffset(SpanIndexType span,GirderIndexType gdr,Float64 distFromStartOfSpan,Float64* pStation,Float64* pOffset)
{
   VALIDATE( BRIDGE );
   CComPtr<IStation> station;
   Float64 offset;
   HRESULT hr = m_BridgeGeometryTool->StationAndOffset(m_Bridge,span,gdr,distFromStartOfSpan,&station,&offset);
   ATLASSERT( SUCCEEDED(hr) );

   station->get_Value(pStation);
   *pOffset = offset;
}

void CBridgeAgentImp::GetStationAndOffset(const pgsPointOfInterest& poi,Float64* pStation,Float64* pOffset)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();
   GetStationAndOffset(span,gdr,poi.GetDistFromStart(),pStation,pOffset);
}

Float64 CBridgeAgentImp::GetDistanceFromStartOfBridge(const pgsPointOfInterest& poi)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();
   return GetDistanceFromStartOfBridge(span,gdr,poi.GetDistFromStart());
}

Float64 CBridgeAgentImp::GetDistanceFromStartOfBridge(SpanIndexType span,GirderIndexType gdr,Float64 distFromStartOfSpan)
{
   Float64 station,offset;
   GetStationAndOffset(span,gdr,distFromStartOfSpan,&station,&offset);

   Float64 start_station = GetPierStation(0);
   return station - start_station;
}

void CBridgeAgentImp::GetDistFromStartOfSpan(GirderIndexType gdrIdx,Float64 distFromStartOfBridge,SpanIndexType* pSpanIdx,Float64* pDistFromStartOfSpan)
{
   VALIDATE(BRIDGE);

   m_BridgeGeometryTool->GirderLinePoint(m_Bridge,distFromStartOfBridge,gdrIdx,pSpanIdx,pDistFromStartOfSpan);
}

bool CBridgeAgentImp::IsInteriorGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   return pBridgeDesc->GetSpan(spanIdx)->IsInteriorGirder(gdrIdx);
}

bool CBridgeAgentImp::IsExteriorGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   return pBridgeDesc->GetSpan(spanIdx)->IsExteriorGirder(gdrIdx);
}

void CBridgeAgentImp::GetLeftSideEndDiaphragmSize(PierIndexType pierIdx,Float64* pW,Float64* pH)
{
   VALIDATE( BRIDGE );

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData* pPierData = pBridgeDesc->GetPier(pierIdx);
   ATLASSERT( pPierData );

   *pH = pPierData->GetConnectionLibraryEntry(pgsTypes::Back)->GetDiaphragmHeight();
   *pW = pPierData->GetConnectionLibraryEntry(pgsTypes::Back)->GetDiaphragmWidth();
}

void CBridgeAgentImp::GetRightSideEndDiaphragmSize(PierIndexType pierIdx,Float64* pW,Float64* pH)
{
   VALIDATE( BRIDGE );
  
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData* pPierData = pBridgeDesc->GetPier(pierIdx);
   ATLASSERT( pPierData );

   *pH = pPierData->GetConnectionLibraryEntry(pgsTypes::Ahead)->GetDiaphragmHeight();
   *pW = pPierData->GetConnectionLibraryEntry(pgsTypes::Ahead)->GetDiaphragmWidth();
}

Float64 CBridgeAgentImp::GetGirderStartBearingOffset(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   // returns distance from CL pier to the bearing line,
   // measured along CL girder
   CComPtr<IPrecastGirder> girder;
   GetGirder(spanIdx,gdrIdx,&girder);
   Float64 length;
   girder->get_LeftBearingOffset(&length);
   return length;
}

Float64 CBridgeAgentImp::GetGirderEndBearingOffset(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   // returns distance from CL pier to the bearing line,
   // measured along CL girder
   CComPtr<IPrecastGirder> girder;
   GetGirder(spanIdx,gdrIdx,&girder);
   Float64 length;
   girder->get_RightBearingOffset(&length);
   return length;
}

Float64 CBridgeAgentImp::GetGirderStartSupportWidth(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   // returns the support width
   // measured along the CL girder
   VALIDATE( BRIDGE );

   CComPtr<IConnection> start, end;
   HRESULT hr = GetConnections(spanIdx,gdrIdx,&start,&end);
   ATLASSERT(SUCCEEDED(hr));

   Float64 support_width;
   start->get_SupportWidth(&support_width);

   return support_width;
}

Float64 CBridgeAgentImp::GetGirderEndSupportWidth(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   // returns the support width
   // measured along the CL girder
   VALIDATE( BRIDGE );

   CComPtr<IConnection> start, end;
   HRESULT hr = GetConnections(spanIdx,gdrIdx,&start,&end);
   ATLASSERT(SUCCEEDED(hr));

   Float64 support_width;
   end->get_SupportWidth(&support_width);

   return support_width;
}

Float64 CBridgeAgentImp::GetCLPierToCLBearingDistance(SpanIndexType span,GirderIndexType gdr,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementType measure)
{
   VALIDATE( BRIDGE );

   Float64 dist; // distance from CL pier to CL Bearing along the CL girder
   switch( pierFace )
   {
      case pgsTypes::Ahead: // at start of span
         dist = GetGirderStartBearingOffset(span,gdr);
         break;

      case pgsTypes::Back: // at end of span
         dist = GetGirderEndBearingOffset(span,gdr);
         break;
   }

   if ( measure == pgsTypes::NormalToItem )
   {
      // we want dist to be measured normal to the pier
      // adjust the distance
      PierIndexType pier = (pierFace == pgsTypes::Ahead ? span : span+1);
      CComPtr<IDirection> dirBeam, dirPier;
      GetGirderBearing(span,gdr,&dirBeam);
      GetPierDirection(pier,&dirPier);

      CComPtr<IAngle> angleBetween;
      dirPier->AngleBetween(dirBeam,&angleBetween);

      Float64 angle;
      angleBetween->get_Value(&angle);

      dist *= sin(angle);
   }


   return dist;
}

Float64 CBridgeAgentImp::GetCLPierToGirderEndDistance(SpanIndexType span,GirderIndexType gdr,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementType measure)
{
   VALIDATE( BRIDGE );
   Float64 distCLPierToCLBearingAlongGirder = GetCLPierToCLBearingDistance(span,gdr,pierFace,pgsTypes::AlongItem);
   Float64 endDist;

   switch ( pierFace )
   {
   case pgsTypes::Ahead: // at start of span
      endDist = GetGirderStartConnectionLength(span,gdr);
      break;

   case pgsTypes::Back: // at end of span
      endDist = GetGirderEndConnectionLength(span,gdr);
      break;
   }

   Float64 distCLPierToEndGirderAlongGirder = distCLPierToCLBearingAlongGirder - endDist;

   if ( measure == pgsTypes::NormalToItem )
   {
      // we want dist to be measured normal to the pier
      // adjust the distance
      PierIndexType pier = (pierFace == pgsTypes::Ahead ? span : span+1);
      CComPtr<IDirection> dirBeam, dirPier;
      GetGirderBearing(span,gdr,&dirBeam);
      GetPierDirection(pier,&dirPier);

      CComPtr<IAngle> angleBetween;
      dirPier->AngleBetween(dirBeam,&angleBetween);

      Float64 angle;
      angleBetween->get_Value(&angle);

      distCLPierToEndGirderAlongGirder *= sin(angle);
   }

   return distCLPierToEndGirderAlongGirder;
}

void CBridgeAgentImp::GetGirderBearing(SpanIndexType span,GirderIndexType gdr,IDirection** ppBearing)
{
   VALIDATE( BRIDGE );
   m_BridgeGeometryTool->GirderLineBearing(m_Bridge,span,gdr,ppBearing);
}

void CBridgeAgentImp::GetGirderAngle(SpanIndexType span,GirderIndexType gdr,pgsTypes::PierFaceType face,IAngle** ppAngle)
{
   CComPtr<IDirection> gdrDir;
   GetGirderBearing(span,gdr,&gdrDir);

   CComPtr<IDirection> pierDir;
   GetPierDirection(face == pgsTypes::Ahead ? span : span+1,&pierDir);

   pierDir->AngleBetween(gdrDir,ppAngle);
}

//-----------------------------------------------------------------------------
bool CBridgeAgentImp::GetSpan(Float64 station,SpanIndexType* pSpanIdx)
{
   PierIndexType nPiers = GetPierCount();
   Float64 prev_pier = GetPierStation(0);
   for ( PierIndexType pierIdx = 1; pierIdx < nPiers; pierIdx++ )
   {
      Float64 next_pier;
      next_pier = GetPierStation(pierIdx);

      if ( IsLE(prev_pier,station) && IsLE(station,next_pier) )
      {
         *pSpanIdx = SpanIndexType(pierIdx-1);
         return true;
      }

      prev_pier = next_pier;
   }

   return false; // not found
}

//-----------------------------------------------------------------------------
GDRCONFIG CBridgeAgentImp::GetGirderConfiguration(SpanIndexType span,GirderIndexType gdr)
{
   // Make sure girder is properly modeled before beginning
   VALIDATE( GIRDER );

   GDRCONFIG config;

   // Get the girder
   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   // Strand fills
   CComPtr<IIndexArray> fill[3];
   ConfigStrandFillVector fillVec[3];

   girder->get_StraightStrandFill(&fill[pgsTypes::Straight]);
   IndexArray2ConfigStrandFillVec(fill[pgsTypes::Straight], fillVec[pgsTypes::Straight]);
   config.PrestressConfig.SetStrandFill(pgsTypes::Straight, fillVec[pgsTypes::Straight]);

   girder->get_HarpedStrandFill(&fill[pgsTypes::Harped]);
   IndexArray2ConfigStrandFillVec(fill[pgsTypes::Harped], fillVec[pgsTypes::Harped]);
   config.PrestressConfig.SetStrandFill(pgsTypes::Harped, fillVec[pgsTypes::Harped]);

   girder->get_TemporaryStrandFill(&fill[pgsTypes::Temporary]);
   IndexArray2ConfigStrandFillVec(fill[pgsTypes::Temporary], fillVec[pgsTypes::Temporary]);
   config.PrestressConfig.SetStrandFill(pgsTypes::Temporary, fillVec[pgsTypes::Temporary]);

   // Get harping point offsets
   girder->get_HarpedStrandAdjustmentEnd(&config.PrestressConfig.EndOffset);
   girder->get_HarpedStrandAdjustmentHP(&config.PrestressConfig.HpOffset);

   // Get jacking force and debonding
   GET_IFACE(IGirderData,pGirderData);   
   const CGirderData* pgirderData = pGirderData->GetGirderData(span,gdr);
   for ( Uint16 i = 0; i < 3; i++ )
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;

      config.PrestressConfig.Pjack[strandType] = pgirderData->PrestressData.Pjack[strandType];

      // Convert debond data
      // Use tool to compute strand position index from grid index used by CDebondInfo
      ConfigStrandFillTool fillTool( fillVec[strandType] );

      std::vector<CDebondInfo>::const_iterator iter = pgirderData->PrestressData.Debond[strandType].begin();
      std::vector<CDebondInfo>::const_iterator iterend = pgirderData->PrestressData.Debond[strandType].end();
      while(iter != iterend)
      {
         const CDebondInfo& debond_info = *iter;

         // convert grid index to strands index
         StrandIndexType index1, index2;
         fillTool.GridIndexToStrandPositionIndex(debond_info.strandTypeGridIdx, &index1, &index2);

         DEBONDCONFIG di;
         ATLASSERT(index1 != INVALID_INDEX);
         di.strandIdx         = index1;
         di.LeftDebondLength  = debond_info.Length1;
         di.RightDebondLength = debond_info.Length2;

         config.PrestressConfig.Debond[i].push_back(di);

         if ( index2 != INVALID_INDEX )
         {
            di.strandIdx         = index2;
            di.LeftDebondLength  = debond_info.Length1;
            di.RightDebondLength = debond_info.Length2;

            config.PrestressConfig.Debond[i].push_back(di);
         }

         iter++;
      }

      // sorts based on strand index
      std::sort(config.PrestressConfig.Debond[strandType].begin(),config.PrestressConfig.Debond[strandType].end());

      for ( Uint16 j = 0; j < 2; j++ )
      {
         pgsTypes::MemberEndType endType = pgsTypes::MemberEndType(j);
         const std::vector<GridIndexType>& gridIndicies(pgirderData->PrestressData.GetExtendedStrands(strandType,endType));
         std::vector<StrandIndexType> strandIndicies;
         std::vector<GridIndexType>::const_iterator iter(gridIndicies.begin());
         std::vector<GridIndexType>::const_iterator end(gridIndicies.end());
         for ( ; iter != end; iter++ )
         {
            // convert grid index to strands index
            GridIndexType gridIdx = *iter;
            StrandIndexType index1, index2;
            fillTool.GridIndexToStrandPositionIndex(gridIdx, &index1, &index2);
            ATLASSERT(index1 != INVALID_INDEX);
            strandIndicies.push_back(index1);
            if ( index2 != INVALID_INDEX )
            {
               strandIndicies.push_back(index2);
            }
         }
         std::sort(strandIndicies.begin(),strandIndicies.end());
         config.PrestressConfig.SetExtendedStrands(strandType,endType,strandIndicies);
      }
   }

   config.PrestressConfig.TempStrandUsage = pgirderData->PrestressData.TempStrandUsage;

   // Get concrete properties
   config.Fc       = GetFcGdr(span,gdr);
   config.Fci      = GetFciGdr(span,gdr);
   config.ConcType = GetGdrConcreteType(span,gdr);
   config.bHasFct  = DoesGdrConcreteHaveAggSplittingStrength(span,gdr);
   config.Fct      = GetGdrConcreteAggSplittingStrength(span,gdr);


   config.Ec  = GetEcGdr(span,gdr);
   config.Eci = GetEciGdr(span,gdr);
   config.bUserEci = pgirderData->Material.bUserEci;
   config.bUserEc  = pgirderData->Material.bUserEc;

   // Slab offset
   config.SlabOffset[pgsTypes::metStart] = GetSlabOffset(span,gdr,pgsTypes::metStart);
   config.SlabOffset[pgsTypes::metEnd]   = GetSlabOffset(span,gdr,pgsTypes::metEnd);

   // Stirrup data
	const CShearData& rsheardata = GetShearData(span,gdr);

   WriteShearDataToStirrupConfig(rsheardata, config.StirrupConfig);

   return config;
}

bool CBridgeAgentImp::DoesLeftSideEndDiaphragmLoadGirder(PierIndexType pierIdx)
{
   VALIDATE( BRIDGE );
  
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData* pPierData = pBridgeDesc->GetPier(pierIdx);
   ATLASSERT( pPierData );

   return (pPierData->GetConnectionLibraryEntry(pgsTypes::Back)->GetDiaphragmLoadType() != ConnectionLibraryEntry::DontApply);
}

bool CBridgeAgentImp::DoesRightSideEndDiaphragmLoadGirder(PierIndexType pierIdx)
{
   VALIDATE( BRIDGE );
  
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData* pPierData = pBridgeDesc->GetPier(pierIdx);
   ATLASSERT( pPierData );

   return (pPierData->GetConnectionLibraryEntry(pgsTypes::Ahead)->GetDiaphragmLoadType() != ConnectionLibraryEntry::DontApply);
}

Float64 CBridgeAgentImp::GetEndDiaphragmLoadLocationAtStart(SpanIndexType span,GirderIndexType gdr)
{
   VALIDATE( BRIDGE );
  
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);
   const CPierData* pPierData = pSpan->GetPrevPier();
   ATLASSERT( pPierData );

   Float64 dist;
   if (pPierData->GetConnectionLibraryEntry(pgsTypes::Ahead)->GetDiaphragmLoadType()==ConnectionLibraryEntry::ApplyAtSpecifiedLocation)
   {
      // return distance adjusted for skew
      dist  = pPierData->GetConnectionLibraryEntry(pgsTypes::Ahead)->GetDiaphragmLoadLocation();

      CComPtr<IAngle> angle;
      GetPierSkew(span,&angle);
      Float64 value;
      angle->get_Value(&value);

      dist /=  cos ( fabs(value) );
   }
   else if (pPierData->GetConnectionLibraryEntry(pgsTypes::Ahead)->GetDiaphragmLoadType()==ConnectionLibraryEntry::ApplyAtBearingCenterline)
   {
      // same as bearing offset
      dist = GetGirderStartBearingOffset(span,gdr);
   }
   else
   {
      CHECK(0); // end diaphragm load is not applied
      dist = 0.0;
   }

   return dist;
}

Float64 CBridgeAgentImp::GetEndDiaphragmLoadLocationAtEnd(SpanIndexType span,GirderIndexType gdr)
{
   VALIDATE( BRIDGE );
  
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);
   const CPierData* pPierData = pSpan->GetNextPier();
   ATLASSERT( pPierData );

   Float64 dist;
   if (pPierData->GetConnectionLibraryEntry(pgsTypes::Back)->GetDiaphragmLoadType()==ConnectionLibraryEntry::ApplyAtSpecifiedLocation)
   {
      // return distance adjusted for skew
      dist  = pPierData->GetConnectionLibraryEntry(pgsTypes::Back)->GetDiaphragmLoadLocation();

      CComPtr<IAngle> angle;
      GetPierSkew(span+1,&angle);
      Float64 value;
      angle->get_Value(&value);

      dist /=  cos ( fabs(value) );
   }
   else if (pPierData->GetConnectionLibraryEntry(pgsTypes::Back)->GetDiaphragmLoadType()==ConnectionLibraryEntry::ApplyAtBearingCenterline)
   {
      // same as bearing offset
      dist = GetGirderEndBearingOffset(span,gdr);
   }
   else
   {
      CHECK(0); // end diaphragm load is not applied
      dist = 0.0;
   }

   return dist;
}

std::vector<IntermedateDiaphragm> CBridgeAgentImp::GetIntermediateDiaphragms(pgsTypes::Stage stage,SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   // make sure stage is a valid value for this function
   ATLASSERT( stage == pgsTypes::CastingYard || stage == pgsTypes::BridgeSite1 );

   GET_IFACE(IGirder, pGirder);

   GET_IFACE(ILibrary,pLib);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(spanIdx);
   std::_tstring strGirderName = pSpan->GetGirderTypes()->GetGirderName(gdrIdx);

   const GirderLibraryEntry* pGirderEntry = pLib->GetGirderEntry( strGirderName.c_str() );

   Float64 span_length      = GetSpanLength( spanIdx, gdrIdx );
   Float64 girder_length    = GetGirderLength( spanIdx, gdrIdx );
   Float64 end_size         = GetGirderStartConnectionLength( spanIdx, gdrIdx );
   Float64 start_brg_offset = GetGirderStartBearingOffset( spanIdx,gdrIdx );
   Float64 end_brg_offset   = GetGirderEndBearingOffset( spanIdx, gdrIdx );
   bool   bIsInterior      = IsInteriorGirder( spanIdx, gdrIdx );

   // base the span length on the maximum span length in this span
   // we want the same number of diaphragms on every girder
   GirderIndexType nGirders = GetGirderCount(spanIdx);
   Float64 max_span_length = 0;
   for ( GirderIndexType i = 0; i < nGirders; i++ )
   {
      max_span_length = _cpp_max(max_span_length, GetSpanLength( spanIdx, i ));
   }

   CComPtr<ISpanCollection> spans;
   m_Bridge->get_Spans(&spans);
   CComPtr<ISpan> span;
   spans->get_Item(spanIdx,&span);

   CComPtr<IGirderSpacing> gdrSpacing;
   span->get_GirderSpacing(etStart,&gdrSpacing);

   CComPtr<IAngle> objSkew1, objSkew2;
   GetPierSkew(spanIdx,  &objSkew1);
   GetPierSkew(spanIdx+1,&objSkew2);

   Float64 skew1, skew2;
   objSkew1->get_Value(&skew1);
   objSkew2->get_Value(&skew2);

   std::vector<IntermedateDiaphragm> diaphragms;

   const GirderLibraryEntry::DiaphragmLayoutRules& rules = pGirderEntry->GetDiaphragmLayoutRules();

   GirderLibraryEntry::DiaphragmLayoutRules::const_iterator iter;
   for ( iter = rules.begin(); iter != rules.end(); iter++ )
   {
      const GirderLibraryEntry::DiaphragmLayoutRule& rule = *iter;

      if ( max_span_length <= rule.MinSpan || rule.MaxSpan < max_span_length )
         continue; // this rule doesn't apply

      IntermedateDiaphragm diaphragm;
      if ( rule.Method == GirderLibraryEntry::dwmInput )
      {
         diaphragm.m_bCompute = false;
         diaphragm.P = rule.Weight;
      }
      else
      {
         diaphragm.m_bCompute = true;
         diaphragm.H = rule.Height;
         diaphragm.T = rule.Thickness;
      }

      // determine location of diaphragm load (from the reference point - whatever that is, see below)
      Float64 location1 = rule.Location;
      Float64 location2 = rule.Location;
      if ( rule.MeasureType == GirderLibraryEntry::mtFractionOfSpanLength )
      {
         location1 *= span_length;
         location2 = (1 - location2)*span_length;
      }
      else if ( rule.MeasureType == GirderLibraryEntry::mtFractionOfGirderLength )
      {
         location1 *= girder_length;
         location2 = (1 - location2)*girder_length;
      }

      // adjust location so that it is measured from the end of the girder
      if ( rule.MeasureLocation == GirderLibraryEntry::mlBearing )
      {
         // reference point is the bearing so add the end size
         location1 += end_size;
         location2 += end_size;
      }
      else if ( rule.MeasureLocation == GirderLibraryEntry::mlCenterlineOfGirder )
      {
         // reference point is the center line of the girder so go back from the centerline
         location1 = girder_length/2 - location1;
         location2 = girder_length/2 + location2; // locate the diaphragm -/+ from cl girder
      }

      if ( location1 < 0.0 || girder_length < location1 || location2 < 0.0 || girder_length < location2 )
      {
         GET_IFACE(IEAFStatusCenter,pStatusCenter);
         std::_tstring str(_T("An interior diaphragm is located outside of the girder length. The diaphragm load will not be applied. Check the diaphragm rules for this girder."));

         pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID,m_scidInformationalError,str.c_str());
         pStatusCenter->Add(pStatusItem);
         break;
      }

      // location the first diaphragm
      diaphragm.Location = location1;
      Float64 skew = (skew2-skew1)*location1/(start_brg_offset + span_length + end_brg_offset) + skew1;
      pgsPointOfInterest poi(spanIdx,gdrIdx,location1);
      bool bDiaphragmAdded = false;

      Float64 left_spacing, right_spacing;
      gdrSpacing->get_SpacingAlongGirder(gdrIdx,location1, qcbLeft, &left_spacing);
      gdrSpacing->get_SpacingAlongGirder(gdrIdx,location1, qcbRight,&right_spacing);

      if ( rule.Type == GirderLibraryEntry::dtExternal && stage == pgsTypes::BridgeSite1 )
      {
         // external diaphragms are only applied in the bridge site stage
         // determine length (width) of the diaphragm
         Float64 W = 0;
         WebIndexType nWebs = pGirder->GetNumberOfWebs(spanIdx,gdrIdx);

         Float64 web_location_left  = pGirder->GetWebLocation(poi,0);
         Float64 web_location_right = pGirder->GetWebLocation(poi,nWebs-1);

         Float64 tweb_left  = pGirder->GetWebThickness(poi,0)/2;
         Float64 tweb_right = pGirder->GetWebThickness(poi,nWebs-1)/2;

         if ( bIsInterior )
         {
            W = (left_spacing/2 + right_spacing/2 - fabs(web_location_left) - tweb_left - fabs(web_location_right) - tweb_right);
         }
         else
         {
            W = (gdrIdx == 0 ? right_spacing/2 - fabs(web_location_right) - tweb_right : left_spacing/2 - fabs(web_location_left) - tweb_left );
         }

         diaphragm.W = W/cos(skew);

         if ( !diaphragm.m_bCompute )
         {
            // P is weight/length of external application
            // make P the total weight here
            diaphragm.P *= diaphragm.W;
         }

         diaphragms.push_back(diaphragm);
         bDiaphragmAdded = true;
      }
      else
      {
         // internal diaphragm
         if (
              (rule.Construction == GirderLibraryEntry::ctCastingYard && stage == pgsTypes::CastingYard ) ||
              (rule.Construction == GirderLibraryEntry::ctBridgeSite  && stage == pgsTypes::BridgeSite1 )
            )
         {
            if ( diaphragm.m_bCompute )
            {
               // determine length (width) of the diaphragm
               Float64 W = 0;
               WebIndexType nWebs = pGirder->GetNumberOfWebs(spanIdx,gdrIdx);

               if ( 1 < nWebs )
               {
                  SpacingIndexType nSpaces = nWebs-1;

                  // add up the spacing between the centerlines of webs in the girder cross section
                  for ( SpacingIndexType spaceIdx = 0; spaceIdx < nSpaces; spaceIdx++ )
                  {
                     Float64 s = pGirder->GetWebSpacing(poi,spaceIdx);
                     W += s;
                  }

                  // deduct the thickness of the webs
                  for ( WebIndexType webIdx = 0; webIdx < nWebs; webIdx++ )
                  {
                     Float64 t = pGirder->GetWebThickness(poi,webIdx);

                     if ( webIdx == 0 || webIdx == nWebs-1 )
                        W -= t/2;
                     else
                        W -= t;
                  }
               }

               diaphragm.W = W/cos(skew);
            }

            diaphragms.push_back(diaphragm);
            bDiaphragmAdded = true;
         }
      }


      if ( !IsEqual(location1,location2) && bDiaphragmAdded )
      {
         // location the second diaphragm
         if ( rule.Method == GirderLibraryEntry::dwmInput )
         {
            diaphragm.m_bCompute = false;
            diaphragm.P = rule.Weight;
         }
         else
         {
            diaphragm.m_bCompute = true;
            diaphragm.H = rule.Height;
            diaphragm.T = rule.Thickness;
         }

         diaphragm.Location = location2;
         Float64 skew = (skew2-skew1)*location2/(start_brg_offset + span_length + end_brg_offset) + skew1;
         pgsPointOfInterest poi(spanIdx,gdrIdx,location2);
         bool bDiaphragmAdded = false;

         gdrSpacing->get_SpacingAlongGirder(gdrIdx,location2, qcbLeft, &left_spacing);
         gdrSpacing->get_SpacingAlongGirder(gdrIdx,location2, qcbRight,&right_spacing);

         if ( rule.Type == GirderLibraryEntry::dtExternal && stage == pgsTypes::BridgeSite1 )
         {
            // external diaphragms are only applied in the bridge site stage
            // determine length (width) of the diaphragm
            Float64 W = 0;
            WebIndexType nWebs = pGirder->GetNumberOfWebs(spanIdx,gdrIdx);

            Float64 web_location_left  = pGirder->GetWebLocation(poi,0);
            Float64 web_location_right = pGirder->GetWebLocation(poi,nWebs-1);

            Float64 tweb_left  = pGirder->GetWebThickness(poi,0)/2;
            Float64 tweb_right = pGirder->GetWebThickness(poi,nWebs-1)/2;

            if ( bIsInterior )
            {
               W = (left_spacing/2 + right_spacing/2 - fabs(web_location_left) - tweb_left - fabs(web_location_right) - tweb_right);
            }
            else
            {
               W = (gdrIdx == 0 ? right_spacing/2 - fabs(web_location_right) - tweb_right : left_spacing/2 - fabs(web_location_left) - tweb_left );
            }

            diaphragm.W = W/cos(skew);

            if ( !diaphragm.m_bCompute )
            {
               // P is weight/length of external application
               // make P the total weight here
               diaphragm.P *= diaphragm.W;
            }

            diaphragms.push_back(diaphragm);
            bDiaphragmAdded = true;
         }
         else
         {
            // internal diaphragm
            if (
                 (rule.Construction == GirderLibraryEntry::ctCastingYard && stage == pgsTypes::CastingYard ) ||
                 (rule.Construction == GirderLibraryEntry::ctBridgeSite  && stage == pgsTypes::BridgeSite1 )
               )
            {
               if ( diaphragm.m_bCompute )
               {
                     // determine length (width) of the diaphragm
                     Float64 W = 0;
                     WebIndexType nWebs = pGirder->GetNumberOfWebs(spanIdx,gdrIdx);
                     if ( 1 < nWebs )
                     {
                     SpacingIndexType nSpaces = nWebs-1;

                     // add up the spacing between the centerlines of webs in the girder cross section
                     for ( SpacingIndexType spaceIdx = 0; spaceIdx < nSpaces; spaceIdx++ )
                     {
                        Float64 s = pGirder->GetWebSpacing(poi,spaceIdx);
                        W += s;
                     }

                     // deduct the thickness of the webs
                     for ( WebIndexType webIdx = 0; webIdx < nWebs; webIdx++ )
                     {
                        Float64 t = pGirder->GetWebThickness(poi,webIdx);

                        if ( webIdx == 0 || webIdx == nWebs-1 )
                           W -= t/2;
                        else
                           W -= t;
                     }

                     diaphragm.W = W/cos(skew);
                  }
               }

               diaphragms.push_back(diaphragm);
               bDiaphragmAdded = true;
            }
         }
      }
   }

   return diaphragms;
}

pgsTypes::SupportedDeckType CBridgeAgentImp::GetDeckType()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();
   return pDeck->DeckType;
}

Float64 CBridgeAgentImp::GetSlabOffset(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::MemberEndType end)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   return pBridgeDesc->GetSpan(spanIdx)->GetGirderTypes()->GetSlabOffset(gdrIdx,end);
}

Float64 CBridgeAgentImp::GetSlabOffset(const pgsPointOfInterest& poi)
{
   // compute the "A" dimension at the poi by linearly interpolating the value
   // between the start and end bearings (this assumes no camber in the girder)
   SpanIndexType  spanIdx = poi.GetSpan();
   GirderIndexType gdrIdx = poi.GetGirder();
   Float64 dist_from_start_of_girder = poi.GetDistFromStart();

   Float64 Astart = GetSlabOffset(spanIdx,gdrIdx,pgsTypes::metStart);
   Float64 Aend   = GetSlabOffset(spanIdx,gdrIdx,pgsTypes::metEnd);
   Float64 span_length = GetSpanLength(spanIdx,gdrIdx);

   Float64 dist_to_start_brg = GetGirderStartConnectionLength(spanIdx,gdrIdx);

   Float64 dist_from_brg_to_poi = dist_from_start_of_girder - dist_to_start_brg;

   Float64 slab_offset = ::LinInterp(dist_from_brg_to_poi,Astart,Aend,span_length);

   return slab_offset;
}

Float64 CBridgeAgentImp::GetSlabOffset(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   // compute the "A" dimension at the poi by linearly interpolating the value
   // between the start and end bearings (this assumes no camber in the girder)
   SpanIndexType  spanIdx = poi.GetSpan();
   GirderIndexType gdrIdx = poi.GetGirder();
   Float64 dist_from_start_of_girder = poi.GetDistFromStart();

   Float64 Astart = config.SlabOffset[pgsTypes::metStart];
   Float64 Aend   = config.SlabOffset[pgsTypes::metEnd];
   Float64 span_length = GetSpanLength(spanIdx,gdrIdx);

   Float64 dist_to_start_brg = GetGirderStartConnectionLength(spanIdx,gdrIdx);

   Float64 dist_from_brg_to_poi = dist_from_start_of_girder - dist_to_start_brg;

   Float64 slab_offset = ::LinInterp(dist_from_brg_to_poi,Astart,Aend,span_length);

   return slab_offset;
}

bool CBridgeAgentImp::IsCompositeDeck()
{
   pgsTypes::SupportedDeckType deckType = GetDeckType();

   return ( deckType == pgsTypes::sdtCompositeCIP ||
            deckType == pgsTypes::sdtCompositeSIP ||
            deckType == pgsTypes::sdtCompositeOverlay );
}

bool CBridgeAgentImp::HasOverlay()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();

   return (pDeck->WearingSurface == pgsTypes::wstFutureOverlay || pDeck->WearingSurface == pgsTypes::wstOverlay);
}

bool CBridgeAgentImp::IsFutureOverlay()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();

   return (pDeck->WearingSurface == pgsTypes::wstFutureOverlay);
}

Float64 CBridgeAgentImp::GetOverlayWeight()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();

   return pDeck->OverlayWeight;
}

Float64 CBridgeAgentImp::GetOverlayDepth()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();

   Float64 depth = 0;
   if ( pDeck->bInputAsDepthAndDensity )
   {
      depth = pDeck->OverlayDepth;
   }
   else
   {
      // depth not explicitly input... estimate based on 140 pcf material
      Float64 g = unitSysUnitsMgr::GetGravitationalAcceleration();
      depth = pDeck->OverlayWeight / (g*::ConvertToSysUnits(140.0,unitMeasure::LbfPerFeet3));
   }

   return depth;
}

Float64 CBridgeAgentImp::GetFillet()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();
   if ( pDeck->DeckType == pgsTypes::sdtNone )
      return 0.0;
   else
      return pDeck->Fillet;
}

Float64 CBridgeAgentImp::GetGrossSlabDepth(const pgsPointOfInterest& poi)
{
   VALIDATE( BRIDGE );
   return GetGrossSlabDepth();
}

Float64 CBridgeAgentImp::GetStructuralSlabDepth(const pgsPointOfInterest& poi)
{
   VALIDATE( BRIDGE );

   if ( GetDeckType() == pgsTypes::sdtNone )
      return 0.00;

   CComPtr<IBridgeDeck> deck;
   m_Bridge->get_Deck(&deck);

   Float64 structural_depth;
   deck->get_StructuralDepth(&structural_depth);

   return structural_depth;
}

Float64 CBridgeAgentImp::GetCastSlabDepth(const pgsPointOfInterest& poi)
{
   VALIDATE( BRIDGE );
   Float64 cast_depth = GetCastDepth();
   return cast_depth;
}

Float64 CBridgeAgentImp::GetPanelDepth(const pgsPointOfInterest& poi)
{
   VALIDATE( BRIDGE );
   Float64 panel_depth = GetPanelDepth();
   return panel_depth;
}

Float64 CBridgeAgentImp::GetLeftSlabOverhang(Float64 distFromStartOfBridge)
{
   VALIDATE( BRIDGE );

   if ( GetDeckType() == pgsTypes::sdtNone )
   {
      Float64 distFromStartOfSpan;
      SpanIndexType spanIdx;
      GirderIndexType gdrIdx = 0;
      GetDistFromStartOfSpan(gdrIdx,distFromStartOfBridge,&spanIdx,&distFromStartOfSpan);
      ATLASSERT(spanIdx != INVALID_INDEX);

      Float64 brgOffset  = GetGirderStartBearingOffset(spanIdx,gdrIdx);
      Float64 end_offset = GetGirderStartConnectionLength(spanIdx,gdrIdx);
      Float64 distFromStartOfGirder = distFromStartOfSpan - brgOffset + end_offset;

      pgsPointOfInterest poi(spanIdx,gdrIdx,distFromStartOfGirder);

      Float64 top_width = GetTopWidth(poi);
      return top_width/2;
   }

   Float64 left, right;
   HRESULT hr = GetSlabOverhangs(distFromStartOfBridge,&left,&right);
   ATLASSERT(hr == S_OK);
   return left;
}

Float64 CBridgeAgentImp::GetRightSlabOverhang(Float64 distFromStartOfBridge)
{
   VALIDATE( BRIDGE );

   if ( GetDeckType() == pgsTypes::sdtNone )
   {
      Float64 distFromStartOfSpan;
      SpanIndexType spanIdx;
      GirderIndexType gdrIdx = INVALID_INDEX; // use the right most girder
      GetDistFromStartOfSpan(gdrIdx,distFromStartOfBridge,&spanIdx,&distFromStartOfSpan);
      ATLASSERT(spanIdx != INVALID_INDEX);

      // now that we know the actual span, get the right most girder index
      gdrIdx = GetGirderCount(spanIdx) - 1;

      Float64 brgOffset  = GetGirderStartBearingOffset(spanIdx,gdrIdx);
      Float64 end_offset = GetGirderStartConnectionLength(spanIdx,gdrIdx);
      Float64 distFromStartOfGirder = distFromStartOfSpan - brgOffset + end_offset;

      pgsPointOfInterest poi(spanIdx,gdrIdx,distFromStartOfGirder);

      Float64 top_width = GetTopWidth(poi);
      return top_width/2;
   }

   Float64 left, right;
   HRESULT hr = GetSlabOverhangs(distFromStartOfBridge,&left,&right);
   ATLASSERT(hr == S_OK);
   return right;
}

Float64 CBridgeAgentImp::GetLeftSlabOverhang(SpanIndexType span,Float64 distFromStartOfSpan)
{
   VALIDATE( BRIDGE );
   Float64 distFromStartOfBridge = GetDistanceFromStartOfBridge(span,0,distFromStartOfSpan);
   return GetLeftSlabOverhang(distFromStartOfBridge);
}

Float64 CBridgeAgentImp::GetRightSlabOverhang(SpanIndexType span,Float64 distFromStartOfSpan)
{
   VALIDATE( BRIDGE );
   GirderIndexType nGirders = GetGirderCount(span);
   Float64 distFromStartOfBridge = GetDistanceFromStartOfBridge(span,nGirders-1,distFromStartOfSpan);
   return GetRightSlabOverhang(distFromStartOfBridge);
}

Float64 CBridgeAgentImp::GetLeftSlabGirderOverhang(SpanIndexType span,Float64 distFromLeftPier)
{
   VALIDATE( BRIDGE );

   if ( GetDeckType() == pgsTypes::sdtNone )
      return 0.0;

   Float64 pier_station = GetPierStation(span);
   Float64 station = distFromLeftPier + pier_station;

   // measure normal to the alignment
   Float64 overhang;
   HRESULT hr = m_BridgeGeometryTool->DeckOverhang(m_Bridge,station,NULL,qcbLeft,&overhang);
   ATLASSERT( SUCCEEDED(hr) );

   return overhang;
}

Float64 CBridgeAgentImp::GetRightSlabGirderOverhang(SpanIndexType span,Float64 distFromLeftPier)
{
   if ( GetDeckType() == pgsTypes::sdtNone )
      return 0.0;

   VALIDATE( BRIDGE );

   Float64 pier_station = GetPierStation(span);
   Float64 station = distFromLeftPier + pier_station;
   
   // measure normal to the alignment
   Float64 overhang;
   HRESULT hr = m_BridgeGeometryTool->DeckOverhang(m_Bridge,station,NULL,qcbRight,&overhang);
   ATLASSERT( SUCCEEDED(hr) );

   return overhang;
}

Float64 CBridgeAgentImp::GetLeftSlabOverhang(PierIndexType pier)
{
   VALIDATE( BRIDGE );

   if ( GetDeckType() == pgsTypes::sdtNone )
      return 0.0;

   Float64 pier_station = GetPierStation(pier);
//   CComPtr<IDirection> dir;
//   GetPierDirection(pier,&dir);
   
   Float64 overhang;
//   HRESULT hr = m_BridgeGeometryTool->DeckOverhang(m_Bridge,pier_station,dir,qcbLeft,&overhang);
   HRESULT hr = m_BridgeGeometryTool->DeckOverhang(m_Bridge,pier_station,NULL,qcbLeft,&overhang);
   ATLASSERT( SUCCEEDED(hr) );

   return overhang;
}

Float64 CBridgeAgentImp::GetRightSlabOverhang(PierIndexType pier)
{
   VALIDATE( BRIDGE );

   if ( GetDeckType() == pgsTypes::sdtNone )
      return 0.0;

   Float64 pier_station = GetPierStation(pier);
//   CComPtr<IDirection> dir;
//   GetPierDirection(pier,&dir);
   
   Float64 overhang;
//   HRESULT hr = m_BridgeGeometryTool->DeckOverhang(m_Bridge,pier_station,dir,qcbRight,&overhang);
   HRESULT hr = m_BridgeGeometryTool->DeckOverhang(m_Bridge,pier_station,NULL,qcbRight,&overhang);
   ATLASSERT( SUCCEEDED(hr) );

   return overhang;
}

Float64 CBridgeAgentImp::GetLeftSlabEdgeOffset(PierIndexType pier)
{
   VALIDATE( BRIDGE );
   Float64 start_station = GetPierStation(0);
   Float64 pier_station  = GetPierStation(pier);
   Float64 dist_from_start_of_bridge = pier_station - start_station;
   return GetLeftSlabEdgeOffset(dist_from_start_of_bridge);
}

Float64 CBridgeAgentImp::GetRightSlabEdgeOffset(PierIndexType pier)
{
   VALIDATE( BRIDGE );
   Float64 start_station = GetPierStation(0);
   Float64 pier_station  = GetPierStation(pier);
   Float64 dist_from_start_of_bridge = pier_station - start_station;
   return GetRightSlabEdgeOffset(dist_from_start_of_bridge);
}

Float64 CBridgeAgentImp::GetLeftSlabEdgeOffset(Float64 distFromStartOfBridge)
{
   std::map<Float64,Float64>::iterator found = m_LeftSlabEdgeOffset.find(distFromStartOfBridge);
   if ( found != m_LeftSlabEdgeOffset.end() )
      return found->second;

   VALIDATE( BRIDGE );
   Float64 station = GetPierStation(0);
   station += distFromStartOfBridge;
   Float64 offset;
   m_BridgeGeometryTool->DeckOffset(m_Bridge,station,NULL,qcbLeft,&offset);

   m_LeftSlabEdgeOffset.insert(std::make_pair(distFromStartOfBridge,offset));

   return offset;
}

Float64 CBridgeAgentImp::GetRightSlabEdgeOffset(Float64 distFromStartOfBridge)
{
   std::map<Float64,Float64>::iterator found = m_RightSlabEdgeOffset.find(distFromStartOfBridge);
   if ( found != m_RightSlabEdgeOffset.end() )
      return found->second;

   VALIDATE( BRIDGE );
   Float64 station = GetPierStation(0);
   station += distFromStartOfBridge;
   Float64 offset;
   m_BridgeGeometryTool->DeckOffset(m_Bridge,station,NULL,qcbRight,&offset);

   m_RightSlabEdgeOffset.insert(std::make_pair(distFromStartOfBridge,offset));

   return offset;
}

Float64 CBridgeAgentImp::GetLeftCurbOffset(Float64 distFromStartOfBridge)
{
   VALIDATE( BRIDGE );
   Float64 station = GetPierStation(0);
   station += distFromStartOfBridge;
   Float64 offset;
   m_BridgeGeometryTool->CurbOffset(m_Bridge,station,NULL,qcbLeft,&offset);
   return offset;
}

Float64 CBridgeAgentImp::GetRightCurbOffset(Float64 distFromStartOfBridge)
{
   VALIDATE( BRIDGE );
   Float64 station = GetPierStation(0);
   station += distFromStartOfBridge;
   Float64 offset;
   m_BridgeGeometryTool->CurbOffset(m_Bridge,station,NULL,qcbRight,&offset);
   return offset;
}

Float64 CBridgeAgentImp::GetLeftCurbOffset(SpanIndexType span,Float64 distFromStartOfSpan)
{
   Float64 distFromStartOfBridge = GetDistanceFromStartOfBridge(span,0,distFromStartOfSpan);
   return GetLeftCurbOffset(distFromStartOfBridge);
}

Float64 CBridgeAgentImp::GetRightCurbOffset(SpanIndexType span,Float64 distFromStartOfSpan)
{
   GirderIndexType nGirders = GetGirderCount(span);
   Float64 distFromStartOfBridge = GetDistanceFromStartOfBridge(span,nGirders-1,distFromStartOfSpan);
   return GetRightCurbOffset(distFromStartOfBridge);
}

Float64 CBridgeAgentImp::GetLeftCurbOffset(PierIndexType pier)
{
   Float64 start_station = GetPierStation(0);
   Float64 pier_station = GetPierStation(pier);
   Float64 distFromStartOfBridge = pier_station - start_station;
   return GetLeftCurbOffset(distFromStartOfBridge);
}

Float64 CBridgeAgentImp::GetRightCurbOffset(PierIndexType pier)
{
   Float64 start_station = GetPierStation(0);
   Float64 pier_station = GetPierStation(pier);
   Float64 distFromStartOfBridge = pier_station - start_station;
   return GetRightCurbOffset(distFromStartOfBridge);
}

Float64 CBridgeAgentImp::GetCurbToCurbWidth(const pgsPointOfInterest& poi)
{
   Float64 distFromStartOfBridge = GetDistanceFromStartOfBridge(poi);
   return GetCurbToCurbWidth(distFromStartOfBridge);
}

Float64 CBridgeAgentImp::GetCurbToCurbWidth(SpanIndexType span,GirderIndexType gdr,Float64 distFromStartOfSpan)
{
   Float64 distFromStartOfBridge = GetDistanceFromStartOfBridge(span,gdr,distFromStartOfSpan);
   return GetCurbToCurbWidth(distFromStartOfBridge);
}

Float64 CBridgeAgentImp::GetCurbToCurbWidth(Float64 distFromStartOfBridge)
{
   Float64 left_offset, right_offset;
   left_offset = GetLeftCurbOffset(distFromStartOfBridge);
   right_offset = GetRightCurbOffset(distFromStartOfBridge);
   return right_offset - left_offset;
}

Float64 CBridgeAgentImp::GetLeftInteriorCurbOffset(double distFromStartOfBridge)
{
   VALIDATE( BRIDGE );
   Float64 station = GetPierStation(0);
   station += distFromStartOfBridge;
   Float64 offset;
   m_BridgeGeometryTool->InteriorCurbOffset(m_Bridge,station,NULL,qcbLeft,&offset);
   return offset;
}

Float64 CBridgeAgentImp::GetRightInteriorCurbOffset(double distFromStartOfBridge)
{
   VALIDATE( BRIDGE );
   Float64 station = GetPierStation(0);
   station += distFromStartOfBridge;
   Float64 offset;
   m_BridgeGeometryTool->InteriorCurbOffset(m_Bridge,station,NULL,qcbRight,&offset);
   return offset;
}

Float64 CBridgeAgentImp::GetLeftOverlayToeOffset(double distFromStartOfBridge)
{
   Float64 slab_edge = GetLeftSlabEdgeOffset(distFromStartOfBridge);

   CComPtr<ISidewalkBarrier> pSwBarrier;
   m_Bridge->get_LeftBarrier(&pSwBarrier);

   Float64 toe_width;
   pSwBarrier->get_OverlayToeWidth(&toe_width);

   return slab_edge + toe_width;
}

Float64 CBridgeAgentImp::GetRightOverlayToeOffset(double distFromStartOfBridge)
{
   Float64 slab_edge = GetRightSlabEdgeOffset(distFromStartOfBridge);

   CComPtr<ISidewalkBarrier> pSwBarrier;
   m_Bridge->get_RightBarrier(&pSwBarrier);

   Float64 toe_width;
   pSwBarrier->get_OverlayToeWidth(&toe_width);

   return slab_edge - toe_width;
}

Float64 CBridgeAgentImp::GetLeftOverlayToeOffset(const pgsPointOfInterest& poi)
{
   Float64 distFromStartOfBridge = GetDistanceFromStartOfBridge(poi);
   return GetLeftOverlayToeOffset(distFromStartOfBridge);
}

Float64 CBridgeAgentImp::GetRightOverlayToeOffset(const pgsPointOfInterest& poi)
{
   Float64 distFromStartOfBridge = GetDistanceFromStartOfBridge(poi);
   return GetRightOverlayToeOffset(distFromStartOfBridge);
}

void CBridgeAgentImp::GetSlabPerimeter(CollectionIndexType nPoints,IPoint2dCollection** points)
{
   VALIDATE( BRIDGE );

   CComPtr<IPoint2dCollection> left_edge, right_edge;
   m_BridgeGeometryTool->DeckEdgePoints(m_Bridge,qcbLeft, nPoints,&left_edge);
   m_BridgeGeometryTool->DeckEdgePoints(m_Bridge,qcbRight,nPoints,&right_edge);

   // append the left edge to the right edge, in reverse order
   CollectionIndexType count;
   left_edge->get_Count(&count);
   bool bDone = false;
   CollectionIndexType i = count-1;
   do
   {
      CComPtr<IPoint2d> p;
      left_edge->get_Item(i,&p);
      right_edge->Add(p);

      if ( i == 0 )
         bDone = true;

      i--;
   } while ( !bDone );

   (*points) = right_edge;
   (*points)->AddRef();
}

void CBridgeAgentImp::GetSlabPerimeter(SpanIndexType startSpanIdx,SpanIndexType endSpanIdx,CollectionIndexType nPoints,IPoint2dCollection** points)
{
   VALIDATE( BRIDGE );

   CComPtr<IAlignment> alignment;
   GetAlignment(&alignment);

   ASSERT( 3 <= nPoints );

   CComPtr<IPoint2dCollection> thePoints;
   thePoints.CoCreateInstance(CLSID_Point2dCollection);

   PierIndexType startPierIdx = startSpanIdx;
   PierIndexType endPierIdx   = endSpanIdx+1;

   Float64 startStation = GetPierStation(startPierIdx);
   Float64 endStation   = GetPierStation(endPierIdx);
   Float64 stationInc   = (endStation - startStation)/(nPoints-1);

   CComPtr<IDirection> startDirection, endDirection;
   GetPierDirection(startPierIdx,&startDirection);
   GetPierDirection(endPierIdx,  &endDirection);
   Float64 dirStart, dirEnd;
   startDirection->get_Value(&dirStart);
   endDirection->get_Value(&dirEnd);

   Float64 station   = startStation;

   // Locate points along right side of deck
   // Get station of deck points at first and last piers, projected normal to aligment
   CComPtr<IPoint2d> objStartPointRight, objEndPointRight;
   m_BridgeGeometryTool->DeckEdgePoint(m_Bridge,startStation,startDirection,qcbRight,&objStartPointRight);
   m_BridgeGeometryTool->DeckEdgePoint(m_Bridge,endStation,  endDirection,  qcbRight,&objEndPointRight);

   Float64 start_normal_station_right, end_normal_station_right;
   Float64 offset;

   GetStationAndOffset(objStartPointRight,&start_normal_station_right,&offset);
   GetStationAndOffset(objEndPointRight,  &end_normal_station_right,  &offset);

   // If there is a skew, the first deck edge can be before the pier station, or after it. 
   // Same for the last deck edge. We must deal with this
   thePoints->Add(objStartPointRight);

   CollectionIndexType pntIdx;
   for (pntIdx = 0; pntIdx < nPoints; pntIdx++ )
   {
      if (station > start_normal_station_right && station < end_normal_station_right)
      {
         CComPtr<IDirection> objDirection;
         alignment->Normal(CComVariant(station), &objDirection);

         CComPtr<IPoint2d> point;
         HRESULT hr = m_BridgeGeometryTool->DeckEdgePoint(m_Bridge,station,objDirection,qcbRight,&point);
         ATLASSERT( SUCCEEDED(hr) );

         thePoints->Add(point);
      }

      station   += stationInc;
   }

   thePoints->Add(objEndPointRight);

   // Locate points along left side of deck (working backwards)
   CComPtr<IPoint2d> objStartPointLeft, objEndPointLeft;
   m_BridgeGeometryTool->DeckEdgePoint(m_Bridge,startStation,startDirection,qcbLeft,&objStartPointLeft);
   m_BridgeGeometryTool->DeckEdgePoint(m_Bridge,endStation,  endDirection,  qcbLeft,&objEndPointLeft);

   Float64 start_normal_station_left, end_normal_station_left;

   GetStationAndOffset(objStartPointLeft,&start_normal_station_left,&offset);
   GetStationAndOffset(objEndPointLeft,  &end_normal_station_left,  &offset);

   thePoints->Add(objEndPointLeft);

   station   = endStation;
   for ( pntIdx = 0; pntIdx < nPoints; pntIdx++ )
   {
      if (station > start_normal_station_left && station < end_normal_station_left)
      {
         CComPtr<IDirection> objDirection;
         alignment->Normal(CComVariant(station), &objDirection);

         CComPtr<IPoint2d> point;
         HRESULT hr = m_BridgeGeometryTool->DeckEdgePoint(m_Bridge,station,objDirection,qcbLeft,&point);
         ATLASSERT( SUCCEEDED(hr) );

         thePoints->Add(point);
      }

      station   -= stationInc;
   }

   thePoints->Add(objStartPointLeft);

   (*points) = thePoints;
   (*points)->AddRef();
}

void CBridgeAgentImp::GetSpanPerimeter(SpanIndexType spanIdx,CollectionIndexType nPoints,IPoint2dCollection** points)
{
   VALIDATE( BRIDGE );

   CComPtr<IAlignment> alignment;
   GetAlignment(&alignment);

   ASSERT( 3 <= nPoints );

   CComPtr<IPoint2dCollection> thePoints;
   thePoints.CoCreateInstance(CLSID_Point2dCollection);

   PierIndexType startPierIdx = spanIdx;
   PierIndexType endPierIdx   = spanIdx+1;

   Float64 startStation = GetPierStation(startPierIdx);
   Float64 endStation   = GetPierStation(endPierIdx);
   Float64 stationInc   = (endStation - startStation)/(nPoints-1);

   CComPtr<IDirection> startDirection, endDirection;
   GetPierDirection(startPierIdx,&startDirection);
   GetPierDirection(endPierIdx,  &endDirection);
   Float64 dirStart, dirEnd;
   startDirection->get_Value(&dirStart);
   endDirection->get_Value(&dirEnd);

   Float64 station   = startStation;

   // Locate points along right side of deck
   // Get station of deck points at first and last piers, projected normal to aligment
   CComPtr<IPoint2d> objStartPointRight, objEndPointRight;
   m_BridgeGeometryTool->DeckEdgePoint(m_Bridge,startStation,startDirection,qcbRight,&objStartPointRight);
   m_BridgeGeometryTool->DeckEdgePoint(m_Bridge,endStation,  endDirection,  qcbRight,&objEndPointRight);

   Float64 start_normal_station_right, end_normal_station_right;
   Float64 offset;

   GetStationAndOffset(objStartPointRight,&start_normal_station_right,&offset);
   GetStationAndOffset(objEndPointRight,  &end_normal_station_right,  &offset);

   // If there is a skew, the first deck edge can be before the pier station, or after it. 
   // Same for the last deck edge. We must deal with this
   thePoints->Add(objStartPointRight);

   CollectionIndexType pntIdx;
   for (pntIdx = 0; pntIdx < nPoints; pntIdx++ )
   {
      if (station > start_normal_station_right && station < end_normal_station_right)
      {
         CComPtr<IDirection> objDirection;
         alignment->Normal(CComVariant(station), &objDirection);

         CComPtr<IPoint2d> point;
         HRESULT hr = m_BridgeGeometryTool->DeckEdgePoint(m_Bridge,station,objDirection,qcbRight,&point);
         ATLASSERT( SUCCEEDED(hr) );

         thePoints->Add(point);
      }

      station   += stationInc;
   }

   thePoints->Add(objEndPointRight);

   // Locate points along left side of deck (working backwards)
   CComPtr<IPoint2d> objStartPointLeft, objEndPointLeft;
   m_BridgeGeometryTool->DeckEdgePoint(m_Bridge,startStation,startDirection,qcbLeft,&objStartPointLeft);
   m_BridgeGeometryTool->DeckEdgePoint(m_Bridge,endStation,  endDirection,  qcbLeft,&objEndPointLeft);

   Float64 start_normal_station_left, end_normal_station_left;

   GetStationAndOffset(objStartPointLeft,&start_normal_station_left,&offset);
   GetStationAndOffset(objEndPointLeft,  &end_normal_station_left,  &offset);

   thePoints->Add(objEndPointLeft);

   station   = endStation;
   for ( pntIdx = 0; pntIdx < nPoints; pntIdx++ )
   {
      if (station > start_normal_station_left && station < end_normal_station_left)
      {
         CComPtr<IDirection> objDirection;
         alignment->Normal(CComVariant(station), &objDirection);

         CComPtr<IPoint2d> point;
         HRESULT hr = m_BridgeGeometryTool->DeckEdgePoint(m_Bridge,station,objDirection,qcbLeft,&point);
         ATLASSERT( SUCCEEDED(hr) );

         thePoints->Add(point);
      }

      station   -= stationInc;
   }

   thePoints->Add(objStartPointLeft);

   (*points) = thePoints;
   (*points)->AddRef();
}

void CBridgeAgentImp::GetLeftSlabEdgePoint(Float64 station, IDirection* direction,IPoint2d** point)
{
   GetSlabEdgePoint(station,direction,qcbLeft,point);
}

void CBridgeAgentImp::GetLeftSlabEdgePoint(Float64 station, IDirection* direction,IPoint3d** point)
{
   GetSlabEdgePoint(station,direction,qcbLeft,point);
}

void CBridgeAgentImp::GetRightSlabEdgePoint(Float64 station, IDirection* direction,IPoint2d** point)
{
   GetSlabEdgePoint(station,direction,qcbRight,point);
}

void CBridgeAgentImp::GetRightSlabEdgePoint(Float64 station, IDirection* direction,IPoint3d** point)
{
   GetSlabEdgePoint(station,direction,qcbRight,point);
}

Float64 CBridgeAgentImp::GetPierStation(PierIndexType pierIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IPierCollection> piers;
   m_Bridge->get_Piers(&piers);

   CComPtr<IPier> pier;
   piers->get_Item(pierIdx,&pier);

   CComPtr<IStation> station;
   pier->get_Station(&station);

   Float64 value;
   station->get_Value(&value);

   return value;
}

Float64 CBridgeAgentImp::GetAheadBearingStation(PierIndexType pierIdx,GirderIndexType gdrIdx)
{
   // returns the station, where a line projected from the intersection of the
   // centerline of bearing and the centerline of girder, intersects the alignment
   // at a right angle
   VALIDATE( BRIDGE );
   Float64 pier_station = GetPierStation(pierIdx); // station where pier hits alignment
   Float64 offset = 0;

   SpanIndexType spanIdx = pierIdx; // the "ahead" span has the same index as this pier

   // if this is not the last pier, get the offset
   // if this is the last pier there is no "ahead" side
   if ( pierIdx != GetPierCount()-1 )
      offset = GetGirderStartBearingOffset(spanIdx,gdrIdx);

   Float64 brg_station = pier_station + offset; // station where bearing hits alignment

   CComPtr<ISpanCollection> spans;
   m_Bridge->get_Spans(&spans);

   CComPtr<ISpan> span;
   spans->get_Item(spanIdx,&span);
   
   GirderIndexType nGirders;
   span->get_GirderCount(&nGirders);


   CComPtr<IGirderSpacing> girderSpacing;
   span->get_GirderSpacing(etStart,&girderSpacing);
   Float64 girder_offset;
   girderSpacing->get_GirderOffset(gdrIdx,mlCenterlineBearing,mtNormal,&girder_offset);

   Float64 alignment_offset;
   m_Bridge->get_AlignmentOffset(&alignment_offset);

   CComPtr<IAngle> skew;
   GetPierSkew(pierIdx,&skew);
   Float64 value;
   skew->get_Value(&value);

   Float64 station = brg_station + (alignment_offset + girder_offset)*tan(value);

   return station;
}

Float64 CBridgeAgentImp::GetBackBearingStation(PierIndexType pierIdx,GirderIndexType gdrIdx)
{
   // returns the station, where a line projected from the intersection of the
   // centerline of bearing and the centerline of girder, intersects the alignment
   // at a right angle
   VALIDATE( BRIDGE );
   Float64 pier_station = GetPierStation(pierIdx); // station where pier hits alignment
   Float64 offset = 0;

   SpanIndexType spanIdx = (pierIdx == 0 ? INVALID_INDEX : pierIdx-1); // the "back" span has an index one less than this pier
   if ( spanIdx == INVALID_INDEX )
      spanIdx = 0;

   // if this is not the first pier, get the offset
   // if this is the first pier there is no "back" side
   if ( pierIdx != 0 )
      offset = GetGirderEndBearingOffset(spanIdx,gdrIdx);

   Float64 brg_station = pier_station - offset; // station where bearing hits alignment

   CComPtr<ISpanCollection> spans;
   m_Bridge->get_Spans(&spans);

   CComPtr<ISpan> span;
   spans->get_Item(spanIdx,&span);
   
   GirderIndexType nGirders;
   span->get_GirderCount(&nGirders);


   CComPtr<IGirderSpacing> girderSpacing;
   span->get_GirderSpacing(etEnd,&girderSpacing);
   Float64 girder_offset;
   girderSpacing->get_GirderOffset(gdrIdx,mlCenterlineBearing,mtNormal,&girder_offset);


   Float64 alignment_offset;
   m_Bridge->get_AlignmentOffset(&alignment_offset);


   CComPtr<IAngle> skew;
   GetPierSkew(pierIdx,&skew);
   Float64 value;
   skew->get_Value(&value);

   Float64 station = brg_station + (alignment_offset + girder_offset)*tan(value);

   return station;
}

void CBridgeAgentImp::GetPierDirection(PierIndexType pierIdx,IDirection** ppDirection)
{
   VALIDATE( BRIDGE );

   CComPtr<IPierCollection> piers;
   m_Bridge->get_Piers(&piers);

   CComPtr<IPier> pier;
   piers->get_Item(pierIdx,&pier);

   pier->get_Direction(ppDirection);
}

void CBridgeAgentImp::GetPierSkew(PierIndexType pierIdx,IAngle** ppAngle)
{
   VALIDATE( BRIDGE );

   CComPtr<IPierCollection> piers;
   m_Bridge->get_Piers(&piers);

   CComPtr<IPier> pier;
   piers->get_Item(pierIdx,&pier);

   pier->get_SkewAngle(ppAngle);
}

std::_tstring CBridgeAgentImp::GetLeftSidePierConnection(PierIndexType pierIdx)
{
   VALIDATE( BRIDGE );

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData* pPierData = pBridgeDesc->GetPier(pierIdx);
   ATLASSERT( pPierData );

   if ( pPierData->GetPrevSpan() == NULL )
      return pPierData->GetConnectionLibraryEntry(pgsTypes::Ahead)->GetName();
   else
      return pPierData->GetConnectionLibraryEntry(pgsTypes::Back)->GetName();
}

std::_tstring CBridgeAgentImp::GetRightSidePierConnection(PierIndexType pierIdx)
{
   VALIDATE( BRIDGE );
   
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData* pPierData = pBridgeDesc->GetPier(pierIdx);
   ATLASSERT( pPierData );

   if ( pPierData->GetNextSpan() == NULL )
      return pPierData->GetConnectionLibraryEntry(pgsTypes::Back)->GetName();
   else
      return pPierData->GetConnectionLibraryEntry(pgsTypes::Ahead)->GetName();
}

void CBridgeAgentImp::GetPierPoints(PierIndexType pier,IPoint2d** left,IPoint2d** alignment,IPoint2d** bridge,IPoint2d** right)
{
   VALIDATE( BRIDGE );
   CComPtr<ICogoInfo> cogoInfo;
   m_Bridge->get_CogoInfo(&cogoInfo);

   CComPtr<ICogoModel> cogoModel;
   m_Bridge->get_CogoModel(&cogoModel);

   CComPtr<IPointCollection> points;
   cogoModel->get_Points(&points);

   CogoElementKey id;
   cogoInfo->get_PierPointID(pier,pptLeft,&id);
   points->get_Item(id,left);

   cogoInfo->get_PierPointID(pier,pptAlignment,&id);
   points->get_Item(id,alignment);

   cogoInfo->get_PierPointID(pier,pptBridge,&id);
   points->get_Item(id,bridge);

   cogoInfo->get_PierPointID(pier,pptRight,&id);
   points->get_Item(id,right);
}

void CBridgeAgentImp::IsContinuousAtPier(PierIndexType pierIdx,bool* pbLeft,bool* pbRight)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData* pPierData = pBridgeDesc->GetPier(pierIdx);
   ATLASSERT( pPierData );

   *pbLeft = pPierData->IsContinuous();
   *pbRight = *pbLeft;
}

void CBridgeAgentImp::IsIntegralAtPier(PierIndexType pierIdx,bool* pbLeft,bool* pbRight)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData* pPierData = pBridgeDesc->GetPier(pierIdx);
   ATLASSERT( pPierData );

   pPierData->IsIntegral(pbLeft,pbRight);
}

void CBridgeAgentImp::GetContinuityStage(PierIndexType pierIdx,pgsTypes::Stage* pLeft,pgsTypes::Stage* pRight)
{
   VALIDATE( BRIDGE );

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData* pPierData = pBridgeDesc->GetPier(pierIdx);
   ATLASSERT( pPierData );

   *pLeft  = (pPierData->GetConnectionType() == pgsTypes::ContinuousBeforeDeck || pPierData->GetConnectionType() == pgsTypes::IntegralBeforeDeck)  ? pgsTypes::BridgeSite1 : pgsTypes::BridgeSite2;
   *pRight = *pLeft;
}

bool CBridgeAgentImp::GetSkewAngle(Float64 station,LPCTSTR strOrientation,Float64* pSkew)
{
   VALIDATE( COGO_MODEL );

   CComPtr<IAlignment> alignment;
   GetAlignment(&alignment);

   std::_tstring orientation(strOrientation);
   std::transform(orientation.begin(),orientation.end(),orientation.begin(),(int(*)(int))std::toupper);

   if ( orientation.compare(_T("N")) == 0 || orientation.compare(_T("NORMAL")) == 0 )
   {
      // normal to alignment
      *pSkew = 0.0;
   }
   else if ( orientation[0] == _T('N') || orientation[0] == _T('S') )
   {
      // defined by bearing
      CComPtr<IDirection> brg;
      brg.CoCreateInstance(CLSID_Direction);
      HRESULT hr = brg->FromString(CComBSTR(strOrientation));
      if ( FAILED(hr) )
         return false;

      CComPtr<IDirection> normal;
      alignment->Normal(CComVariant(station),&normal);

      CComPtr<IAngle> angle;
      brg->AngleBetween(normal,&angle);

      Float64 value;
      angle->get_Value(&value);

      while ( PI_OVER_2 < value )
         value -= M_PI;

      *pSkew = value;
   }
   else
   {
      // defined by skew angle
      CComPtr<IAngle> angle;
      angle.CoCreateInstance(CLSID_Angle);
      HRESULT hr = angle->FromString(CComBSTR(strOrientation));
      if ( FAILED(hr) )
         return false;

      Float64 value;
      angle->get_Value(&value);
      *pSkew = value;
   }

   return true;
}

bool CBridgeAgentImp::ProcessNegativeMoments(SpanIndexType spanIdx)
{
   // don't need to process negative moments if this is a simple span design
   // or if there isn't any continuity
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   if ( analysisType == pgsTypes::Simple )
      return false;

   PierIndexType startPier = (spanIdx == ALL_SPANS ? 0 : spanIdx);
   PierIndexType endPier   = (spanIdx == ALL_SPANS ? GetSpanCount_Private() : spanIdx+1);

   for ( PierIndexType pier = startPier; pier <= endPier; pier++ )
   {
      bool bContinuousLeft,bContinuousRight;
      IsContinuousAtPier(pier,&bContinuousLeft,&bContinuousRight);

      bool bIntegralLeft,bIntegralRight;
      IsIntegralAtPier(pier,&bIntegralLeft,&bIntegralRight);

      if ( bContinuousLeft || bContinuousRight || bIntegralLeft || bIntegralRight )
         return true;
   }

   return false;
}

/////////////////////////////////////////////////////////////////////////
// IBridgeMaterial
//
Float64 CBridgeAgentImp::GetEcGdr(SpanIndexType span,GirderIndexType gdr)
{
   VALIDATE( CONCRETE );

   SpanGirderHashType key = HashSpanGirder(span,gdr);
   Float64 Ec = m_pGdrConc[key]->GetE();
   return Ec;
}

Float64 CBridgeAgentImp::GetEciGdr(SpanIndexType span,GirderIndexType gdr)
{
   VALIDATE( CONCRETE );

   SpanGirderHashType key = HashSpanGirder(span,gdr);
   Float64 Eci = m_pGdrReleaseConc[key]->GetE();
   return Eci;
}

Float64 CBridgeAgentImp::GetFcGdr(SpanIndexType span,GirderIndexType gdr)
{
   VALIDATE( CONCRETE );
   SpanGirderHashType key = HashSpanGirder(span,gdr);
   return m_pGdrConc[key]->GetFc();
}

Float64 CBridgeAgentImp::GetFciGdr(SpanIndexType span,GirderIndexType gdr)
{
   VALIDATE( CONCRETE );
   SpanGirderHashType key = HashSpanGirder(span,gdr);
   return m_pGdrReleaseConc[key]->GetFc();
}

Float64 CBridgeAgentImp::GetMaxAggrSizeGdr(SpanIndexType span,GirderIndexType gdr)
{
   VALIDATE( CONCRETE );
   SpanGirderHashType key = HashSpanGirder(span,gdr);
   return m_pGdrConc[key]->GetMaxAggregateSize();
}

Float64 CBridgeAgentImp::GetK1Gdr(SpanIndexType span,GirderIndexType gdr)
{
   ATLASSERT(false); // use the new version on IBridgeMaterialEx
   GET_IFACE(IGirderData,pGirderData);
   Float64 K1 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      const CGirderMaterial* pGdrMaterial = pGirderData->GetGirderMaterial(span,gdr);
      K1 *= pGdrMaterial->EcK1;
   }

   return K1;
}

Float64 CBridgeAgentImp::GetEcSlab()
{
   VALIDATE( CONCRETE );
   return m_pSlabConc->GetE();
}

Float64 CBridgeAgentImp::GetK1Slab()
{
   ATLASSERT(false); // use the new version on IBridgeMaterialEx
   VALIDATE( CONCRETE );
   Float64 K1 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      K1 *= m_SlabEcK1;
   }
   return K1;
}

Float64 CBridgeAgentImp::GetFcSlab()
{
   VALIDATE( CONCRETE );
   return m_pSlabConc->GetFc();
}

Float64 CBridgeAgentImp::GetStrDensityGdr(SpanIndexType span,GirderIndexType gdr)
{
   VALIDATE( CONCRETE );
   SpanGirderHashType key = HashSpanGirder(span,gdr);
   return m_pGdrConc[key]->GetDensity();
}

Float64 CBridgeAgentImp::GetWgtDensityGdr(SpanIndexType span,GirderIndexType gdr)
{
   VALIDATE( CONCRETE );
   SpanGirderHashType key = HashSpanGirder(span,gdr);
   return m_pGdrConc[key]->GetDensityForWeight();
}

Float64 CBridgeAgentImp::GetStrDensitySlab()
{
   VALIDATE( CONCRETE );
   return m_pSlabConc->GetDensity();
}

Float64 CBridgeAgentImp::GetWgtDensitySlab()
{
   VALIDATE( CONCRETE );
   return m_pSlabConc->GetDensityForWeight();
}

Float64 CBridgeAgentImp::GetMaxAggrSizeSlab()
{
   VALIDATE( CONCRETE );
   return m_pSlabConc->GetMaxAggregateSize();
}

Float64 CBridgeAgentImp::GetDensityRailing(pgsTypes::TrafficBarrierOrientation orientation)
{
   VALIDATE( CONCRETE );
   return m_pRailingConc[orientation]->GetDensityForWeight();
}

Float64 CBridgeAgentImp::GetEcRailing(pgsTypes::TrafficBarrierOrientation orientation)
{
   VALIDATE( CONCRETE );
   return m_pRailingConc[orientation]->GetE();
}

const matPsStrand* CBridgeAgentImp::GetStrand(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType)
{
   GET_IFACE(IGirderData,pGirderData);
   return pGirderData->GetStrandMaterial(span,gdr,strandType);
}


Float64 CBridgeAgentImp::GetEconc(Float64 fc,Float64 density,Float64 K1)
{
   ATLASSERT(false); // this method is obsolete 
   return K1*lrfdConcreteUtil::ModE(fc,density, false ); // ignore LRFD limits
}

Float64 CBridgeAgentImp::GetFlexureModRupture(Float64 fc)
{
   ATLASSERT(false); // this method is obsolete because of LWC
   // use IBridgeMaterialEx::GetFlexureModRupture(fc,concType);
   return lrfdConcreteUtil::ModRupture( fc, lrfdConcreteUtil::NormalDensity, GetFlexureFrCoefficient() );
}

Float64 CBridgeAgentImp::GetShearModRupture(Float64 fc)
{
   ATLASSERT(false); // this method is obsolete because of LWC
   // use IBridgeMaterialEx::GetShearModRupture(fc,concType);
   return lrfdConcreteUtil::ModRupture( fc, lrfdConcreteUtil::NormalDensity, GetShearFrCoefficient() );
}

Float64 CBridgeAgentImp::GetFlexureFrCoefficient()
{
   ATLASSERT(false); // this method is obsolete
   // use IBridgeMaterialEx::GetFlexureFrCoefficient(concType)
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   return pSpecEntry->GetFlexureModulusOfRuptureCoefficient(pgsTypes::Normal);
}

Float64 CBridgeAgentImp::GetShearFrCoefficient()
{
   ATLASSERT(false); // this method is obsolete
   // use IBridgeMaterialEx::GetShearFrCoefficient(concType)
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   return pSpecEntry->GetShearModulusOfRuptureCoefficient(pgsTypes::Normal);
}

Float64 CBridgeAgentImp::GetFlexureFrGdr(SpanIndexType span,GirderIndexType gdr)
{
   VALIDATE( CONCRETE );
   Float64 fc = GetFcGdr(span,gdr);
   pgsTypes::ConcreteType type = GetGdrConcreteType(span,gdr);
   return GetFlexureModRupture( fc, type );
}

Float64 CBridgeAgentImp::GetShearFrGdr(SpanIndexType span,GirderIndexType gdr)
{
   VALIDATE( CONCRETE );
   Float64 fc = GetFcGdr(span,gdr);
   pgsTypes::ConcreteType type = GetGdrConcreteType(span,gdr);
   return GetShearModRupture( fc, type );
}

Float64 CBridgeAgentImp::GetFriGdr(SpanIndexType span,GirderIndexType gdr)
{
   VALIDATE( CONCRETE );
   Float64 fci;
   fci = GetFciGdr(span, gdr);
   pgsTypes::ConcreteType type = GetGdrConcreteType(span,gdr);
   return GetFlexureModRupture( fci, type );
}

Float64 CBridgeAgentImp::GetFlexureFrSlab()
{
   VALIDATE( CONCRETE );
   pgsTypes::ConcreteType type = GetSlabConcreteType();
   return lrfdConcreteUtil::ModRupture( m_pSlabConc->GetFc(), lrfdConcreteUtil::DensityType(type), GetFlexureFrCoefficient(type) );
}

Float64 CBridgeAgentImp::GetShearFrSlab()
{
   VALIDATE( CONCRETE );
   pgsTypes::ConcreteType type = GetSlabConcreteType();
   return lrfdConcreteUtil::ModRupture( m_pSlabConc->GetFc(), lrfdConcreteUtil::DensityType(type), GetShearFrCoefficient(type) );
}

void CBridgeAgentImp::GetLongitudinalRebarProperties(SpanIndexType span,GirderIndexType gdr,Float64* pE,Float64 *pFy,Float64* pFu)
{
   GET_IFACE(ILongitudinalRebar,pLongRebar);
   CLongitudinalRebarData lrd = pLongRebar->GetLongitudinalRebarData(span,gdr);

   const matRebar* pRebar = lrfdRebarPool::GetInstance()->GetRebar(lrd.BarType,lrd.BarGrade,matRebar::bs3);
   *pE  = pRebar->GetE();
   *pFy = pRebar->GetYieldStrength();
   *pFu = pRebar->GetUltimateStrength();
}

std::_tstring CBridgeAgentImp::GetLongitudinalRebarName(SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE(ILongitudinalRebar,pLongRebar);
   return pLongRebar->GetLongitudinalRebarMaterial(span,gdr);
}

void CBridgeAgentImp::GetLongitudinalRebarMaterial(SpanIndexType spanIdx,GirderIndexType gdrIdx,matRebar::Type& type,matRebar::Grade& grade)
{
   GET_IFACE(ILongitudinalRebar,pLongRebar);
   pLongRebar->GetLongitudinalRebarMaterial(spanIdx,gdrIdx,type,grade);
}

void CBridgeAgentImp::GetTransverseRebarProperties(SpanIndexType span,GirderIndexType gdr,Float64* pE,Float64 *pFy,Float64* pFu)
{
	GET_IFACE(IShear,pShear);
	CShearData shear_data = pShear->GetShearData(span,gdr);
   const matRebar* pRebar = lrfdRebarPool::GetInstance()->GetRebar(shear_data.ShearBarType,shear_data.ShearBarGrade,matRebar::bs3);
   *pE  = pRebar->GetE();
   *pFy = pRebar->GetYieldStrength();
   *pFu = pRebar->GetUltimateStrength();
}

std::_tstring CBridgeAgentImp::GetTransverseRebarName(SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE(IShear,pShear);
   return pShear->GetStirrupMaterial(span,gdr);
}

void CBridgeAgentImp::GetTransverseRebarMaterial(SpanIndexType spanIdx,GirderIndexType gdrIdx,matRebar::Type& type,matRebar::Grade& grade)
{
   GET_IFACE(IShear,pShear);
   pShear->GetStirrupMaterial(spanIdx,gdrIdx,type,grade);
}

void CBridgeAgentImp::GetDeckRebarProperties(Float64* pE,Float64 *pFy,Float64* pFu)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CDeckDescription* pDeck = pIBridgeDesc->GetDeckDescription();
   const matRebar* pRebar = lrfdRebarPool::GetInstance()->GetRebar(pDeck->DeckRebarData.TopRebarType,pDeck->DeckRebarData.TopRebarGrade,matRebar::bs3);
   *pE  = pRebar->GetE();
   *pFy = pRebar->GetYieldStrength();
   *pFu = pRebar->GetUltimateStrength();
}

std::_tstring CBridgeAgentImp::GetDeckRebarName()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CDeckDescription* pDeck = pIBridgeDesc->GetDeckDescription();
   return lrfdRebarPool::GetMaterialName(pDeck->DeckRebarData.TopRebarType,pDeck->DeckRebarData.TopRebarGrade);
}

void CBridgeAgentImp::GetDeckRebarMaterial(matRebar::Type& type,matRebar::Grade& grade)
{
   // top and bottom mat use the same material
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CDeckDescription* pDeck = pIBridgeDesc->GetDeckDescription();
   type = pDeck->DeckRebarData.TopRebarType;
   grade = pDeck->DeckRebarData.TopRebarGrade;
}

Float64 CBridgeAgentImp::GetNWCDensityLimit()
{
   Float64 limit;
   if ( lrfdVersionMgr::GetUnits() == lrfdVersionMgr::US )
      limit = ::ConvertToSysUnits(135.0,unitMeasure::LbfPerFeet3);
   else
      limit = ::ConvertToSysUnits(2150.0,unitMeasure::KgPerMeter3);

   return limit;
}

/////////////////////////////////////////////////////////////////////////
// IBridgeMaterialEx
//
Float64 CBridgeAgentImp::GetLWCDensityLimit()
{
   Float64 limit;
   if ( lrfdVersionMgr::GetUnits() == lrfdVersionMgr::US )
      limit = ::ConvertToSysUnits(120.0,unitMeasure::LbfPerFeet3);
   else
      limit = ::ConvertToSysUnits(1925.0,unitMeasure::KgPerMeter3);

   return limit;
}

pgsTypes::ConcreteType CBridgeAgentImp::GetGdrConcreteType(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   VALIDATE( CONCRETE );
   SpanGirderHashType key = HashSpanGirder(spanIdx,gdrIdx);
   return (pgsTypes::ConcreteType)m_pGdrConc[key]->GetType();
}

bool CBridgeAgentImp::DoesGdrConcreteHaveAggSplittingStrength(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   VALIDATE( CONCRETE );
   SpanGirderHashType key = HashSpanGirder(spanIdx,gdrIdx);
   return m_pGdrConc[key]->HasAggSplittingStrength();
}

Float64 CBridgeAgentImp::GetGdrConcreteAggSplittingStrength(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   VALIDATE( CONCRETE );
   SpanGirderHashType key = HashSpanGirder(spanIdx,gdrIdx);
   return m_pGdrConc[key]->GetAggSplittingStrength();
}

pgsTypes::ConcreteType CBridgeAgentImp::GetSlabConcreteType()
{
   VALIDATE( CONCRETE );
   return (pgsTypes::ConcreteType)m_pSlabConc->GetType();
}

bool CBridgeAgentImp::DoesSlabConcreteHaveAggSplittingStrength()
{
   VALIDATE( CONCRETE );
   return m_pSlabConc->HasAggSplittingStrength();
}

Float64 CBridgeAgentImp::GetSlabConcreteAggSplittingStrength()
{
   VALIDATE( CONCRETE );
   return m_pSlabConc->GetAggSplittingStrength();
}

Float64 CBridgeAgentImp::GetFlexureModRupture(Float64 fc,pgsTypes::ConcreteType type)
{
   return lrfdConcreteUtil::ModRupture( fc, lrfdConcreteUtil::DensityType(type), GetFlexureFrCoefficient(type) );
}

Float64 CBridgeAgentImp::GetFlexureFrCoefficient(pgsTypes::ConcreteType type)
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   return pSpecEntry->GetFlexureModulusOfRuptureCoefficient(type);
}

Float64 CBridgeAgentImp::GetFlexureFrCoefficient(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   pgsTypes::ConcreteType type = GetGdrConcreteType(spanIdx,gdrIdx);
   return GetFlexureFrCoefficient(type);
}

Float64 CBridgeAgentImp::GetShearModRupture(Float64 fc,pgsTypes::ConcreteType type)
{
   return lrfdConcreteUtil::ModRupture( fc, lrfdConcreteUtil::DensityType(type), GetShearFrCoefficient(type) );
}

Float64 CBridgeAgentImp::GetShearFrCoefficient(pgsTypes::ConcreteType type)
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   return pSpecEntry->GetShearModulusOfRuptureCoefficient(type);
}

Float64 CBridgeAgentImp::GetShearFrCoefficient(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   pgsTypes::ConcreteType type = GetGdrConcreteType(spanIdx,gdrIdx);
   return GetShearFrCoefficient(type);
}

Float64 CBridgeAgentImp::GetEccK1Gdr(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE(IGirderData,pGirderData);
   Float64 K1 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      const CGirderMaterial* pGdrMaterial = pGirderData->GetGirderMaterial(spanIdx,gdrIdx);
      K1 *= pGdrMaterial->EcK1;
   }

   return K1;
}

Float64 CBridgeAgentImp::GetEccK2Gdr(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE(IGirderData,pGirderData);
   Float64 K2 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      const CGirderMaterial* pGdrMaterial = pGirderData->GetGirderMaterial(spanIdx,gdrIdx);
      K2 *= pGdrMaterial->EcK2;
   }

   return K2;
}

Float64 CBridgeAgentImp::GetCreepK1Gdr(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE(IGirderData,pGirderData);
   Float64 K1 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      const CGirderMaterial* pGdrMaterial = pGirderData->GetGirderMaterial(spanIdx,gdrIdx);
      K1 *= pGdrMaterial->CreepK1;
   }

   return K1;
}

Float64 CBridgeAgentImp::GetCreepK2Gdr(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE(IGirderData,pGirderData);
   Float64 K2 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      const CGirderMaterial* pGdrMaterial = pGirderData->GetGirderMaterial(spanIdx,gdrIdx);
      K2 *= pGdrMaterial->CreepK2;
   }

   return K2;
}

Float64 CBridgeAgentImp::GetShrinkageK1Gdr(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE(IGirderData,pGirderData);
   Float64 K1 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      const CGirderMaterial* pGdrMaterial = pGirderData->GetGirderMaterial(spanIdx,gdrIdx);
      K1 *= pGdrMaterial->ShrinkageK1;
   }

   return K1;
}

Float64 CBridgeAgentImp::GetShrinkageK2Gdr(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE(IGirderData,pGirderData);
   Float64 K2 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      const CGirderMaterial* pGdrMaterial = pGirderData->GetGirderMaterial(spanIdx,gdrIdx);
      K2 *= pGdrMaterial->ShrinkageK2;
   }

   return K2;
}

Float64 CBridgeAgentImp::GetEccK1Slab()
{
   VALIDATE( CONCRETE );
   Float64 K1 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      K1 *= m_SlabEcK1;
   }
   return K1;
}

Float64 CBridgeAgentImp::GetEccK2Slab()
{
   VALIDATE( CONCRETE );
   Float64 K2 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      K2 *= m_SlabEcK2;
   }
   return K2;
}

Float64 CBridgeAgentImp::GetCreepK1Slab()
{
   VALIDATE( CONCRETE );
   Float64 K1 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      K1 *= m_SlabCreepK1;
   }
   return K1;
}

Float64 CBridgeAgentImp::GetCreepK2Slab()
{
   VALIDATE( CONCRETE );
   Float64 K2 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      K2 *= m_SlabCreepK2;
   }
   return K2;
};

Float64 CBridgeAgentImp::GetShrinkageK1Slab()
{
   VALIDATE( CONCRETE );
   Float64 K1 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      K1 *= m_SlabShrinkageK1;
   }
   return K1;
}

Float64 CBridgeAgentImp::GetShrinkageK2Slab()
{
   VALIDATE( CONCRETE );
   Float64 K2 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      K2 *= m_SlabShrinkageK2;
   }
   return K2;
};

Float64 CBridgeAgentImp::GetEconc(Float64 fc,Float64 density,Float64 K1,Float64 K2)
{
   return K1*K2*lrfdConcreteUtil::ModE(fc,density, false ); // ignore LRFD limits
}

/////////////////////////////////////////////////////////////////////////
// IStageMap
CComBSTR CBridgeAgentImp::GetStageName(pgsTypes::Stage stage)
{
   CComBSTR bstrStage;
   switch(stage)
   {
      case pgsTypes::CastingYard:
         bstrStage = _T("Casting Yard");
      break;

      case pgsTypes::GirderPlacement:
         bstrStage = _T("Girder Placement");
      break;

      case pgsTypes::TemporaryStrandRemoval:
         bstrStage = _T("Temporary Strand Removal");
      break;

      case pgsTypes::BridgeSite1:
         bstrStage = _T("Bridge Site 1");
      break;

      case pgsTypes::BridgeSite2:
         bstrStage = _T("Bridge Site 2");
      break;

      case pgsTypes::BridgeSite3:
         bstrStage = _T("Bridge Site 3");
      break;

      default:
         ATLASSERT(false); // SHOULD NEVER GET HERE
   }

   return bstrStage;
}

CComBSTR CBridgeAgentImp::GetLimitStateName(pgsTypes::LimitState ls)
{
   CComBSTR bstrLimitState;
   switch(ls)
   {
      case pgsTypes::ServiceI:
         bstrLimitState = _T("Service I");
         break;

      case pgsTypes::ServiceIA:
         bstrLimitState = _T("Service IA");
         break;

      case pgsTypes::ServiceIII:
         bstrLimitState = _T("Service III");
         break;

      case pgsTypes::StrengthI:
         bstrLimitState = _T("Strength I");
         break;

      case pgsTypes::StrengthII:
         bstrLimitState = _T("Strength II");
         break;

      case pgsTypes::FatigueI:
         bstrLimitState = _T("Fatigue I");
         break;

      case pgsTypes::StrengthI_Inventory:
         bstrLimitState = _T("Strength I (Inventory)");
         break;

      case pgsTypes::StrengthI_Operating:
         bstrLimitState = _T("Strength I (Operating)");
         break;

      case pgsTypes::ServiceIII_Inventory:
         bstrLimitState = _T("Service III (Inventory)");
         break;

      case pgsTypes::ServiceIII_Operating:
         bstrLimitState = _T("Service III (Operating)");
         break;

      case pgsTypes::StrengthI_LegalRoutine:
         bstrLimitState = _T("Strength I (Legal - Routine)");
         break;

      case pgsTypes::StrengthI_LegalSpecial:
         bstrLimitState = _T("Strength I (Legal - Special)");
         break;

      case pgsTypes::ServiceIII_LegalRoutine:
         bstrLimitState = _T("Service III (Legal - Routine)");
         break;

      case pgsTypes::ServiceIII_LegalSpecial:
         bstrLimitState = _T("Service III (Legal - Special)");
         break;

      case pgsTypes::StrengthII_PermitRoutine:
         bstrLimitState = _T("Strength II (Routine Permit Rating)");
         break;

      case pgsTypes::ServiceI_PermitRoutine:
         bstrLimitState = _T("Service I (Routine Permit Rating)");
         break;

      case pgsTypes::StrengthII_PermitSpecial:
         bstrLimitState = _T("Strength II (Special Permit Rating)");
         break;

      case pgsTypes::ServiceI_PermitSpecial:
         bstrLimitState = _T("Service I (Special Permit Rating)");
         break;

      default:
         ATLASSERT(false); // SHOULD NEVER GET HERE
   }

   return bstrLimitState;
}

pgsTypes::Stage CBridgeAgentImp::GetStageType(CComBSTR bstrStage)
{
   if ( bstrStage == CComBSTR("Girder Placement") )
   {
      return pgsTypes::GirderPlacement;
   }
   else if ( bstrStage == CComBSTR("Temporary Strand Removal") )
   {
      return pgsTypes::TemporaryStrandRemoval;
   }
   else if ( bstrStage == CComBSTR("Bridge Site 1") )
   {
      return pgsTypes::BridgeSite1;
   }
   else if ( bstrStage == CComBSTR("Bridge Site 2") )
   {
      return pgsTypes::BridgeSite2;
   }
   else if ( bstrStage == CComBSTR("Bridge Site 3") )
   {
      return pgsTypes::BridgeSite3;
   }
   else
   {
      ATLASSERT(false); // SHOULD NEVER GET HERE
      return (pgsTypes::Stage)-999;
   }
}

/////////////////////////////////////////////////////////////////////////
// ILongRebarGeometry
void CBridgeAgentImp::GetRebars(const pgsPointOfInterest& poi,IRebarSection** rebarSection)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();
   Float64 loc = poi.GetDistFromStart();

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   CComPtr<IRebarLayout> rebar_layout;
   girder->get_RebarLayout(&rebar_layout);

   rebar_layout->CreateRebarSection(loc,rebarSection);
}

Float64 CBridgeAgentImp::GetAsBottomHalf(const pgsPointOfInterest& poi,bool bDevAdjust)
{
   return GetAsTensionSideOfGirder(poi,bDevAdjust,false);
}

Float64 CBridgeAgentImp::GetAsTopHalf(const pgsPointOfInterest& poi,bool bDevAdjust)
{
   return GetAsGirderTopHalf(poi,bDevAdjust) + GetAsDeckTopHalf(poi,bDevAdjust);
}

Float64 CBridgeAgentImp::GetAsGirderTopHalf(const pgsPointOfInterest& poi,bool bDevAdjust)
{
   return GetAsTensionSideOfGirder(poi,bDevAdjust,true);
}

Float64 CBridgeAgentImp::GetAsDeckTopHalf(const pgsPointOfInterest& poi,bool bDevAdjust)
{
   Float64 As_Top    = GetAsTopMat(poi,ILongRebarGeometry::All);
   Float64 As_Bottom = GetAsBottomMat(poi,ILongRebarGeometry::All);

   return As_Top + As_Bottom;
}

Float64 CBridgeAgentImp::GetDevLengthFactor(SpanIndexType span,GirderIndexType gdr,IRebarSectionItem* rebarItem)
{
   CComPtr<IRebar> rebar;
   rebarItem->get_Rebar(&rebar);
   Float64 fc = GetFcGdr(span,gdr);

   REBARDEVLENGTHDETAILS details = GetRebarDevelopmentLengthDetails(rebar,fc);

   Float64 fra = 1.0;
   Float64 start,end;
   rebarItem->get_DistFromStart(&start);
   rebarItem->get_DistFromEnd(&end);

   Float64 fra1 = start/details.ldb;
   Float64 fra2 = end/details.ldb;
   fra = Min3(fra1,fra2,fra);

   return fra;
}

Float64 CBridgeAgentImp::GetPPRTopHalf(const pgsPointOfInterest& poi)
{
   SpanIndexType span = poi.GetSpan();
   GirderIndexType gdr  = poi.GetGirder();

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Float64 As = GetAsDeckTopHalf(poi,false);

   if ( pSpecEntry->IncludeRebarForMoment() )
      As += GetAsGirderTopHalf(poi,false);

   Float64 Aps = GetApsTopHalf(poi,dlaNone);

   const matPsStrand* pstrand = GetStrand(span,gdr,pgsTypes::Permanent);
   CHECK(pstrand!=0);
   Float64 fps = pstrand->GetYieldStrength();
   CHECK(fps>0.0);

   Float64 E,fs,fu;
   GetLongitudinalRebarProperties(span,gdr,&E,&fs,&fu);

   Float64 denom = Aps*fps + As*fs;
   Float64 ppr;
   if ( IsZero(denom) )
      ppr = 0.0;
   else
      ppr = (Aps*fps)/denom;

   return ppr;
}

Float64 CBridgeAgentImp::GetPPRTopHalf(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Float64 As = GetAsDeckTopHalf(poi,false);

   if ( pSpecEntry->IncludeRebarForMoment() )
      As += GetAsGirderTopHalf(poi,false);

   Float64 Aps = GetApsTopHalf(poi,config,dlaNone);

   const matPsStrand* pstrand = GetStrand(span,gdr,pgsTypes::Permanent);
   CHECK(pstrand!=0);
   Float64 fps = pstrand->GetYieldStrength();
   CHECK(fps>0.0);

   Float64 E,fs,fu;
   GetLongitudinalRebarProperties(span,gdr,&E,&fs,&fu);

   Float64 denom = Aps*fps + As*fs;
   Float64 ppr;
   if ( IsZero(denom) )
      ppr = 0.0;
   else
      ppr = (Aps*fps)/denom;

   return ppr;
}

Float64 CBridgeAgentImp::GetPPRBottomHalf(const pgsPointOfInterest& poi)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Float64 As = 0;
   if ( pSpecEntry->IncludeRebarForMoment() )
      As = GetAsBottomHalf(poi,false);

   Float64 Aps = GetApsBottomHalf(poi,dlaNone);

   const matPsStrand* pstrand = GetStrand(span,gdr,pgsTypes::Permanent);
   CHECK(pstrand!=0);
   Float64 fps = pstrand->GetYieldStrength();
   CHECK(fps>0.0);

   Float64 E,fs,fu;
   GetLongitudinalRebarProperties(span,gdr,&E,&fs,&fu);

   Float64 denom = Aps*fps + As*fs;
   Float64 ppr;
   if ( IsZero(denom) )
      ppr = 0.0;
   else
      ppr = (Aps*fps)/denom;

   return ppr;
}

Float64 CBridgeAgentImp::GetPPRBottomHalf(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Float64 As = 0;
   if ( pSpecEntry->IncludeRebarForMoment() )
      As = GetAsBottomHalf(poi,false);

   Float64 Aps = GetApsBottomHalf(poi,config,dlaNone);

   const matPsStrand* pstrand = GetStrand(span,gdr,pgsTypes::Permanent);
   CHECK(pstrand!=0);
   Float64 fps = pstrand->GetYieldStrength();
   CHECK(fps>0.0);

   Float64 E,fs,fu;
   GetLongitudinalRebarProperties(span,gdr,&E,&fs,&fu);

   Float64 denom = Aps*fps + As*fs;
   Float64 ppr;
   if ( IsZero(denom) )
      ppr = 0.0;
   else
      ppr = (Aps*fps)/denom;

   return ppr;
}

REBARDEVLENGTHDETAILS CBridgeAgentImp::GetRebarDevelopmentLengthDetails(IRebar* rebar,Float64 fc)
{
   REBARDEVLENGTHDETAILS details;
   rebar->get_NominalArea(&details.Ab);
   rebar->get_NominalDiameter(&details.db);
   rebar->get_YieldStrength(&details.fy);
   details.fc = fc;

   if ( lrfdVersionMgr::GetUnits() == lrfdVersionMgr::US )
   {
      Float64 Ab = ::ConvertFromSysUnits(details.Ab,unitMeasure::Inch2);
      Float64 db = ::ConvertFromSysUnits(details.db,unitMeasure::Inch);
      Float64 fc = ::ConvertFromSysUnits(details.fc,unitMeasure::KSI);
      Float64 fy = ::ConvertFromSysUnits(details.fy,unitMeasure::KSI);
   
      details.ldb1 = 1.25*Ab*fy/sqrt(fc);
      details.ldb1 = ::ConvertToSysUnits(details.ldb1,unitMeasure::Inch);
   
      details.ldb2 = 0.4*db*fy;
      details.ldb2 = ::ConvertToSysUnits(details.ldb2,unitMeasure::Inch);

      Float64 ldb_min = ::ConvertToSysUnits(12.0,unitMeasure::Inch);

      details.ldb = Max3(details.ldb1,details.ldb2,ldb_min);
   }
   else
   {
      Float64 Ab = ::ConvertFromSysUnits(details.Ab,unitMeasure::Millimeter2);
      Float64 db = ::ConvertFromSysUnits(details.db,unitMeasure::Millimeter);
      Float64 fc = ::ConvertFromSysUnits(details.fc,unitMeasure::MPa);
      Float64 fy = ::ConvertFromSysUnits(details.fy,unitMeasure::MPa);
   
      details.ldb1 = 0.02*Ab*fy/sqrt(fc);
      details.ldb1 = ::ConvertToSysUnits(details.ldb1,unitMeasure::Millimeter);
   
      details.ldb2 = 0.06*db*fy;
      details.ldb2 = ::ConvertToSysUnits(details.ldb2,unitMeasure::Millimeter);

      Float64 ldb_min = ::ConvertToSysUnits(12.0,unitMeasure::Millimeter);

      details.ldb = Max3(details.ldb1,details.ldb2,ldb_min);
   }

   return details;
}

Float64 CBridgeAgentImp::GetCoverTopMat()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();
   const CDeckRebarData& rebarData = pDeck->DeckRebarData;

   return rebarData.TopCover;
}

Float64 CBridgeAgentImp::GetAsTopMat(const pgsPointOfInterest& poi,ILongRebarGeometry::DeckRebarType drt)
{
   return GetAsDeckMats(poi,drt,true,false);
}

Float64 CBridgeAgentImp::GetCoverBottomMat()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();
   const CDeckRebarData& rebarData = pDeck->DeckRebarData;

   return rebarData.BottomCover;
}

Float64 CBridgeAgentImp::GetAsBottomMat(const pgsPointOfInterest& poi,ILongRebarGeometry::DeckRebarType drt)
{
   return GetAsDeckMats(poi,drt,false,true);
}

/////////////////////////////////////////////////////////////////////////
// IStirrupGeometry
bool CBridgeAgentImp::AreStirrupZonesSymmetrical(SpanIndexType span,GirderIndexType gdr)
{
   const CShearData& shear_data = GetShearData(span, gdr);
   return shear_data.bAreZonesSymmetrical;
}

ZoneIndexType CBridgeAgentImp::GetNumPrimaryZones(SpanIndexType span,GirderIndexType gdr)
{
   const CShearData& shear_data = GetShearData(span, gdr);
   ZoneIndexType nZones = shear_data.ShearZones.size();

   if (nZones == 0)
      return 0;
   else
   {
      // determine the actual number of zones within the girder
	   GET_IFACE(IBridge,pBridge);
      if (shear_data.bAreZonesSymmetrical)
      {
         Float64 half_girder_length = pBridge->GetGirderLength(span, gdr)/2.0;

         Float64 end_of_zone = 0.0;
         for (ZoneIndexType zoneIdx = 0; zoneIdx < nZones; zoneIdx++)
         {
            end_of_zone += shear_data.ShearZones[zoneIdx].ZoneLength;
            if (half_girder_length < end_of_zone)
               return 2*(zoneIdx+1)-1;
         }
         return nZones*2-1;
      }
      else
      {
         Float64 girder_length = pBridge->GetGirderLength(span, gdr);

         Float64 end_of_zone = 0.0;
         for (ZoneIndexType zoneIdx = 0; zoneIdx < nZones; zoneIdx++)
         {
            end_of_zone += shear_data.ShearZones[zoneIdx].ZoneLength;
            if (girder_length < end_of_zone)
               return (zoneIdx+1);
         }

         return nZones;
      }
   }
}

void CBridgeAgentImp::GetPrimaryZoneBounds(SpanIndexType span,GirderIndexType gdr, ZoneIndexType zone, Float64* start, Float64* end)
{
   const CShearData& shear_data = GetShearData(span, gdr);
   Float64 gird_len = GetGirderLength(span,gdr);

   if(shear_data.bAreZonesSymmetrical)
   {
      ZoneIndexType nz = GetNumPrimaryZones(span, gdr);
      CHECK(nz>zone);

      ZoneIndexType idx = GetPrimaryZoneIndex(span, gdr, shear_data,zone);

      // determine which side of girder zone is on
      enum side {OnLeft, OverCenter, OnRight} zside;
      if (zone == (nz-1)/2)
         zside = OverCenter;
      else if (idx==zone)
         zside = OnLeft;
      else
         zside = OnRight;

      ZoneIndexType zsiz = shear_data.ShearZones.size();
      Float64 l_end=0.0;
      Float64 l_start=0.0;
      for (ZoneIndexType i=0; i<=idx; i++)
      {
         if (i==zsiz-1)
            l_end+= BIG_LENGTH; // last zone in infinitely long
         else
            l_end+= shear_data.ShearZones[i].ZoneLength;

         if (l_end>=gird_len/2.0)
            CHECK(i==idx);  // better be last one or too many zones

         if (i!=idx)
            l_start = l_end;
      }

      if (zside==OnLeft)
      {
         *start = l_start;
         *end =   l_end;
      }
      else if (zside==OverCenter)
      {
         *start = l_start;
         *end =   gird_len - l_start;
      }
      else if (zside==OnRight)
      {
         *start = gird_len - l_end;
         *end =   gird_len - l_start;
      }
   }
   else
   {
      // Non-symmetrical zones
      ZoneIndexType idx = GetPrimaryZoneIndex(span, gdr, shear_data,zone);
      ZoneIndexType zsiz = shear_data.ShearZones.size();

      Float64 l_end=0.0;
      Float64 l_start=0.0;
      for (ZoneIndexType i=0; i<=idx; i++)
      {
         if (i==zsiz-1)
            l_end = gird_len; // last zone goes to end of girder
         else
            l_end+= shear_data.ShearZones[i].ZoneLength;

         if (l_end>=gird_len)
         {
            l_end = gird_len;
            break;
         }

         if (i!=idx)
            l_start = l_end;
      }

      *start = l_start;
      *end =   l_end;
   }
}

void CBridgeAgentImp::GetPrimaryVertStirrupBarInfo(SpanIndexType span,GirderIndexType gdr,ZoneIndexType zone, matRebar::Size* pSize, Float64* pCount, Float64* pSpacing)
{
   const CShearData& shear_data = GetShearData(span, gdr);
   ZoneIndexType idx = GetPrimaryZoneIndex(span, gdr, shear_data, zone);
   const CShearZoneData& rzone = shear_data.ShearZones[idx];

   *pSize    = rzone.VertBarSize;
   *pCount   = rzone.nVertBars;
   *pSpacing = rzone.BarSpacing;
}

Float64 CBridgeAgentImp::GetPrimaryHorizInterfaceBarCount(SpanIndexType span,GirderIndexType gdr,ZoneIndexType zone)
{
   const CShearData& shear_data = GetShearData(span, gdr);
   ZoneIndexType idx = GetPrimaryZoneIndex(span, gdr, shear_data,zone);
   const CShearZoneData& rzone = shear_data.ShearZones[idx];

   return rzone.nHorzInterfaceBars;
}

matRebar::Size CBridgeAgentImp::GetPrimaryConfinementBarSize(SpanIndexType span,GirderIndexType gdr,ZoneIndexType zone)
{
   const CShearData& shear_data = GetShearData(span, gdr);
   ZoneIndexType idx = GetPrimaryZoneIndex(span, gdr, shear_data,zone);
   const CShearZoneData& rzone = shear_data.ShearZones[idx];

   return rzone.ConfinementBarSize;
}

ZoneIndexType CBridgeAgentImp::GetNumHorizInterfaceZones(SpanIndexType span,GirderIndexType gdr)
{
   const CShearData& shear_data = GetShearData(span, gdr);
   ZoneIndexType nZones = shear_data.HorizontalInterfaceZones.size();
   if (nZones == 0)
      return 0;
   else
   {
      // determine the actual number of zones within the girder
	   GET_IFACE(IBridge,pBridge);
      if (shear_data.bAreZonesSymmetrical)
      {
         Float64 half_girder_length = pBridge->GetGirderLength(span, gdr)/2.0;

         Float64 end_of_zone = 0.0;
         for (ZoneIndexType zoneIdx = 0; zoneIdx < nZones; zoneIdx++)
         {
            end_of_zone += shear_data.HorizontalInterfaceZones[zoneIdx].ZoneLength;
            if (half_girder_length < end_of_zone)
               return 2*(zoneIdx+1)-1;
         }
         return nZones*2-1;
      }
      else
      {
         Float64 girder_length = pBridge->GetGirderLength(span, gdr);

         Float64 end_of_zone = 0.0;
         for (ZoneIndexType zoneIdx = 0; zoneIdx < nZones; zoneIdx++)
         {
            end_of_zone += shear_data.HorizontalInterfaceZones[zoneIdx].ZoneLength;
            if (girder_length < end_of_zone)
               return (zoneIdx+1);
         }

         return nZones;
      }
   }
}

void CBridgeAgentImp::GetHorizInterfaceZoneBounds(SpanIndexType span,GirderIndexType gdr, ZoneIndexType zone, Float64* start, Float64* end)
{
   const CShearData& shear_data = GetShearData(span, gdr);
   Float64 gird_len = GetGirderLength(span,gdr);

   if(shear_data.bAreZonesSymmetrical)
   {
      ZoneIndexType nz = GetNumHorizInterfaceZones(span,gdr);
      CHECK(nz>zone);

      ZoneIndexType idx = GetHorizInterfaceZoneIndex(span,gdr,shear_data,zone);

      // determine which side of girder zone is on
      enum side {OnLeft, OverCenter, OnRight} zside;
      if (zone == (nz-1)/2)
         zside = OverCenter;
      else if (idx==zone)
         zside = OnLeft;
      else
         zside = OnRight;

      ZoneIndexType zsiz = shear_data.HorizontalInterfaceZones.size();
      Float64 l_end=0.0;
      Float64 l_start=0.0;
      for (ZoneIndexType i=0; i<=idx; i++)
      {
         if (i==zsiz-1)
            l_end+= BIG_LENGTH; // last zone in infinitely long
         else
            l_end+= shear_data.HorizontalInterfaceZones[i].ZoneLength;

         if (l_end>=gird_len/2.0)
            CHECK(i==idx);  // better be last one or too many zones

         if (i!=idx)
            l_start = l_end;
      }

      if (zside==OnLeft)
      {
         *start = l_start;
         *end =   l_end;
      }
      else if (zside==OverCenter)
      {
         *start = l_start;
         *end =   gird_len - l_start;
      }
      else if (zside==OnRight)
      {
         *start = gird_len - l_end;
         *end =   gird_len - l_start;
      }
   }
   else
   {
      // Non-symmetrical zones
      ZoneIndexType idx = GetHorizInterfaceZoneIndex(span,gdr,shear_data,zone);
      ZoneIndexType zsiz = shear_data.HorizontalInterfaceZones.size();

      Float64 l_end=0.0;
      Float64 l_start=0.0;
      for (ZoneIndexType i=0; i<=idx; i++)
      {
         if (i==zsiz-1)
            l_end = gird_len; // last zone goes to end of girder
         else
            l_end+= shear_data.HorizontalInterfaceZones[i].ZoneLength;

         if (l_end>=gird_len)
         {
            l_end = gird_len;
            break;
         }

         if (i!=idx)
            l_start = l_end;
      }

      *start = l_start;
      *end =   l_end;
   }
}

void CBridgeAgentImp::GetHorizInterfaceBarInfo(SpanIndexType span,GirderIndexType gdr,ZoneIndexType zone, matRebar::Size* pSize, Float64* pCount, Float64* pSpacing)
{
   const CShearData& shear_data = GetShearData(span, gdr);
   ZoneIndexType idx = GetHorizInterfaceZoneIndex(span,gdr,shear_data,zone);

   const CHorizontalInterfaceZoneData& rdata = shear_data.HorizontalInterfaceZones[idx];
   *pSize = rdata.BarSize;
   *pCount = rdata.nBars;
   *pSpacing = rdata.BarSpacing;
}

void CBridgeAgentImp::GetAddSplittingBarInfo(SpanIndexType span,GirderIndexType gdr, matRebar::Size* pSize, Float64* pZoneLength, Float64* pnBars, Float64* pSpacing)
{
   const CShearData& shear_data = GetShearData(span, gdr);
   *pSize = shear_data.SplittingBarSize;
   if (*pSize!=matRebar::bsNone)
   {
      *pZoneLength = shear_data.SplittingZoneLength;
      *pnBars = shear_data.nSplittingBars;
      *pSpacing = shear_data.SplittingBarSpacing;
   }
   else
   {
      *pZoneLength = 0.0;
      *pnBars = 0.0;
      *pSpacing = 0.0;
   }
}

void CBridgeAgentImp::GetAddConfinementBarInfo(SpanIndexType span,GirderIndexType gdr, matRebar::Size* pSize, Float64* pZoneLength, Float64* pSpacing)
{
   const CShearData& shear_data = GetShearData(span, gdr);
   *pSize = shear_data.ConfinementBarSize;
   if (*pSize!=matRebar::bsNone)
   {
      *pZoneLength = shear_data.ConfinementZoneLength;
      *pSpacing = shear_data.ConfinementBarSpacing;
   }
   else
   {
      *pZoneLength = 0.0;
      *pSpacing = 0.0;
   }
}


Float64 CBridgeAgentImp::GetVertStirrupBarNominalDiameter(const pgsPointOfInterest& poi)
{
   const CShearData& rsheardata = GetShearData(poi.GetSpan(), poi.GetGirder());
   
   const CShearZoneData& rzonedata = GetPrimaryShearZoneDataAtPoi(poi, rsheardata);

   matRebar::Size barSize = rzonedata.VertBarSize;
   if ( barSize!=matRebar::bsNone && !IsZero(rzonedata.BarSpacing) )
   {
      lrfdRebarPool* prp = lrfdRebarPool::GetInstance();
      const matRebar* pRebar = prp->GetRebar(rsheardata.ShearBarType,rsheardata.ShearBarGrade,barSize);

      return (pRebar ? pRebar->GetNominalDimension() : 0.0);
   }
   else
      return 0.0;
}

Float64 CBridgeAgentImp::GetAlpha(const pgsPointOfInterest& poi)
{
   return ::ConvertToSysUnits(90.,unitMeasure::Degree);
}

Float64 CBridgeAgentImp::GetVertStirrupAvs(const pgsPointOfInterest& poi, matRebar::Size* pSize, Float64* pSingleBarArea, Float64* pCount, Float64* pSpacing)
{
   Float64 avs(0.0);
   Float64 Abar(0.0);
   Float64 nBars(0.0);
   Float64 spacing(0.0);

   SpanIndexType span = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   const CShearData& rsheardata = GetShearData(span, gdr);
   
   const CShearZoneData& rzonedata = GetPrimaryShearZoneDataAtPoi(poi, rsheardata);

   matRebar::Size barSize = rzonedata.VertBarSize;
   if ( barSize!=matRebar::bsNone && !IsZero(rzonedata.BarSpacing) )
   {
      lrfdRebarPool* prp = lrfdRebarPool::GetInstance();
      const matRebar* pbar = prp->GetRebar(rsheardata.ShearBarType,rsheardata.ShearBarGrade,barSize);

      Abar   = pbar->GetNominalArea();
      nBars = rzonedata.nVertBars;
      spacing = rzonedata.BarSpacing;

      // area of stirrups per unit length for this zone
      // (assume stirrups are smeared out along zone)
      if (spacing > 0.0)
         avs = nBars * Abar / spacing;
   }

   *pSize          = barSize;
   *pSingleBarArea = Abar;
   *pCount         = nBars;
   *pSpacing       = spacing;

   return avs;
}

bool CBridgeAgentImp::DoStirrupsEngageDeck(SpanIndexType span,GirderIndexType gdr)
{
   // Just check if any stirrups engage deck
   const CShearData& rShearData = GetShearData(span, gdr);

   for (CShearData::ShearZoneConstIterator its = rShearData.ShearZones.begin(); its != rShearData.ShearZones.end(); its++)
   {
      if (its->VertBarSize != matRebar::bsNone && its->nHorzInterfaceBars > 0)
         return true;
   }

   for (CShearData::HorizontalInterfaceZoneConstIterator it = rShearData.HorizontalInterfaceZones.begin(); it != rShearData.HorizontalInterfaceZones.end(); it++)
   {
      if (it->BarSize != matRebar::bsNone)
         return true;
   }

   return false;
}

bool CBridgeAgentImp::DoAllPrimaryStirrupsEngageDeck(SpanIndexType span,GirderIndexType gdr)
{
   // Check if all vertical stirrups engage deck
   const CShearData& rShearData = GetShearData(span, gdr);

   if (rShearData.ShearZones.empty())
   {
      return false;
   }
   else
   {
      for (CShearData::ShearZoneConstIterator its = rShearData.ShearZones.begin(); its != rShearData.ShearZones.end(); its++)
      {
         // Make sure there are vertical bars, and at least as many horiz int bars
         if (its->VertBarSize==matRebar::bsNone ||
             its->nVertBars <= 0                ||
             its->nHorzInterfaceBars < its->nVertBars)
            return false;
      }
   }

   return true;
}

Float64 CBridgeAgentImp::GetPrimaryHorizInterfaceS(const pgsPointOfInterest& poi)
{
   Float64 spacing = 0.0;

   SpanIndexType span = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   const CShearData& rShearData = GetShearData(span, gdr);

   // Horizontal legs in primary zones
   const CShearZoneData& rzonedata = GetPrimaryShearZoneDataAtPoi(poi, rShearData);
   if (rzonedata.VertBarSize!=matRebar::bsNone && rzonedata.nHorzInterfaceBars>0.0)
   {
      spacing = rzonedata.BarSpacing;
   }

   return spacing;
}

Float64 CBridgeAgentImp::GetPrimaryHorizInterfaceBarCount(const pgsPointOfInterest& poi)
{
   Float64 cnt = 0.0;

   SpanIndexType span = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   const CShearData& rShearData = GetShearData(span, gdr);

   // Horizontal legs in primary zones
   const CShearZoneData& rzonedata = GetPrimaryShearZoneDataAtPoi(poi, rShearData);
   if (rzonedata.VertBarSize!=matRebar::bsNone)
   {
      cnt =  rzonedata.nHorzInterfaceBars;
   }

   return cnt;
}

Float64 CBridgeAgentImp::GetAdditionalHorizInterfaceS(const pgsPointOfInterest& poi)
{
   Float64 spacing = 0.0;

   SpanIndexType span = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   const CShearData& rShearData = GetShearData(span, gdr);

   // Additional horizontal bars
   const CHorizontalInterfaceZoneData* phdata = GetHorizInterfaceShearZoneDataAtPoi( poi,  rShearData);
   if ( phdata!=NULL && phdata->BarSize!=matRebar::bsNone )
   {
      spacing = phdata->BarSpacing;
   }

   return spacing;
}

Float64 CBridgeAgentImp::GetAdditionalHorizInterfaceBarCount(const pgsPointOfInterest& poi)
{
   Float64 cnt = 0.0;

   SpanIndexType span = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   const CShearData& rShearData = GetShearData(span, gdr);

   // Additional horizontal bars
   const CHorizontalInterfaceZoneData* phdata = GetHorizInterfaceShearZoneDataAtPoi( poi,  rShearData);
   if ( phdata!=NULL && phdata->BarSize!=matRebar::bsNone )   
   {
      cnt = phdata->nBars;
   }

   return cnt;
}

Float64 CBridgeAgentImp::GetPrimaryHorizInterfaceAvs(const pgsPointOfInterest& poi, matRebar::Size* pSize, Float64* pSingleBarArea, Float64* pCount, Float64* pSpacing)
{
   Float64 avs(0.0);
   Float64 Abar(0.0);
   Float64 nBars(0.0);
   Float64 spacing(0.0);

   SpanIndexType span = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   const CShearData& rsheardata = GetShearData(span, gdr);
   
   // First get avs from primary bar zone
   const CShearZoneData& rzonedata = GetPrimaryShearZoneDataAtPoi(poi, rsheardata);

   matRebar::Size barSize = rzonedata.VertBarSize;

   if ( barSize!=matRebar::bsNone && !IsZero(rzonedata.BarSpacing) && rzonedata.nHorzInterfaceBars>0.0 )
   {
      lrfdRebarPool* prp = lrfdRebarPool::GetInstance();
      const matRebar* pbar = prp->GetRebar(rsheardata.ShearBarType,rsheardata.ShearBarGrade,barSize);

      Abar = pbar->GetNominalArea();
      nBars = rzonedata.nHorzInterfaceBars;
      spacing = rzonedata.BarSpacing;

      if (spacing > 0.0)
         avs =   nBars * Abar / spacing;
   }

   *pSize          = barSize;
   *pSingleBarArea = Abar;
   *pCount         = nBars;
   *pSpacing       = spacing;

   return avs;
}

Float64 CBridgeAgentImp::GetAdditionalHorizInterfaceAvs(const pgsPointOfInterest& poi, matRebar::Size* pSize, Float64* pSingleBarArea, Float64* pCount, Float64* pSpacing)
{
   Float64 avs(0.0);
   Float64 Abar(0.0);
   Float64 nBars(0.0);
   Float64 spacing(0.0);
   matRebar::Size barSize(matRebar::bsNone);

   SpanIndexType span = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   const CShearData& rsheardata = GetShearData(span, gdr);
   const CHorizontalInterfaceZoneData* phdata = GetHorizInterfaceShearZoneDataAtPoi( poi,  rsheardata);
  
   if (phdata!=NULL)
   {
      barSize = phdata->BarSize;

      if ( barSize!=matRebar::bsNone && !IsZero(phdata->BarSpacing) && phdata->nBars>0.0 )
      {
         lrfdRebarPool* prp = lrfdRebarPool::GetInstance();
         const matRebar* pbar = prp->GetRebar(rsheardata.ShearBarType,rsheardata.ShearBarGrade,barSize);

         Abar    = pbar->GetNominalArea();
         nBars   = phdata->nBars;
         spacing = phdata->BarSpacing;

         if (spacing > 0.0)
            avs =  nBars * Abar / spacing;
      }
   }

   *pSize          = barSize;
   *pSingleBarArea = Abar;
   *pCount         = nBars;
   *pSpacing       = spacing;

   return avs;
}

Float64 CBridgeAgentImp::GetSplittingAv(SpanIndexType span,GirderIndexType gdr,Float64 start,Float64 end)
{
   ATLASSERT(end>start);

   Float64 Av = 0.0;

   const CShearData& rsheardata = GetShearData(span, gdr);

   // Get component from primary bars
   if (rsheardata.bUsePrimaryForSplitting)
   {
      Av += GetPrimarySplittingAv( span, gdr, start, end, rsheardata);
   }

   // Component from additional splitting bars
   if (rsheardata.SplittingBarSize!=matRebar::bsNone && rsheardata.nSplittingBars)
   {
      Float64 spacing = rsheardata.SplittingBarSpacing;
      Float64 length = 0.0;
      if (spacing <=0.0)
      {
         ATLASSERT(0); // UI should block this
      }
      else
      {
         // determine how much additional bars is in our start/end region
         Float64 zone_length = rsheardata.SplittingZoneLength;
         // left end first
         if (start<zone_length)
         {
            Float64 zend = min(end, zone_length);
            length = zend-start;
         }
         else
         {
            // try right end
            Float64 gird_len = GetGirderLength(span,gdr);
            if (end>=gird_len)
            {
               Float64 zstart = max(gird_len-zone_length, start);
               length = end-zstart;
            }
         }

         if (length > 0.0)
         {
            // We have bars in region. multiply av/s * length
            lrfdRebarPool* prp = lrfdRebarPool::GetInstance();
            const matRebar* pbar = prp->GetRebar(rsheardata.ShearBarType, rsheardata.ShearBarGrade, rsheardata.SplittingBarSize);

            Float64 Abar   = pbar->GetNominalArea();

            Float64 avs = rsheardata.nSplittingBars * Abar / spacing;

            Av += avs * length;
         }
      }
   }

   return Av;
}

Float64 CBridgeAgentImp::GetPrimarySplittingAv(SpanIndexType span,GirderIndexType gdr,Float64 start,Float64 end, const CShearData& rShearData)
{
   if (!rShearData.bUsePrimaryForSplitting)
   {
      ATLASSERT(0); // shouldn't be called for this case
      return 0.0;
   }

   // Get total amount of splitting steel between start and end
   Float64 Av = 0;

   ZoneIndexType nbrZones = GetNumPrimaryZones(span,gdr);
   for ( ZoneIndexType zone = 0; zone < nbrZones; zone++ )
   {
      Float64 zoneStart, zoneEnd;
      GetPrimaryZoneBounds(span , gdr, zone, &zoneStart, &zoneEnd);

      Float64 length; // length of zone which falls within the range

      // zoneStart                zoneEnd
      //   |-----------------------|
      //       zone is here (1) zoneStart                zoneEnd
      //                           |-----------------------|
      //                                 or here (2)    zoneStart                zoneEnd
      //                                                   |-----------------------|
      //                                                           here (3)
      //                |=============================================|
      //             start                 Range                     end
      //      zoneStart                                                   zoneEnd
      //        |-------------------------------------------------------------|
      //                       (4) zone is larger than range 

      if ( start <= zoneStart && zoneEnd <= end )
      {
         // Case 2 - entire zone is in the range
         length = zoneEnd - zoneStart;
      }
      else if ( zoneStart < start && end < zoneEnd )
      {
         // Case 4
         length = end - start;
      }
      else if ( zoneStart < start && InRange(start,zoneEnd,end) )
      {
         // Case 1
         length = zoneEnd - start;
      }
      else if ( InRange(start,zoneStart,end) && end < zoneEnd )
      {
         // Case 3
         length = end - zoneStart;
      }
      else
      {
         continue; // This zone doesn't touch the range at all... go back to the start of the loop
      }

      // We are in a zone - determine Av/S and multiply by length
      ZoneIndexType idx = GetPrimaryZoneIndex(span, gdr, rShearData, zone);

      const CShearZoneData& rzonedata = rShearData.ShearZones[idx];

      matRebar::Size barSize = rzonedata.VertBarSize; // splitting is same as vert bars
      if ( barSize!=matRebar::bsNone && !IsZero(rzonedata.BarSpacing) )
      {
         lrfdRebarPool* prp = lrfdRebarPool::GetInstance();
         const matRebar* pbar = prp->GetRebar(rShearData.ShearBarType,rShearData.ShearBarGrade,barSize);

         Float64 Abar   = pbar->GetNominalArea();

         // area of stirrups per unit length for this zone
         // (assume stirrups are smeared out along zone)
         Float64 avs = rzonedata.nVertBars * Abar / rzonedata.BarSpacing;

         Av += avs * length;
      }
   }

   return Av;
}

void CBridgeAgentImp::GetStartConfinementBarInfo(SpanIndexType span,GirderIndexType gdr, Float64 requiredZoneLength, matRebar::Size* pSize, Float64* pProvidedZoneLength, Float64* pSpacing)
{
   const CShearData& shear_data = GetShearData(span, gdr);

   ZoneIndexType nbrZones = GetNumPrimaryZones(span,gdr);

   // First get data from primary zones - use min bar size and max spacing from zones in required region
   Float64 primSpc(-1), primZonL(-1);
   matRebar::Size primSize(matRebar::bsNone);

   Float64 ezloc;

   // walk from left to right on girder
   for ( ZoneIndexType zone = 0; zone < nbrZones; zone++ )
   {
      Float64 zoneStart;
      GetPrimaryZoneBounds(span , gdr, zone, &zoneStart, &ezloc);

      ZoneIndexType idx = GetPrimaryZoneIndex(span, gdr, shear_data,zone);

      const CShearZoneData& rzone = shear_data.ShearZones[idx];

      if (rzone.ConfinementBarSize!=matRebar::bsNone)
      {
         primSize = min(primSize, rzone.ConfinementBarSize);
         primSpc = max(primSpc, rzone.BarSpacing);

         primZonL = ezloc;
      }
      else
      {
         // Can't have any zones with no confinement bars in required area - break with what we have so far
         break;
      }

      if (ezloc + TOL > requiredZoneLength)
      {
         break; // actual zone length exceeds required - we are done
      }
   }

   // Next get additional confinement bar info
   Float64 addlSpc, addlZonL;
   matRebar::Size addlSize;
   GetAddConfinementBarInfo(span,gdr, &addlSize, &addlZonL, &addlSpc);

   // Use either primary bars or additional bars. Choose by which has addequate zone length, smallest spacing, largest bars
   ChooseConfinementBars(requiredZoneLength, primSpc, primZonL, primSize, addlSpc, addlZonL, addlSize,
                         pSize, pProvidedZoneLength, pSpacing);
}


void CBridgeAgentImp::GetEndConfinementBarInfo(  SpanIndexType span,GirderIndexType gdr, Float64 requiredZoneLength, matRebar::Size* pSize, Float64* pProvidedZoneLength, Float64* pSpacing)
{
   const CShearData& shear_data = GetShearData(span, gdr);

   Float64 glen = GetGirderLength(span, gdr);

   ZoneIndexType nbrZones = GetNumPrimaryZones(span,gdr);

   // First get data from primary zones - use min bar size and max spacing from zones in required region
   Float64 primSpc(-1), primZonL(-1);
   matRebar::Size primSize(matRebar::bsNone);

   Float64 ezloc;
   // walk from right to left on girder
   for ( ZoneIndexType zone = nbrZones-1; zone>=0; zone-- )
   {
      Float64 zoneStart, zoneEnd;
      GetPrimaryZoneBounds(span , gdr, zone, &zoneStart, &zoneEnd);

      ezloc = glen - zoneStart;

      ZoneIndexType idx = GetPrimaryZoneIndex(span, gdr, shear_data, zone);

      const CShearZoneData& rzone = shear_data.ShearZones[idx];

      if (rzone.ConfinementBarSize!=matRebar::bsNone)
      {
         primSize = min(primSize, rzone.ConfinementBarSize);
         primSpc = max(primSpc, rzone.BarSpacing);

         primZonL = ezloc;
      }
      else
      {
         // Can't have any zones with no confinement bars in required area - break with what we have so far
         break;
      }

      if (ezloc + TOL > requiredZoneLength)
      {
         break; // actual zone length exceeds required - we are done
      }
   }

   // Next get additional confinement bar info
   Float64 addlSpc, addlZonL;
   matRebar::Size addlSize;
   GetAddConfinementBarInfo(span,gdr, &addlSize, &addlZonL, &addlSpc);

   // Use either primary bars or additional bars. Choose by which has adequate zone length, smallest spacing, largest bars
   ChooseConfinementBars(requiredZoneLength, primSpc, primZonL, primSize, addlSpc, addlZonL, addlSize,
                         pSize, pProvidedZoneLength, pSpacing);
}

// private:

void CBridgeAgentImp::InvalidateStirrupData()
{
   m_ShearData.clear();
}

const CShearData& CBridgeAgentImp::GetShearData(SpanIndexType span,GirderIndexType gdr)
{
   SpanGirderHashType hash = HashSpanGirder(span,gdr);
   ShearDataIterator found;
   found = m_ShearData.find( hash );
   if ( found == m_ShearData.end() )
   {
	   GET_IFACE2(m_pBroker,IShear,pShear);
	   CShearData shear_data = pShear->GetShearData(span,gdr);
      std::pair<ShearDataIterator,bool> insit = m_ShearData.insert( std::make_pair(hash, pShear->GetShearData(span,gdr) ) );
      ATLASSERT( insit.second );
      return (*insit.first).second;
   }
   else
   {
      ATLASSERT( found != m_ShearData.end() );
      return (*found).second;
   }
}

ZoneIndexType CBridgeAgentImp::GetPrimaryZoneIndex(SpanIndexType span, GirderIndexType gdr, const CShearData& rShearData, ZoneIndexType zone)
{
   // mapping so that we only need to store half of the zones
   ZoneIndexType nZones = GetNumPrimaryZones(span, gdr); 
   ATLASSERT(zone < nZones);
   if (rShearData.bAreZonesSymmetrical)
   {
      ZoneIndexType nz2 = (nZones+1)/2;
      if (zone < nz2)
         return zone;
      else
         return nZones-zone-1;
   }
   else
   {
      // mapping is 1:1 for non-sym
      return zone;
   }
}

ZoneIndexType CBridgeAgentImp::GetHorizInterfaceZoneIndex(SpanIndexType span, GirderIndexType gdr, const CShearData& rShearData, ZoneIndexType zone)
{
   // mapping so that we only need to store half of the zones
   ZoneIndexType nZones = GetNumHorizInterfaceZones(span, gdr); 
   ATLASSERT(zone < nZones);
   if (rShearData.bAreZonesSymmetrical)
   {
      ZoneIndexType nz2 = (nZones+1)/2;
      if (zone < nz2)
         return zone;
      else
         return nZones-zone-1;
   }
   else
   {
      // mapping is 1:1 for non-sym
      return zone;
   }
}

bool CBridgeAgentImp::IsPoiInEndRegion(const pgsPointOfInterest& poi, Float64 distFromEnds)
{
   Float64 dist = poi.GetDistFromStart();

   // look at left end first
   if (dist <= distFromEnds)
   {
      return true;
   }

   // right end
   Float64 glen = GetGirderLength(poi.GetSpan(),poi.GetGirder());

   if (dist >= glen-distFromEnds)
   {
      return true;
   }

   return false;
}

ZoneIndexType CBridgeAgentImp::GetPrimaryShearZoneIndexAtPoi(const pgsPointOfInterest& poi, const CShearData& rShearData)
{
   // NOTE: The logic here is identical to GetHorizInterfaceShearZoneIndexAtPoi
   //       If you fix a bug here, you need to fix it there also

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   ZoneIndexType nz = GetNumPrimaryZones(span, gdr);
   if (nz==0)
      return INVALID_INDEX;

   Float64 glen = GetGirderLength(span,gdr);
   Float64 location = poi.GetDistFromStart();

   Float64 lft_brg_loc = GetGirderStartConnectionLength(span,gdr);
   Float64 rgt_brg_loc = glen - GetGirderEndConnectionLength(span,gdr);

   // use template function to do heavy work
   ZoneIndexType zone =  GetZoneIndexAtLocation(location, glen, lft_brg_loc, rgt_brg_loc, rShearData.bAreZonesSymmetrical, 
                                                rShearData.ShearZones.begin(), rShearData.ShearZones.end(), 
                                                rShearData.ShearZones.size());

   return zone;
}

const CShearZoneData& CBridgeAgentImp::GetPrimaryShearZoneDataAtPoi(const pgsPointOfInterest& poi, const CShearData& rShearData)
{
   ZoneIndexType idx = GetPrimaryShearZoneIndexAtPoi(poi,rShearData);
   const CShearZoneData& rzonedata = rShearData.ShearZones[idx];

   return rzonedata;
}

ZoneIndexType CBridgeAgentImp::GetHorizInterfaceShearZoneIndexAtPoi(const pgsPointOfInterest& poi, const CShearData& rShearData)
{
   // NOTE: The logic here is identical to GetPrimaryShearZoneAtPoi
   //       If you fix a bug here, you need to fix it there also

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   ZoneIndexType nz = GetNumHorizInterfaceZones(span, gdr);
   if (nz==0)
      return INVALID_INDEX;

   Float64 glen = GetGirderLength(span,gdr);
   Float64 location = poi.GetDistFromStart();

   Float64 lft_brg_loc = GetGirderStartConnectionLength(span,gdr);
   Float64 rgt_brg_loc = glen - GetGirderEndConnectionLength(span,gdr);

   // use template function to do heavy work
   ZoneIndexType zone =  GetZoneIndexAtLocation(location, glen, lft_brg_loc, rgt_brg_loc, rShearData.bAreZonesSymmetrical, 
                                                rShearData.HorizontalInterfaceZones.begin(), rShearData.HorizontalInterfaceZones.end(), 
                                                rShearData.HorizontalInterfaceZones.size());

      return zone;
}

const CHorizontalInterfaceZoneData* CBridgeAgentImp::GetHorizInterfaceShearZoneDataAtPoi(const pgsPointOfInterest& poi, const CShearData& rShearData)
{
   ZoneIndexType idx = GetHorizInterfaceShearZoneIndexAtPoi(poi,rShearData);
   if (idx!=INVALID_INDEX)
   {
      return &rShearData.HorizontalInterfaceZones[idx];
   }
   else
   {
      return NULL;
   }
}

/////////////////////////////////////////////////////////////////////////
// IStrandGeometry
//
Float64 CBridgeAgentImp::GetEccentricity(const pgsPointOfInterest& poi,bool bIncTemp, Float64* nEffectiveStrands)
{
   VALIDATE( GIRDER );
   SpanIndexType spanIdx = poi.GetSpan();
   GirderIndexType gdrIdx = poi.GetGirder();

   CComPtr<IPrecastGirder> girder;
   GetGirder(spanIdx,gdrIdx,&girder);

   Float64 nStraight  = 0;
   Float64 nHarped    = 0;
   Float64 nTemporary = 0;

   Float64 ecc_straight  = 0;
   Float64 ecc_harped    = 0;
   Float64 ecc_temporary = 0;

   Float64 ecc = 0;

   ecc_straight = GetSsEccentricity(poi,&nStraight);
   ecc_harped   = GetHsEccentricity(poi,&nHarped);

   Float64 asStraight  = GetStrand(spanIdx,gdrIdx,pgsTypes::Straight)->GetNominalArea()*nStraight;
   Float64 asHarped    = GetStrand(spanIdx,gdrIdx,pgsTypes::Harped)->GetNominalArea()*nHarped;
   Float64 asTemporary = 0;

   if ( bIncTemp )
   {
      ecc_temporary = GetTempEccentricity(poi,&nTemporary);
      asTemporary = GetStrand(spanIdx,gdrIdx,pgsTypes::Temporary)->GetNominalArea()*nTemporary;
   }

   *nEffectiveStrands = nStraight + nHarped + nTemporary;
   if ( *nEffectiveStrands > 0.0)
   {
      ecc = (asStraight*ecc_straight + asHarped*ecc_harped + asTemporary*ecc_temporary) / (asStraight + asHarped + asTemporary);
   }
   else
   {
      ecc = 0.0;
   }

   return ecc;
}

Float64 CBridgeAgentImp::GetEccentricity(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType, Float64* nEffectiveStrands)
{
   Float64 e = -9999999999;
   switch( strandType )
   {
   case pgsTypes::Straight:
      e = GetSsEccentricity(poi,nEffectiveStrands);
      break;

   case pgsTypes::Harped:
      e = GetHsEccentricity(poi,nEffectiveStrands);
      break;
   
   case pgsTypes::Temporary:
      e = GetTempEccentricity(poi,nEffectiveStrands);
      break;
   }

   return e;
}

Float64 CBridgeAgentImp::GetHsEccentricity(const pgsPointOfInterest& poi, Float64* nEffectiveStrands)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   CComPtr<IPoint2dCollection> points;
   girder->get_HarpedStrandPositions(poi.GetDistFromStart(),&points);

   CollectionIndexType nStrandPoints;
   points->get_Count(&nStrandPoints);

   Float64 ecc=0.0;

   if (nStrandPoints==0)
   {
      *nEffectiveStrands = 0.0;
   }
   else
   {
      // must account for partial bonding if poi is near end of girder
      // NOTE: bond factor is 1.0 if at girder ends
      Float64 poi_loc = poi.GetDistFromStart();
      Float64 bond_factor = 1.0;

      GET_IFACE(IPrestressForce,pPrestress);
      Float64 xfer_length = pPrestress->GetXferLength(span,gdr,pgsTypes::Harped);

      if (poi_loc>0.0 && poi_loc<xfer_length)
      {
         bond_factor = poi_loc/xfer_length;
      }
      else
      {
         Float64 girder_length = GetGirderLength(span,gdr);

         if (poi_loc>girder_length-xfer_length && poi_loc<girder_length)
         {
            bond_factor = (girder_length-poi_loc)/xfer_length;
         }
      }

      *nEffectiveStrands = (Float64)nStrandPoints*bond_factor;

      if (0 < nStrandPoints)
      {
         Float64 cg=0.0;
         for (CollectionIndexType strandPointIdx = 0; strandPointIdx < nStrandPoints; strandPointIdx++)
         {
            CComPtr<IPoint2d> point;
            points->get_Item(strandPointIdx,&point);
            Float64 y;
            point->get_Y(&y);

            cg += y;
         }

         cg = cg / (Float64)nStrandPoints;

         Float64 Yb = GetYb(pgsTypes::CastingYard,poi);
         ecc = Yb-cg;
      }
   }
   
   return ecc;
}

Float64 CBridgeAgentImp::GetSsEccentricity(const pgsPointOfInterest& poi, Float64* nEffectiveStrands)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   StrandIndexType num_strands;
   girder->GetStraightStrandCount(&num_strands);
   if (0 < num_strands)
   {
      StrandIndexType num_bonded_strands = 0;
      Float64 num_eff_strands = 0.0; // weighted number of effective strands

      Float64 girder_length = GetGirderLength(span,gdr);

      Float64 dist = poi.GetDistFromStart();

      // treat comp at ends different than in interior
      Float64 cg = 0.0;
      if (IsZero(dist))
      {
         // we are at the girder left end. Treat all bonded strands as full strength
         for (StrandIndexType strandIndex = 0; strandIndex < num_strands; strandIndex++)
         {
            Float64 yloc, left_debond, right_debond;
            girder->GetStraightStrandDebondLengthByPositionIndex(strandIndex, &yloc, &left_debond,&right_debond);

            if (left_debond==0.0)
            {
               // strand is not debonded, we don't care by how much
               cg += yloc;
               num_bonded_strands++;
            }
         }

         num_eff_strands = (Float64)num_bonded_strands;
      }
      else if (IsEqual(dist,girder_length))
      {
         // we are at the girder right end. Treat all bonded strands as full strength
         for (StrandIndexType strandIndex = 0; strandIndex < num_strands; strandIndex++)
         {
            Float64 yloc, left_debond, right_debond;
            girder->GetStraightStrandDebondLengthByPositionIndex(strandIndex, &yloc, &left_debond,&right_debond);

            if (right_debond==0.0)
            {
               cg += yloc;
               num_bonded_strands++;
            }
         }

         num_eff_strands = (Float64)num_bonded_strands;
      }
      else
      {
         // at the girder interior, must deal with partial bonding
         GET_IFACE(IPrestressForce,pPrestress);
         Float64 xfer_length = pPrestress->GetXferLength(span,gdr,pgsTypes::Straight);

         for (StrandIndexType strandIndex = 0; strandIndex < num_strands; strandIndex++)
         {
            Float64 yloc, left_bond, right_bond;
            girder->GetStraightStrandBondedLengthByPositionIndex(strandIndex, dist, &yloc,&left_bond,&right_bond);

            Float64 bond_length = min(left_bond, right_bond);
            
            if (bond_length <= 0.0) 
            {
               ;// do nothing if bond length is zero
            }
            else if (bond_length<xfer_length)
            {
               Float64 factor = bond_length/xfer_length;
               cg += yloc * factor;
               num_eff_strands += factor;
            }
            else
            {
               cg += yloc;
               num_eff_strands += 1.0;
            }
         }
      }

      *nEffectiveStrands = num_eff_strands;

      if (num_eff_strands > 0.0)
      {
         cg = cg/num_eff_strands;

         Float64 Yb = GetYb(pgsTypes::CastingYard,poi);

         Float64 ecc = Yb - cg;
         return ecc;
      }
      else
      {
         // all strands debonded out
         return 0.0;
      }
   }
   else
   {
      // no strands
      *nEffectiveStrands = 0.0;
      return 0.0;
   }
}

Float64 CBridgeAgentImp::GetTempEccentricity(const pgsPointOfInterest& poi, Float64* nEffectiveStrands)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   Float64 poi_loc = poi.GetDistFromStart();

   CComPtr<IPoint2dCollection> points;
   girder->get_TempStrandPositions(poi_loc, &points);

   CollectionIndexType num_strand_positions;
   points->get_Count(&num_strand_positions);

   if (0 < num_strand_positions)
   {
      // must account for partial bonding if poi is near end of girder
      // NOTE: bond factor is 1.0 if at girder ends
      Float64 bond_factor = 1.0;

      GET_IFACE(IPrestressForce,pPrestress);
      Float64 xfer_length = pPrestress->GetXferLength(span,gdr,pgsTypes::Temporary);

      if (poi_loc>0.0 && poi_loc<xfer_length)
      {
         bond_factor = poi_loc/xfer_length;
      }
      else
      {
         Float64 girder_length = GetGirderLength(span,gdr);

         if (poi_loc>girder_length-xfer_length && poi_loc<girder_length)
         {
            bond_factor = (girder_length-poi_loc)/xfer_length;
         }
      }

      *nEffectiveStrands = num_strand_positions*bond_factor;

      Float64 cg=0.0;
      for (CollectionIndexType strandPositionIdx = 0; strandPositionIdx < num_strand_positions; strandPositionIdx++)
      {
         CComPtr<IPoint2d> point;
         points->get_Item(strandPositionIdx,&point);
         Float64 y;
         point->get_Y(&y);

         cg += y;
      }

      cg = cg / (Float64)num_strand_positions;

      Float64 Yb = GetYb(pgsTypes::CastingYard,poi);

      Float64 ecc = Yb-cg;
      return ecc;
   }
   else
   {
      *nEffectiveStrands = 0.0;
      return 0.0;
   }
}

Float64 CBridgeAgentImp::GetMaxStrandSlope(const pgsPointOfInterest& poi)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   Float64 slope;
   girder->ComputeMaxHarpedStrandSlope(poi.GetDistFromStart(),&slope);

   return slope;
}

Float64 CBridgeAgentImp::GetAvgStrandSlope(const pgsPointOfInterest& poi)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   Float64 slope;
   girder->ComputeAvgHarpedStrandSlope(poi.GetDistFromStart(),&slope);

   return slope;
}

Float64 CBridgeAgentImp::GetHsEccentricity(const pgsPointOfInterest& poi, const PRESTRESSCONFIG& rconfig, Float64* nEffectiveStrands)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();
   StrandIndexType Nhs = rconfig.GetNStrands(pgsTypes::Harped);

   ATLASSERT(rconfig.Debond[pgsTypes::Harped].empty()); // we don't deal with debonding of harped strands for design. WBFL needs to be updated

   VALIDATE( GIRDER );

   if (Nhs==0)
   {
      *nEffectiveStrands = 0.0;
      return 0.0;
   }
   else
   {
      CComPtr<IPrecastGirder> girder;
      GetGirder(span,gdr,&girder);

      // must account for partial bonding if poi is near end of girder
      // NOTE: bond factor is 1.0 if at girder ends
      Float64 poi_loc = poi.GetDistFromStart();
      Float64 bond_factor = 1.0;

      GET_IFACE(IPrestressForce,pPrestress);
      Float64 xfer_length = pPrestress->GetXferLength(span,gdr,pgsTypes::Harped);

      if (poi_loc>0.0 && poi_loc<xfer_length)
      {
         bond_factor = poi_loc/xfer_length;
      }
      else
      {
         Float64 girder_length = GetGirderLength(span,gdr);

         if (poi_loc>girder_length-xfer_length && poi_loc<girder_length)
         {
            bond_factor = (girder_length-poi_loc)/xfer_length;
         }
      }

      // Use wrapper to convert strand fill to IIndexArray
      CIndexArrayWrapper strand_fill(rconfig.GetStrandFill(pgsTypes::Harped));

      // save and restore precast girders shift values, before/after geting point locations
      Float64 t_end_shift, t_hp_shift;
      girder->get_HarpedStrandAdjustmentEnd(&t_end_shift);
      girder->get_HarpedStrandAdjustmentHP(&t_hp_shift);

      girder->put_HarpedStrandAdjustmentEnd(rconfig.EndOffset);
      girder->put_HarpedStrandAdjustmentHP(rconfig.HpOffset);

      CComPtr<IPoint2dCollection> points;
      girder->get_HarpedStrandPositionsEx(poi.GetDistFromStart(), &strand_fill, &points);

      girder->put_HarpedStrandAdjustmentEnd(t_end_shift);
      girder->put_HarpedStrandAdjustmentHP(t_hp_shift);

      Float64 ecc=0.0;

      // compute cg
      CollectionIndexType num_strand_positions;
      points->get_Count(&num_strand_positions);

      *nEffectiveStrands = bond_factor*num_strand_positions;

      ATLASSERT( Nhs == StrandIndexType(num_strand_positions));
      if (0 < num_strand_positions)
      {
         Float64 cg=0.0;
         for (CollectionIndexType strandPositionIdx = 0; strandPositionIdx < num_strand_positions; strandPositionIdx++)
         {
            CComPtr<IPoint2d> point;
            points->get_Item(strandPositionIdx,&point);
            Float64 y;
            point->get_Y(&y);

            cg += y;
         }

         cg = cg / (Float64)num_strand_positions;

         Float64 Yb = GetYb(pgsTypes::CastingYard,poi);
         ecc = Yb-cg;
      }

      return ecc;
   }
}

Float64 CBridgeAgentImp::GetSsEccentricity(const pgsPointOfInterest& poi, const PRESTRESSCONFIG& rconfig, Float64* nEffectiveStrands)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();
   StrandIndexType Nss = rconfig.GetNStrands(pgsTypes::Straight);

   VALIDATE( GIRDER );

   if (Nss == 0)
   {
      *nEffectiveStrands = 0;
      return 0.0;
   }
   else
   {
      CComPtr<IPrecastGirder> girder;
      GetGirder(span,gdr,&girder);

      Float64 girder_length = GetGirderLength(span,gdr);
      Float64 poi_loc = poi.GetDistFromStart();

      // transfer
      GET_IFACE(IPrestressForce,pPrestress);
      Float64 xfer_length = pPrestress->GetXferLength(span,gdr,pgsTypes::Straight);

      // debonding - Let's set up a mapping data structure to make dealing with debonding easier (eliminate searching)
      const DebondConfigCollection& rdebonds = rconfig.Debond[pgsTypes::Straight];

      std::vector<Int16> debond_map;
      debond_map.assign(Nss, -1); // strands not debonded have and index of -1
      Int16 dbinc=0;
      for (DebondConfigConstIterator idb=rdebonds.begin(); idb!=rdebonds.end(); idb++)
      {
         const DEBONDCONFIG& dbinfo = *idb;
         if ( dbinfo.strandIdx < Nss)
         {
            debond_map[dbinfo.strandIdx] = dbinc; 
         }
         else
         {
            ATLASSERT(0); // shouldn't be debonding any strands that don't exist
         }

         dbinc++;
      }

      // get all current straight strand locations
      const ConfigStrandFillVector& rfillvect = rconfig.GetStrandFill(pgsTypes::Straight);
      CIndexArrayWrapper strand_fill(rfillvect);

      CComPtr<IPoint2dCollection> points;
      girder->get_StraightStrandPositionsEx(poi.GetDistFromStart(), &strand_fill, &points);

      CollectionIndexType num_strand_positions;
      points->get_Count(&num_strand_positions);
      ATLASSERT( Nss == num_strand_positions);

      // loop over all strands, compute bonded length, and weighted cg of strands
      Float64 cg = 0.0;
      Float64 num_eff_strands = 0.0;

      // special cases are where POI is at girder ends. For this, weight strands fully
      if (IsZero(poi_loc))
      {
         // poi is at left end
         for (CollectionIndexType strandPositionIdx = 0; strandPositionIdx < num_strand_positions; strandPositionIdx++)
         {
            CComPtr<IPoint2d> point;
            points->get_Item(strandPositionIdx,&point);
            Float64 y;
            point->get_Y(&y);

            Int16 dbidx = debond_map[strandPositionIdx];
            if ( dbidx != -1)
            {
               // strand is debonded
               const DEBONDCONFIG& dbinfo = rdebonds[dbidx];
 
               if (dbinfo.LeftDebondLength==0.0)
               {
                  // left end is not debonded
                  num_eff_strands += 1.0;
                  cg += y;
               }
            }
            else
            {
               // not debonded, give full weight
               num_eff_strands += 1.0;
               cg += y;
            }
         }
      }
      else if (IsEqual(poi_loc, girder_length))
      {
         // poi is at right end
         for (CollectionIndexType strandPositionIdx = 0; strandPositionIdx < num_strand_positions; strandPositionIdx++)
         {
            CComPtr<IPoint2d> point;
            points->get_Item(strandPositionIdx,&point);
            Float64 y;
            point->get_Y(&y);

            Int16 dbidx = debond_map[strandPositionIdx];
            if ( dbidx != -1)
            {
               // strand is debonded
               const DEBONDCONFIG& dbinfo = rdebonds[dbidx];
 
               if (dbinfo.RightDebondLength==0.0)
               {
                  // right end is not debonded
                  num_eff_strands += 1.0;
                  cg += y;
               }
            }
            else
            {
               // not debonded, give full weight
               num_eff_strands += 1.0;
               cg += y;
            }
         }
      }
      else
      {
         // poi is between ends
         for (CollectionIndexType strandPositionIdx = 0; strandPositionIdx < num_strand_positions; strandPositionIdx++)
         {
            CComPtr<IPoint2d> point;
            points->get_Item(strandPositionIdx,&point);
            Float64 y;
            point->get_Y(&y);

            // bonded length
            Float64 lft_bond, rgt_bond;
            Int16 dbidx = debond_map[strandPositionIdx];
            if ( dbidx != -1)
            {
               const DEBONDCONFIG& dbinfo = rdebonds[dbidx];
 
               // strand is debonded
               lft_bond = poi_loc - dbinfo.LeftDebondLength;
               rgt_bond = girder_length - dbinfo.RightDebondLength - poi_loc;
            }
            else
            {
               lft_bond = poi_loc;
               rgt_bond = girder_length - poi_loc;
            }

            Float64 bond_len = min(lft_bond, rgt_bond);
            if (bond_len<=0.0)
            {
               // not bonded - do nothing
            }
            else if (bond_len < xfer_length)
            {
               Float64 bf = bond_len/xfer_length; // reduce strand weight for debonding
               num_eff_strands += bf;
               cg += y * bf;
            }
            else
            {
               // fully bonded
               num_eff_strands += 1.0;
               cg += y;
            }
         }
      }

      *nEffectiveStrands = num_eff_strands;

      if (num_eff_strands>0.0)
      {
         cg = cg/num_eff_strands;

         Float64 Yb = GetYb(pgsTypes::CastingYard,poi);

         Float64 ecc = Yb - cg;
         return ecc;
      }
      else
      {
         // all strands debonded out
         return 0.0;
      }
   }
}

Float64 CBridgeAgentImp::GetTempEccentricity(const pgsPointOfInterest& poi, const PRESTRESSCONFIG& rconfig, Float64* nEffectiveStrands)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   StrandIndexType Nts = rconfig.GetNStrands(pgsTypes::Temporary);

   ATLASSERT(rconfig.Debond[pgsTypes::Temporary].empty()); // we don't deal with debonding of temp strands for design. WBFL needs to be updated

   VALIDATE( GIRDER );

   if (Nts==0)
   {
      *nEffectiveStrands = 0.0;
      return 0.0;
   }
   else
   {
      CComPtr<IPrecastGirder> girder;
      GetGirder(span,gdr,&girder);

      // must account for partial bonding if poi is near end of girder
      // NOTE: bond factor is 1.0 if at girder ends
      Float64 poi_loc = poi.GetDistFromStart();
      Float64 bond_factor = 1.0;

      GET_IFACE(IPrestressForce,pPrestress);
      Float64 xfer_length = pPrestress->GetXferLength(span,gdr,pgsTypes::Temporary);

      if (poi_loc>0.0 && poi_loc<xfer_length)
      {
         bond_factor = poi_loc/xfer_length;
      }
      else
      {
         Float64 girder_length = GetGirderLength(span,gdr);

         if (poi_loc>girder_length-xfer_length && poi_loc<girder_length)
         {
            bond_factor = (girder_length-poi_loc)/xfer_length;
         }
      }

      // use continuous interface to set strands
      // Use wrapper to convert strand fill to IIndexArray
      CIndexArrayWrapper strand_fill(rconfig.GetStrandFill(pgsTypes::Temporary));

      CComPtr<IPoint2dCollection> points;
      girder->get_TempStrandPositionsEx(poi.GetDistFromStart(), &strand_fill, &points);

      Float64 ecc=0.0;

      // compute cg
      CollectionIndexType num_strands_positions;
      points->get_Count(&num_strands_positions);

      *nEffectiveStrands = bond_factor * num_strands_positions;

      ATLASSERT(Nts == StrandIndexType(num_strands_positions));
      Float64 cg=0.0;
      for (CollectionIndexType strandPositionIdx = 0; strandPositionIdx < num_strands_positions; strandPositionIdx++)
      {
         CComPtr<IPoint2d> point;
         points->get_Item(strandPositionIdx,&point);
         Float64 y;
         point->get_Y(&y);

         cg += y;
      }

      cg = cg / (Float64)num_strands_positions;

      Float64 Yb = GetYb(pgsTypes::CastingYard,poi);
      ecc = Yb-cg;

      return ecc;
   }
}

Float64 CBridgeAgentImp::GetEccentricity(const pgsPointOfInterest& poi, const PRESTRESSCONFIG& rconfig, bool bIncTemp, Float64* nEffectiveStrands)
{
   VALIDATE( GIRDER );
   SpanIndexType spanIdx = poi.GetSpan();
   GirderIndexType gdrIdx = poi.GetGirder();

   CComPtr<IPrecastGirder> girder;
   GetGirder(spanIdx,gdrIdx,&girder);

   Float64 dns = 0.0;
   Float64 dnh = 0.0;
   Float64 dnt = 0.0;

   Float64 eccs = this->GetSsEccentricity(poi, rconfig, &dns);
   Float64 ecch = this->GetHsEccentricity(poi, rconfig, &dnh);
   Float64 ecct = 0.0;

   Float64 aps[3];
   aps[pgsTypes::Straight]  = GetStrand(spanIdx,gdrIdx,pgsTypes::Straight)->GetNominalArea() * dns;
   aps[pgsTypes::Harped]    = GetStrand(spanIdx,gdrIdx,pgsTypes::Harped)->GetNominalArea()   * dnh;
   aps[pgsTypes::Temporary] = 0;

   if (bIncTemp)
   {
      ecct = this->GetTempEccentricity(poi, rconfig, &dnt);
      aps[pgsTypes::Temporary] = GetStrand(spanIdx,gdrIdx,pgsTypes::Temporary)->GetNominalArea()* dnt;
   }

   Float64 ecc=0.0;
   *nEffectiveStrands = dns + dnh + dnt;
   if (*nEffectiveStrands > 0)
   {
      ecc = (aps[pgsTypes::Straight]*eccs + aps[pgsTypes::Harped]*ecch + aps[pgsTypes::Temporary]*ecct)/(aps[pgsTypes::Straight] + aps[pgsTypes::Harped] + aps[pgsTypes::Temporary]);
   }

   return ecc;
}

Float64 CBridgeAgentImp::GetMaxStrandSlope(const pgsPointOfInterest& poi,const PRESTRESSCONFIG& rconfig)
{
   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(poi.GetSpan(),poi.GetGirder(),&girder);

   // Use wrapper to convert strand fill to IIndexArray
   CIndexArrayWrapper strand_fill(rconfig.GetStrandFill(pgsTypes::Harped));

   Float64 endShift = rconfig.EndOffset;
   Float64 hpShift  = rconfig.HpOffset;

   Float64 slope;
   girder->ComputeMaxHarpedStrandSlopeEx(poi.GetDistFromStart(),&strand_fill,endShift,hpShift,&slope);

   return slope;
}

Float64 CBridgeAgentImp::GetAvgStrandSlope(const pgsPointOfInterest& poi,const PRESTRESSCONFIG& rconfig)
{
   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(poi.GetSpan(),poi.GetGirder(),&girder);

   // use continuous interface to compute
   // Use wrapper to convert strand fill to IIndexArray
   CIndexArrayWrapper strand_fill(rconfig.GetStrandFill(pgsTypes::Harped));

   Float64 slope;
   girder->ComputeAvgHarpedStrandSlopeEx(poi.GetDistFromStart(),&strand_fill,rconfig.EndOffset,rconfig.HpOffset,&slope);

   return slope;
}

Float64 CBridgeAgentImp::GetApsBottomHalf(const pgsPointOfInterest& poi,DevelopmentAdjustmentType devAdjust)
{
   return GetApsTensionSide(poi, devAdjust, false );
}

Float64 CBridgeAgentImp::GetApsBottomHalf(const pgsPointOfInterest& poi, const GDRCONFIG& config,DevelopmentAdjustmentType devAdjust)
{
   return GetApsTensionSide(poi, config, devAdjust, false );
}

Float64 CBridgeAgentImp::GetApsTopHalf(const pgsPointOfInterest& poi,DevelopmentAdjustmentType devAdjust)
{
   return GetApsTensionSide(poi, devAdjust, true );
}

Float64 CBridgeAgentImp::GetApsTopHalf(const pgsPointOfInterest& poi, const GDRCONFIG& config,DevelopmentAdjustmentType devAdjust)
{
   return GetApsTensionSide(poi, config, devAdjust, true );
}

ConfigStrandFillVector CBridgeAgentImp::ComputeStrandFill(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type,StrandIndexType Ns)
{
   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(span, gdr,&girder);

   CContinuousStrandFiller* pFiller = GetContinuousStrandFiller(span, gdr);

   // Get fill in COM format
   CComPtr<IIndexArray> indexarr;
   switch( type )
   {
   case pgsTypes::Straight:
      pFiller->ComputeStraightStrandFill(girder, Ns, &indexarr);
      break;

   case pgsTypes::Harped:
      pFiller->ComputeHarpedStrandFill(girder, Ns, &indexarr);
      break;

   case pgsTypes::Temporary:
      pFiller->ComputeTemporaryStrandFill(girder, Ns, &indexarr);
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   // Convert to ConfigStrandFillVector
   ConfigStrandFillVector Vec;
   IndexArray2ConfigStrandFillVec(indexarr, Vec);

   return Vec;
}

ConfigStrandFillVector CBridgeAgentImp::ComputeStrandFill(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType Ns)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   CStrandFiller filler;
   filler.Init(pGdrEntry);

   // Get fill in COM format
   CComPtr<IIndexArray> indexarr;
   switch( type )
   {
   case pgsTypes::Straight:
      {
         CComPtr<IStrandGrid> startGrid, endGrid;
         startGrid.CoCreateInstance(CLSID_StrandGrid);
         endGrid.CoCreateInstance(CLSID_StrandGrid);
         pGdrEntry->ConfigureStraightStrandGrid(startGrid,endGrid);

         CComQIPtr<IStrandGridFiller> gridFiller(startGrid);
         filler.ComputeStraightStrandFill(gridFiller, Ns, &indexarr);
      }
      break;

   case pgsTypes::Harped:
      {
         CComPtr<IStrandGrid> startGrid, startHPGrid, endHPGrid, endGrid;
         startGrid.CoCreateInstance(CLSID_StrandGrid);
         startHPGrid.CoCreateInstance(CLSID_StrandGrid);
         endHPGrid.CoCreateInstance(CLSID_StrandGrid);
         endGrid.CoCreateInstance(CLSID_StrandGrid);
         pGdrEntry->ConfigureHarpedStrandGrids(startGrid,startHPGrid,endHPGrid,endGrid);

         CComQIPtr<IStrandGridFiller> endGridFiller(startGrid), hpGridFiller(startHPGrid);
         filler.ComputeHarpedStrandFill(pGdrEntry->OddNumberOfHarpedStrands(),endGridFiller, hpGridFiller, Ns, &indexarr);
      }
      break;

   case pgsTypes::Temporary:
      {
         CComPtr<IStrandGrid> startGrid, endGrid;
         startGrid.CoCreateInstance(CLSID_StrandGrid);
         endGrid.CoCreateInstance(CLSID_StrandGrid);
         pGdrEntry->ConfigureTemporaryStrandGrid(startGrid,endGrid);

         CComQIPtr<IStrandGridFiller> gridFiller(startGrid);
         filler.ComputeTemporaryStrandFill(gridFiller, Ns, &indexarr);
      }
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   // Convert IIndexArray to ConfigStrandFillVector
   ConfigStrandFillVector Vec;
   IndexArray2ConfigStrandFillVec(indexarr, Vec);

   return Vec;
}

GridIndexType CBridgeAgentImp::SequentialFillToGridFill(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType StrandNo)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   // Get fill in COM format
   StrandIndexType gridIdx(INVALID_INDEX);
   switch( type )
   {
   case pgsTypes::Straight:
      {
         CComPtr<IStrandGrid> startGrid, endGrid;
         startGrid.CoCreateInstance(CLSID_StrandGrid);
         endGrid.CoCreateInstance(CLSID_StrandGrid);
         pGdrEntry->ConfigureStraightStrandGrid(startGrid,endGrid);

         CComQIPtr<IStrandGridFiller> gridFiller(startGrid);

         CComPtr<IIndexArray> maxFill;
         gridFiller->GetMaxStrandFill(&maxFill); // sequential fill fills max from start

         gridFiller->StrandIndexToGridIndexEx(maxFill, StrandNo, &gridIdx);
      }
      break;

   case pgsTypes::Harped:
      {
         CComPtr<IStrandGrid> startGrid, startHPGrid, endHPGrid, endGrid;
         startGrid.CoCreateInstance(CLSID_StrandGrid);
         startHPGrid.CoCreateInstance(CLSID_StrandGrid);
         endHPGrid.CoCreateInstance(CLSID_StrandGrid);
         endGrid.CoCreateInstance(CLSID_StrandGrid);
         pGdrEntry->ConfigureHarpedStrandGrids(startGrid,startHPGrid,endHPGrid,endGrid);

         CComQIPtr<IStrandGridFiller> endGridFiller(startGrid);

         CComPtr<IIndexArray> maxFill;
         endGridFiller->GetMaxStrandFill(&maxFill); // sequential fill fills max from start

         endGridFiller->StrandIndexToGridIndexEx(maxFill, StrandNo, &gridIdx);
      }
      break;

   case pgsTypes::Temporary:
      {
         CComPtr<IStrandGrid> startGrid, endGrid;
         startGrid.CoCreateInstance(CLSID_StrandGrid);
         endGrid.CoCreateInstance(CLSID_StrandGrid);
         pGdrEntry->ConfigureTemporaryStrandGrid(startGrid,endGrid);

         CComQIPtr<IStrandGridFiller> gridFiller(startGrid);

         CComPtr<IIndexArray> maxFill;
         gridFiller->GetMaxStrandFill(&maxFill); // sequential fill fills max from start

         gridFiller->StrandIndexToGridIndexEx(maxFill, StrandNo, &gridIdx);
      }
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return gridIdx;
}

void CBridgeAgentImp::GridFillToSequentialFill(LPCTSTR strGirderName,pgsTypes::StrandType type,GridIndexType gridIdx, StrandIndexType* pStrandNo1, StrandIndexType* pStrandNo2)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   // Get fill in COM format
   HRESULT hr;
   switch( type )
   {
   case pgsTypes::Straight:
      {
         CComPtr<IStrandGrid> startGrid, endGrid;
         startGrid.CoCreateInstance(CLSID_StrandGrid);
         endGrid.CoCreateInstance(CLSID_StrandGrid);
         pGdrEntry->ConfigureStraightStrandGrid(startGrid,endGrid);

         CComQIPtr<IStrandGridFiller> gridFiller(startGrid);
         hr = gridFiller->GridIndexToStrandIndex(gridIdx, pStrandNo1, pStrandNo2);
         ATLASSERT(SUCCEEDED(hr));
      }
      break;

   case pgsTypes::Harped:
      {
         CComPtr<IStrandGrid> startGrid, startHPGrid, endHPGrid, endGrid;
         startGrid.CoCreateInstance(CLSID_StrandGrid);
         startHPGrid.CoCreateInstance(CLSID_StrandGrid);
         endHPGrid.CoCreateInstance(CLSID_StrandGrid);
         endGrid.CoCreateInstance(CLSID_StrandGrid);
         pGdrEntry->ConfigureHarpedStrandGrids(startGrid,startHPGrid,endHPGrid,endGrid);

         CComQIPtr<IStrandGridFiller> endGridFiller(startGrid);
         hr = endGridFiller->GridIndexToStrandIndex(gridIdx, pStrandNo1, pStrandNo2);
         ATLASSERT(SUCCEEDED(hr));
      }
      break;

   case pgsTypes::Temporary:
      {
         CComPtr<IStrandGrid> startGrid, endGrid;
         startGrid.CoCreateInstance(CLSID_StrandGrid);
         endGrid.CoCreateInstance(CLSID_StrandGrid);
         pGdrEntry->ConfigureTemporaryStrandGrid(startGrid,endGrid);

         CComQIPtr<IStrandGridFiller> gridFiller(startGrid);
         hr = gridFiller->GridIndexToStrandIndex(gridIdx, pStrandNo1, pStrandNo2);
         ATLASSERT(SUCCEEDED(hr));
      }
      break;

   default:
      ATLASSERT(false); // should never get here
   }
}

StrandIndexType CBridgeAgentImp::GetNumStrands(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type)
{
   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(span, gdr,&girder);

   StrandIndexType nStrands(0);
   if ( type == pgsTypes::Straight )
   {
      girder->GetStraightStrandCount(&nStrands);
   }
   else if ( type == pgsTypes::Harped )
   {
      girder->GetHarpedStrandCount(&nStrands);
   }
   else if ( type == pgsTypes::Temporary )
   {
      girder->GetTemporaryStrandCount(&nStrands);
   }
   else if ( type == pgsTypes::Permanent )
   {
      StrandIndexType nh, ns;
      girder->GetStraightStrandCount(&ns);
      girder->GetHarpedStrandCount(&nh);
      nStrands = ns + nh;
   }
   else
      ATLASSERT(0);

   return nStrands;

#pragma Reminder ("Remove this code after testing")
/*
   ATLASSERT(0); // Does this really need to get from bridgedesc?
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderData& girderData = pBridgeDesc->GetSpan(span)->GetGirderTypes()->GetGirderData(gdr);

   StrandIndexType nStrands;
   if ( type == pgsTypes::Permanent )
      nStrands = girderData.PrestressData.GetNstrands(pgsTypes::Straight) + girderData.PrestressData.GetNstrands(pgsTypes::Harped);
   else
      nStrands = girderData.PrestressData.GetNstrands(type);
*/
   return nStrands;
}

StrandIndexType CBridgeAgentImp::GetMaxStrands(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type)
{
   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   StrandIndexType Ns, Nh;
   StrandIndexType nStrands;
   switch( type )
   {
   case pgsTypes::Permanent:
      girder->get_MaxStraightStrands(&Ns);
      girder->get_MaxHarpedStrands(&Nh);
      nStrands = Ns + Nh;
      break;

   case pgsTypes::Straight:
      girder->get_MaxStraightStrands(&nStrands);
      break;

   case pgsTypes::Harped:
      girder->get_MaxHarpedStrands(&nStrands);
      break;

   case pgsTypes::Temporary:
      girder->get_MaxTemporaryStrands(&nStrands);
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return nStrands;
}

StrandIndexType CBridgeAgentImp::GetMaxStrands(LPCTSTR strGirderName,pgsTypes::StrandType type)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGirderEntry = pLib->GetGirderEntry( strGirderName );

   ATLASSERT(pGirderEntry != NULL);
   if ( pGirderEntry == NULL )
      return 0;

   StrandIndexType Ns, Nh;
   StrandIndexType nStrands;
   switch( type )
   {
   case pgsTypes::Permanent:
      Ns = pGirderEntry->GetMaxStraightStrands();
      Nh = pGirderEntry->GetMaxHarpedStrands();
      nStrands = Ns + Nh;
      break;

   case pgsTypes::Straight:
      nStrands = pGirderEntry->GetMaxStraightStrands();
      break;

   case pgsTypes::Harped:
      nStrands = pGirderEntry->GetMaxHarpedStrands();
      break;

   case pgsTypes::Temporary:
      nStrands = pGirderEntry->GetMaxTemporaryStrands();
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return nStrands;
}

Float64 CBridgeAgentImp::GetStrandArea(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type)
{
   StrandIndexType Ns = GetNumStrands(span,gdr,type);
   const matPsStrand* pStrand = GetStrand(span,gdr,type);

   Float64 aps = pStrand->GetNominalArea();
   Float64 Aps = aps * Ns;
   return Aps;
}

Float64 CBridgeAgentImp::GetAreaPrestressStrands(SpanIndexType span,GirderIndexType gdr,bool bIncTemp)
{
   Float64 Aps = GetStrandArea(span,gdr,pgsTypes::Straight) + GetStrandArea(span,gdr,pgsTypes::Harped);
   if ( bIncTemp )
      Aps += GetStrandArea(span,gdr,pgsTypes::Temporary);

   return Aps;
}

Float64 CBridgeAgentImp::GetPjack(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type)
{
   GET_IFACE(IGirderData,pGirderData);
   const CGirderData* pgirderData = pGirderData->GetGirderData(span,gdr);

   if ( type == pgsTypes::Permanent )
      return pgirderData->PrestressData.Pjack[pgsTypes::Straight] + pgirderData->PrestressData.Pjack[pgsTypes::Harped];
   else
      return pgirderData->PrestressData.Pjack[type];
}

Float64 CBridgeAgentImp::GetPjack(SpanIndexType span,GirderIndexType gdr,bool bIncTemp)
{
   Float64 Pj = GetPjack(span,gdr,pgsTypes::Straight) + GetPjack(span,gdr,pgsTypes::Harped);
   if ( bIncTemp )
      Pj += GetPjack(span,gdr,pgsTypes::Temporary);

   return Pj;
}

bool CBridgeAgentImp::GetAreHarpedStrandsForcedStraight(SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);

   return GetAreHarpedStrandsForcedStraightEx( pSpan->GetGirderTypes()->GetGirderName(gdr) );
}

bool CBridgeAgentImp::GetAreHarpedStrandsForcedStraightEx(LPCTSTR strGirderName)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGirderEntry = pLib->GetGirderEntry( strGirderName );
   return pGirderEntry->IsForceHarpedStrandsStraight();
}


Float64 CBridgeAgentImp::GetGirderTopElevation(SpanIndexType span,GirderIndexType gdr)
{
   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   Float64 top;
   girder->get_TopElevation(&top);

   return top;
}

Float64 CBridgeAgentImp::GetSplittingZoneHeight(const pgsPointOfInterest& poi)
{
   CComPtr<IGirderSection> girder_section;
   GetGirderSection(poi,&girder_section);

   CComQIPtr<IPrestressedGirderSection> psg_section(girder_section);

   Float64 splitting_zone_height;
   psg_section->get_SplittingZoneDimension(&splitting_zone_height);

   return splitting_zone_height;
}

pgsTypes::SplittingDirection CBridgeAgentImp::GetSplittingDirection(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   CComPtr<IGirderSection> girder_section;
   pgsPointOfInterest poi(spanIdx,gdrIdx,0.00);
   GetGirderSection(poi,&girder_section);

   CComQIPtr<IPrestressedGirderSection> psg_section(girder_section);
   SplittingDirection splitDir;
   psg_section->get_SplittingDirection(&splitDir);

   return (splitDir == sdVertical ? pgsTypes::sdVertical : pgsTypes::sdHorizontal);
}

void CBridgeAgentImp::GetHarpStrandOffsets(SpanIndexType span,GirderIndexType gdr,Float64* pOffsetEnd,Float64* pOffsetHp)
{
   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);
   girder->get_HarpedStrandAdjustmentEnd(pOffsetEnd);
   girder->get_HarpedStrandAdjustmentHP(pOffsetHp);
}

void CBridgeAgentImp::GetHarpedEndOffsetBounds(SpanIndexType span,GirderIndexType gdr,Float64* DownwardOffset, Float64* UpwardOffset)
{
   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   HRESULT hr = girder->GetHarpedEndAdjustmentBounds(DownwardOffset, UpwardOffset);
   ATLASSERT(SUCCEEDED(hr));
}

void CBridgeAgentImp::GetHarpedEndOffsetBoundsEx(SpanIndexType span,GirderIndexType gdr,const ConfigStrandFillVector& rHarpedFillArray, Float64* DownwardOffset, Float64* UpwardOffset)
{
   VALIDATE( GIRDER );

   if ( !AreStrandsInConfigFillVec(rHarpedFillArray) )
   {
      *DownwardOffset = 0.0;
      *UpwardOffset = 0.0;
   }
   else
   {
      CComPtr<IPrecastGirder> girder;
      GetGirder(span,gdr,&girder);

      // Use wrapper to convert strand fill to IIndexArray
      CIndexArrayWrapper strand_fill( rHarpedFillArray );

      HRESULT hr = girder->GetHarpedEndAdjustmentBoundsEx(&strand_fill,DownwardOffset, UpwardOffset);
      ATLASSERT(SUCCEEDED(hr));
   }
}

void CBridgeAgentImp::GetHarpedEndOffsetBoundsEx(LPCTSTR strGirderName, const ConfigStrandFillVector& rHarpedFillArray, Float64* DownwardOffset, Float64* UpwardOffset)
{
   if ( !AreStrandsInConfigFillVec(rHarpedFillArray) )
   {
      // no strands, just return
      *DownwardOffset = 0.0;
      *UpwardOffset = 0.0;
      return;
   }

   // Set up strand grid filler
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   CComPtr<IStrandGrid> startGrid, startHPGrid, endHPGrid, endGrid;
   startGrid.CoCreateInstance(CLSID_StrandGrid);
   startHPGrid.CoCreateInstance(CLSID_StrandGrid);
   endHPGrid.CoCreateInstance(CLSID_StrandGrid);
   endGrid.CoCreateInstance(CLSID_StrandGrid);
   pGdrEntry->ConfigureHarpedStrandGrids(startGrid,startHPGrid,endHPGrid,endGrid);

   CComQIPtr<IStrandGridFiller> startGridFiller(startGrid);

   // Use wrapper to convert strand fill to IIndexArray
   CIndexArrayWrapper fill(rHarpedFillArray);

   // get adjusted top and bottom bounds
   Float64 top, bottom;
   HRESULT hr = startGridFiller->get_FilledGridBoundsEx(&fill,&bottom,&top);
   ATLASSERT(SUCCEEDED(hr));

   Float64 adjust;
   hr = startGridFiller->get_VerticalStrandAdjustment(&adjust);
   ATLASSERT(SUCCEEDED(hr));

   if (bottom == 0.0 && top == 0.0)
   {
      // no strands exist so we cannot adjust them
      *DownwardOffset = 0.0;
      *UpwardOffset   = 0.0;
   }
   else
   {
      bottom -= adjust;
      top    -= adjust;

      // get max locations of strands
      Float64 bottom_min, top_max;
      CComPtr<IStrandMover> strandMover;
      CreateStrandMover(strGirderName,&strandMover);
      hr = strandMover->get_EndStrandElevationBoundaries(&bottom_min, &top_max);
      ATLASSERT(SUCCEEDED(hr));

      *DownwardOffset = bottom_min - bottom;
      *DownwardOffset = IsZero(*DownwardOffset) ? 0.0 : *DownwardOffset;

      *UpwardOffset   = top_max - top;
      *UpwardOffset   = IsZero(*UpwardOffset)   ? 0.0 : *UpwardOffset;

      // if these fire, strands cannot be adjusted within section bounds. this should be caught at library entry time.
      ATLASSERT(*DownwardOffset<1.0e-06);
      ATLASSERT(*UpwardOffset>-1.0e-06);
   }
}

void CBridgeAgentImp::GetHarpedHpOffsetBounds(SpanIndexType span,GirderIndexType gdr,Float64* DownwardOffset, Float64* UpwardOffset)
{
   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   HRESULT hr = girder->GetHarpedHpAdjustmentBounds(DownwardOffset, UpwardOffset);
   ATLASSERT(SUCCEEDED(hr));
}

void CBridgeAgentImp::GetHarpedHpOffsetBoundsEx(SpanIndexType span,GirderIndexType gdr,const ConfigStrandFillVector& rHarpedFillArray, Float64* DownwardOffset, Float64* UpwardOffset)
{
   VALIDATE( GIRDER );

   if ( !AreStrandsInConfigFillVec(rHarpedFillArray) )
   {
      *DownwardOffset = 0.0;
      *UpwardOffset = 0.0;
   }
   else
   {
      CComPtr<IPrecastGirder> girder;
      GetGirder(span,gdr,&girder);

      // Use wrapper to convert strand fill to IIndexArray
      CIndexArrayWrapper strand_fill( rHarpedFillArray );

      HRESULT hr = girder->GetHarpedHpAdjustmentBoundsEx(&strand_fill,DownwardOffset, UpwardOffset);
      ATLASSERT(SUCCEEDED(hr));
   }
}

void CBridgeAgentImp::GetHarpedHpOffsetBoundsEx(LPCTSTR strGirderName, const ConfigStrandFillVector& rHarpedFillArray, Float64* DownwardOffset, Float64* UpwardOffset)
{
   if ( !AreStrandsInConfigFillVec(rHarpedFillArray) )
   {
      *DownwardOffset = 0.0;
      *UpwardOffset = 0.0;
      return;
   }

   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   CComPtr<IStrandGrid> startGrid, startHPGrid, endHPGrid, endGrid;
   startGrid.CoCreateInstance(CLSID_StrandGrid);
   startHPGrid.CoCreateInstance(CLSID_StrandGrid);
   endHPGrid.CoCreateInstance(CLSID_StrandGrid);
   endGrid.CoCreateInstance(CLSID_StrandGrid);
   pGdrEntry->ConfigureHarpedStrandGrids(startGrid,startHPGrid,endHPGrid,endGrid);

   CComQIPtr<IStrandGridFiller> startGridFiller(startGrid);
   CComQIPtr<IStrandGridFiller> hpGridFiller(startHPGrid);

   // Use wrapper to convert strand fill to IIndexArray
   CIndexArrayWrapper fill(rHarpedFillArray);

   // get adjusted top and bottom bounds
   Float64 top, bottom;
   HRESULT hr = hpGridFiller->get_FilledGridBoundsEx(&fill,&bottom,&top);
   ATLASSERT(SUCCEEDED(hr));

   Float64 adjust;
   hr = hpGridFiller->get_VerticalStrandAdjustment(&adjust);
   ATLASSERT(SUCCEEDED(hr));

   if (bottom == 0.0 && top == 0.0)
   {
      // no strands exist so we cannot adjust them
      *DownwardOffset = 0.0;
      *UpwardOffset   = 0.0;
   }
   else
   {
      bottom -= adjust;
      top    -= adjust;

      // get max locations of strands
      Float64 bottom_min, top_max;
      CComPtr<IStrandMover> strandMover;
      CreateStrandMover(strGirderName,&strandMover);
      hr = strandMover->get_HpStrandElevationBoundaries(&bottom_min, &top_max);
      ATLASSERT(SUCCEEDED(hr));

      *DownwardOffset = bottom_min - bottom;
      *DownwardOffset = IsZero(*DownwardOffset) ? 0.0 : *DownwardOffset;

      *UpwardOffset   = top_max - top;
      *UpwardOffset   = IsZero(*UpwardOffset)   ? 0.0 : *UpwardOffset;

      // if these fire, strands cannot be adjusted within section bounds. this should be caught at library entry time.
      ATLASSERT(*DownwardOffset<1.0e-06);
      ATLASSERT(*UpwardOffset>-1.0e-06);
   }
}

Float64 CBridgeAgentImp::GetHarpedEndOffsetIncrement(SpanIndexType span,GirderIndexType gdr)
{
   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   Float64 increment;

   HRESULT hr = girder->get_HarpedEndAdjustmentIncrement(&increment);
   ATLASSERT(SUCCEEDED(hr));

   return increment;
}

Float64 CBridgeAgentImp::GetHarpedEndOffsetIncrement(LPCTSTR strGirderName)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGirderEntry = pLib->GetGirderEntry(strGirderName);
   Float64 end_increment = pGirderEntry->IsVerticalAdjustmentAllowedEnd() ?  pGirderEntry->GetEndStrandIncrement() : -1.0;
   Float64 hp_increment  = pGirderEntry->IsVerticalAdjustmentAllowedHP()  ?  pGirderEntry->GetHPStrandIncrement()  : -1.0;
   return end_increment;
}

Float64 CBridgeAgentImp::GetHarpedHpOffsetIncrement(SpanIndexType span,GirderIndexType gdr)
{
   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   Float64 increment;

   HRESULT hr = girder->get_HarpedHpAdjustmentIncrement(&increment);
   ATLASSERT(SUCCEEDED(hr));

   return increment;
}

Float64 CBridgeAgentImp::GetHarpedHpOffsetIncrement(LPCTSTR strGirderName)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGirderEntry = pLib->GetGirderEntry(strGirderName);
   Float64 end_increment = pGirderEntry->IsVerticalAdjustmentAllowedEnd() ?  pGirderEntry->GetEndStrandIncrement() : -1.0;
   Float64 hp_increment  = pGirderEntry->IsVerticalAdjustmentAllowedHP()  ?  pGirderEntry->GetHPStrandIncrement()  : -1.0;
   return hp_increment;
}

void CBridgeAgentImp::GetHarpingPointLocations(SpanIndexType span,GirderIndexType gdr,Float64* lhp,Float64* rhp)
{
   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);
   girder->GetHarpingPointLocations(lhp,rhp);
}

void CBridgeAgentImp::GetHighestHarpedStrandLocation(SpanIndexType span,GirderIndexType gdr,Float64* pElevation)
{
   // determine distance from bottom of girder to highest harped strand at end of girder 
   // to compute the txdot ibns TO value
   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   CComPtr<IRect2d> boxStart;
   HRESULT hr = girder->HarpedEndStrandBoundingBox(etStart,&boxStart);
   ATLASSERT(SUCCEEDED(hr));

   Float64 startTop;
   boxStart->get_Top(&startTop);

   CComPtr<IRect2d> boxEnd;
   hr = girder->HarpedEndStrandBoundingBox(etEnd,&boxEnd);
   ATLASSERT(SUCCEEDED(hr));

   Float64 endTop;
   boxEnd->get_Top(&endTop);

   *pElevation = max(startTop,endTop);
}

Uint16 CBridgeAgentImp::GetNumHarpPoints(SpanIndexType span,GirderIndexType gdr)
{
   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   StrandIndexType maxHarped;
   girder->get_MaxHarpedStrands(&maxHarped);

   if(maxHarped == 0)
      return 0;

   Float64 lhp, rhp;
   GetHarpingPointLocations(span, gdr, &lhp, &rhp);

   if (IsEqual(lhp,rhp))
      return 1;
   else
      return 2;
}


StrandIndexType CBridgeAgentImp::GetNextNumStrands(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type,StrandIndexType curNum)
{
   StrandIndexType ns;
   switch(type)
   {
   case pgsTypes::Straight:
      ns = GetNextNumStraightStrands(span,gdr,curNum);
      break;

   case pgsTypes::Harped:
      ns = GetNextNumHarpedStrands(span,gdr,curNum);
      break;

   case pgsTypes::Temporary:
      ns = GetNextNumTempStrands(span,gdr,curNum);
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return ns;
}

StrandIndexType CBridgeAgentImp::GetNextNumStrands(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType curNum)
{
   StrandIndexType ns;
   switch(type)
   {
   case pgsTypes::Straight:
      ns = GetNextNumStraightStrands(strGirderName,curNum);
      break;

   case pgsTypes::Harped:
      ns = GetNextNumHarpedStrands(strGirderName,curNum);
      break;

   case pgsTypes::Temporary:
      ns = GetNextNumTempStrands(strGirderName,curNum);
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return ns;
}

StrandIndexType CBridgeAgentImp::GetPrevNumStrands(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type,StrandIndexType curNum)
{
   VALIDATE( GIRDER );

   StrandIndexType ns;
   if (curNum==0)
   {
      ns = 0;
   }
   else
   {
      switch(type)
      {
      case pgsTypes::Straight:
         ns = GetPrevNumStraightStrands(span,gdr,curNum);
         break;

      case pgsTypes::Harped:
         ns = GetPrevNumHarpedStrands(span,gdr,curNum);
         break;

      case pgsTypes::Temporary:
         ns = GetPrevNumTempStrands(span,gdr,curNum);
         break;

      default:
         ATLASSERT(false); // should never get here
      }
   }

   return ns;
}

StrandIndexType CBridgeAgentImp::GetPrevNumStrands(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType curNum)
{
   if ( curNum == 0 )
      return 0;

   StrandIndexType ns;
   switch(type)
   {
   case pgsTypes::Straight:
      ns = GetPrevNumStraightStrands(strGirderName,curNum);
      break;

   case pgsTypes::Harped:
      ns = GetPrevNumHarpedStrands(strGirderName,curNum);
      break;

   case pgsTypes::Temporary:
      ns = GetPrevNumTempStrands(strGirderName,curNum);
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return ns;
}

StrandIndexType CBridgeAgentImp::GetNumExtendedStrands(SpanIndexType span,GirderIndexType gdr,pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType)
{
   GDRCONFIG config = GetGirderConfiguration(span,gdr);
   return config.PrestressConfig.GetExtendedStrands(strandType,endType).size();
}

bool CBridgeAgentImp::IsExtendedStrand(SpanIndexType span,GirderIndexType gdr,pgsTypes::MemberEndType end,StrandIndexType strandIdx,pgsTypes::StrandType strandType)
{
   GDRCONFIG config = GetGirderConfiguration(span,gdr);
   return IsExtendedStrand(span,gdr,end,strandIdx,strandType,config);
}

bool CBridgeAgentImp::IsExtendedStrand(SpanIndexType span,GirderIndexType gdr,pgsTypes::MemberEndType endType,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG& config)
{
   if ( config.PrestressConfig.GetExtendedStrands(strandType,endType).size() == 0 )
      return false;

   const std::vector<StrandIndexType>& extStrands = config.PrestressConfig.GetExtendedStrands(strandType,endType);
   std::vector<StrandIndexType>::const_iterator iter(extStrands.begin());
   std::vector<StrandIndexType>::const_iterator endIter(extStrands.end());
   for ( ; iter != endIter; iter++ )
   {
      if (*iter == strandIdx)
         return true;
   }

   return false;
}

bool CBridgeAgentImp::IsExtendedStrand(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType)
{
   Float64 Lg = GetSpanLength(poi.GetSpan(),poi.GetGirder());
   pgsTypes::MemberEndType end = (poi.GetDistFromStart() < Lg/2 ? pgsTypes::metStart : pgsTypes::metEnd);
   return IsExtendedStrand(poi.GetSpan(),poi.GetGirder(),end,strandIdx,strandType);
}

bool CBridgeAgentImp::IsExtendedStrand(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG& config)
{
   Float64 Lg = GetSpanLength(poi.GetSpan(),poi.GetGirder());
   pgsTypes::MemberEndType end = (poi.GetDistFromStart() < Lg/2 ? pgsTypes::metStart : pgsTypes::metEnd);
   return IsExtendedStrand(poi.GetSpan(),poi.GetGirder(),end,strandIdx,strandType,config);
}

bool CBridgeAgentImp::IsStrandDebonded(SpanIndexType span,GirderIndexType gdr,StrandIndexType strandIdx,
                                       pgsTypes::StrandType strandType,Float64* pStart,Float64* pEnd)
{
   GDRCONFIG config = GetGirderConfiguration(span,gdr);
   return IsStrandDebonded(span,gdr,strandIdx,strandType,config.PrestressConfig,pStart,pEnd);
}

bool CBridgeAgentImp::IsStrandDebonded(SpanIndexType span,GirderIndexType gdr,StrandIndexType strandIdx,
                                       pgsTypes::StrandType strandType,const PRESTRESSCONFIG& config,Float64* pStart,Float64* pEnd)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   Float64 length = GetGirderLength(span,gdr);

   bool bDebonded = false;
   *pStart = 0.0;
   *pEnd   = 0.0;

   DebondConfigConstIterator iter;
   for ( iter = config.Debond[strandType].begin(); iter != config.Debond[strandType].end(); iter++ )
   {
      const DEBONDCONFIG& di = *iter;
      if ( strandIdx == di.strandIdx )
      {
         *pStart = di.LeftDebondLength;
         *pEnd   = length - di.RightDebondLength;
         bDebonded = true;
         break;
      }
   }

   if ( bDebonded && (length < *pStart) )
      *pStart = length;

   if ( bDebonded && (length < *pEnd) )
      *pEnd = 0;

   // if not debonded, bond starts at 0 and ends at the other end of the girder
   if ( !bDebonded )
   {
      *pStart = 0;
      *pEnd = 0;
   }

   return bDebonded;
}

//-----------------------------------------------------------------------------
bool CBridgeAgentImp::IsStrandDebonded(const pgsPointOfInterest& poi,
                                       StrandIndexType strandIdx,
                                       pgsTypes::StrandType strandType)
{
   VALIDATE( GIRDER );

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   Float64 debond_left, debond_right;
   if ( !IsStrandDebonded(span,gdr,strandIdx,strandType,&debond_left,&debond_right) )
      return false;

   Float64 location = poi.GetDistFromStart();

   if ( location <= debond_left || debond_right <= location )
      return true;

   return false;
}

//-----------------------------------------------------------------------------
StrandIndexType CBridgeAgentImp::GetNumDebondedStrands(SpanIndexType span,
                                                       GirderIndexType gdr,
                                                       pgsTypes::StrandType strandType)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   StrandIndexType nDebondedStrands = 0;
   HRESULT hr;
   switch( strandType )
   {
   case pgsTypes::Permanent: // drop through
   case pgsTypes::Straight:
      hr = girder->GetStraightStrandDebondCount(&nDebondedStrands);
      break;

   case pgsTypes::Harped: // drop through

   case pgsTypes::Temporary:
      hr = S_FALSE; // Assumed only bonded for the end 10'... PS force is constant through the debonded section
                    // this is different than strands debonded at the ends and bonded in the middle
                    // Treat this strand as bonded
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return nDebondedStrands;
}

//-----------------------------------------------------------------------------
RowIndexType CBridgeAgentImp::GetNumRowsWithStrand(SpanIndexType span,
                                             GirderIndexType gdr,
                                             pgsTypes::StrandType strandType )
{
   VALIDATE( GIRDER );

   RowIndexType nRows = 0;

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   HRESULT hr;
   switch( strandType )
   {
   case pgsTypes::Straight:
      hr = girder->get_StraightStrandRowsWithStrand(&nRows);
      break;

   case pgsTypes::Harped:
      hr = girder->get_HarpedStrandRowsWithStrand(&nRows);
      break;

   case pgsTypes::Temporary:
      hr = S_FALSE; // Assumed only bonded for the end 10'... PS force is constant through the debonded section
                    // this is different than strands debonded at the ends and bonded in the middle
      break;        // Treat this strand as bonded

   default:
      ATLASSERT(false); // should never get here
   }

   return nRows;
}

//-----------------------------------------------------------------------------
StrandIndexType CBridgeAgentImp::GetNumStrandInRow(SpanIndexType span,
                                                   GirderIndexType gdr,
                                                   RowIndexType rowIdx,
                                                   pgsTypes::StrandType strandType )
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   StrandIndexType cStrands = 0;
   HRESULT hr;
   switch( strandType )
   {
   case pgsTypes::Straight:
      hr = girder->get_NumStraightStrandsInRow(rowIdx,&cStrands);
      break;

   case pgsTypes::Harped:
      hr = girder->get_NumHarpedStrandsInRow(rowIdx,&cStrands);
      break;

   case pgsTypes::Temporary:
      hr = S_FALSE; // Assumed only bonded for the end 10'... PS force is constant through the debonded section
                    // this is different than strands debonded at the ends and bonded in the middle
                    // Treat this strand as bonded
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return cStrands;
}

std::vector<StrandIndexType> CBridgeAgentImp::GetStrandsInRow(SpanIndexType span,GirderIndexType gdr, RowIndexType rowIdx, pgsTypes::StrandType strandType )
{
   std::vector<StrandIndexType> strandIdxs;
   if ( strandType == pgsTypes::Temporary )
   {
      ATLASSERT(false); // shouldn't get here
      return strandIdxs;
   }

   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   CComPtr<IIndexArray> array;
   if ( strandType == pgsTypes::Straight )
   {
      girder->get_StraightStrandsInRow(rowIdx,&array);
   }
   else
   {
      girder->get_HarpedStrandsInRow(rowIdx,&array);
   }

   CollectionIndexType nItems;

   array->get_Count(&nItems);
   for ( CollectionIndexType i = 0; i < nItems; i++ )
   {
      StrandIndexType strandIdx;
      array->get_Item(i,&strandIdx);
      strandIdxs.push_back(strandIdx);
   }

   return strandIdxs;
}

//-----------------------------------------------------------------------------
StrandIndexType CBridgeAgentImp::GetNumDebondedStrandsInRow(SpanIndexType span,
                                                            GirderIndexType gdr,
                                                            RowIndexType rowIdx,
                                                            pgsTypes::StrandType strandType )
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   StrandIndexType nDebondedStrands = 0;
   HRESULT hr;
   switch( strandType )
   {
   case pgsTypes::Straight:
      hr = girder->get_StraightStrandDebondInRow(rowIdx,&nDebondedStrands);
      break;

   case pgsTypes::Harped:
   case pgsTypes::Temporary:
      hr = S_FALSE; // Assumed only bonded for the end 10'... PS force is constant through the debonded section
                    // this is different than strands debonded at the ends and bonded in the middle
                    // Treat this strand as bonded
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return nDebondedStrands;
}

//-----------------------------------------------------------------------------
bool CBridgeAgentImp::IsExteriorStrandDebondedInRow(SpanIndexType span,
                                                    GirderIndexType gdr,
                                                    RowIndexType rowIdx,
                                                    pgsTypes::StrandType strandType)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   VARIANT_BOOL bExteriorDebonded = VARIANT_FALSE;
   HRESULT hr;
   switch( strandType )
   {
   case pgsTypes::Straight:
      hr = girder->IsExteriorStraightStrandDebondedInRow(rowIdx,&bExteriorDebonded);
      break;

   case pgsTypes::Harped:
   case pgsTypes::Temporary:
      hr = S_FALSE; // Assumed only bonded for the end 10'... PS force is constant through the debonded section
                    // this is different than strands debonded at the ends and bonded in the middle
                    // Treat this strand as bonded
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return bExteriorDebonded == VARIANT_FALSE ? false : true;
}

//-----------------------------------------------------------------------------
Float64 CBridgeAgentImp::GetDebondSection(SpanIndexType span,GirderIndexType gdr,GirderEnd end,SectionIndexType sectionIdx,pgsTypes::StrandType strandType)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   HRESULT hr;
   CComPtr<IDblArray> arrLeft, arrRight;
   switch( strandType )
   {
   case pgsTypes::Straight:
      hr = girder->GetStraightStrandDebondAtSections(&arrLeft,&arrRight);
      break;

   case pgsTypes::Harped:
   case pgsTypes::Temporary:
      hr = S_FALSE; // Assumed only bonded for the end 10'... PS force is constant through the debonded section
                    // this is different than strands debonded at the ends and bonded in the middle
                    // Treat this strand as bonded
      return 0;
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   Float64 location;
   if ( end == IStrandGeometry::geLeftEnd )
   {
      // left end
      arrLeft->get_Item(sectionIdx,&location);
   }
   else
   {
      // right
      arrRight->get_Item(sectionIdx,&location);

      // measure section location from the left end
      // so all return values are consistent
      Float64 gdr_length = GetGirderLength(span, gdr);
      location = gdr_length - location;
   }

   return location;
}

//-----------------------------------------------------------------------------
SectionIndexType CBridgeAgentImp::GetNumDebondSections(SpanIndexType span,GirderIndexType gdr,GirderEnd end,pgsTypes::StrandType strandType)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   HRESULT hr;
   CComPtr<IDblArray> arrLeft, arrRight;
   switch( strandType )
   {
   case pgsTypes::Straight:
      hr = girder->GetStraightStrandDebondAtSections(&arrLeft,&arrRight);
      break;

   case pgsTypes::Harped:
   case pgsTypes::Temporary:
      hr = S_FALSE; // Assumed only bonded for the end 10'... PS force is constant through the debonded section
                    // this is different than strands debonded at the ends and bonded in the middle
                    // Treat this strand as bonded
      return 0;
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   CollectionIndexType c1;
   if (end==IStrandGeometry::geLeftEnd)
   {
      arrLeft->get_Count(&c1);
   }
   else
   {
      arrRight->get_Count(&c1);
   }

   ASSERT(c1 <= Uint16_Max); // make sure there is no loss of data
   return Uint16(c1);
}

//-----------------------------------------------------------------------------
StrandIndexType CBridgeAgentImp::GetNumDebondedStrandsAtSection(SpanIndexType span,GirderIndexType gdr,GirderEnd end,SectionIndexType sectionIdx,pgsTypes::StrandType strandType)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   switch( strandType )
   {

   case pgsTypes::Harped:
   case pgsTypes::Temporary:
                    // Assumed only bonded for the end 10'... PS force is constant through the debonded section
                    // this is different than strands debonded at the ends and bonded in the middle
                    // Treat this strand as bonded
      return 0;
      break;

   case pgsTypes::Straight:
      break; // fall through to below


   default:
      ATLASSERT(false); // should never get here
      return 0;
   }

   HRESULT hr;
   CollectionIndexType nStrands;
   if (end == IStrandGeometry::geLeftEnd)
   {
      // left
      CComPtr<IIndexArray> strands;
      hr = girder->GetStraightStrandDebondAtLeftSection(sectionIdx,&strands);
      ATLASSERT(SUCCEEDED(hr));
      strands->get_Count(&nStrands);
   }
   else
   {
      CComPtr<IIndexArray> strands;
      hr = girder->GetStraightStrandDebondAtRightSection(sectionIdx,&strands);
      ATLASSERT(SUCCEEDED(hr));
      strands->get_Count(&nStrands);
   }

   return StrandIndexType(nStrands);
}

//-----------------------------------------------------------------------------
StrandIndexType CBridgeAgentImp::GetNumBondedStrandsAtSection(SpanIndexType span,GirderIndexType gdr,GirderEnd end,SectionIndexType sectionIdx,pgsTypes::StrandType strandType)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   StrandIndexType nStrands = GetNumStrands(span,gdr,strandType);

   HRESULT hr;
   CComPtr<IDblArray> arrLeft, arrRight;
   switch( strandType )
   {
   case pgsTypes::Straight:
      hr = girder->GetStraightStrandDebondAtSections(&arrLeft,&arrRight);
      ATLASSERT(SUCCEEDED(hr));
      break;

   case pgsTypes::Harped:
   case pgsTypes::Temporary:
                    // Assumed only bonded for the end 10'... PS force is constant through the debonded section
                    // this is different than strands debonded at the ends and bonded in the middle
                    // Treat this strand as bonded
      return 0;
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   // all strands are straight from here on
   StrandIndexType nDebondedStrands = 0;
   if (end == IStrandGeometry::geLeftEnd)
   {
      CollectionIndexType nSections;
      arrLeft->get_Count(&nSections);
      ATLASSERT(sectionIdx < nSections);

      // left
      // how many strands are debonded at this section and all the ones after it?
      for ( CollectionIndexType idx = sectionIdx; idx < nSections; idx++)
      {
         CComPtr<IIndexArray> strands;
         hr = girder->GetStraightStrandDebondAtLeftSection(idx,&strands);
         ATLASSERT(SUCCEEDED(hr));
         CollectionIndexType nDebondedStrandsAtSection;
         strands->get_Count(&nDebondedStrandsAtSection);

         nDebondedStrands += nDebondedStrandsAtSection;
      }
   }
   else
   {
      // right
      CollectionIndexType nSections;
      arrRight->get_Count(&nSections);
      ATLASSERT(sectionIdx < nSections);

      for ( CollectionIndexType idx = sectionIdx; idx < nSections; idx++ )
      {
         CComPtr<IIndexArray> strands;
         hr = girder->GetStraightStrandDebondAtRightSection(idx,&strands);
         ATLASSERT(SUCCEEDED(hr));
         CollectionIndexType nDebondedStrandsAtSection;
         strands->get_Count(&nDebondedStrandsAtSection);

         nDebondedStrands += nDebondedStrandsAtSection;
      }
   }

   ATLASSERT(nDebondedStrands<=nStrands);

   return nStrands - nDebondedStrands;
}

//-----------------------------------------------------------------------------
bool CBridgeAgentImp::CanDebondStrands(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::StrandType strandType)
{
   VALIDATE( BRIDGE );

   if ( strandType == pgsTypes::Harped || strandType == pgsTypes::Temporary )
      return false;

   GET_IFACE(ILibrary,pLib);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(spanIdx);
   std::_tstring strGirderName = pSpan->GetGirderTypes()->GetGirderName(gdrIdx);

   const GirderLibraryEntry* pGirderEntry = pLib->GetGirderEntry( strGirderName.c_str() );

   return pGirderEntry->CanDebondStraightStrands();
}

bool CBridgeAgentImp::CanDebondStrands(LPCTSTR strGirderName,pgsTypes::StrandType strandType)
{
   if ( strandType == pgsTypes::Harped || strandType == pgsTypes::Temporary )
      return false;

   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGirderEntry = pLib->GetGirderEntry( strGirderName );

   return pGirderEntry->CanDebondStraightStrands();
}

//-----------------------------------------------------------------------------
void CBridgeAgentImp::ListDebondableStrands(SpanIndexType span,GirderIndexType gdr,const ConfigStrandFillVector& rFillArray,pgsTypes::StrandType strandType, IIndexArray** list)
{
   GET_IFACE(ILibrary,pLib);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);
   std::_tstring strGirderName = pSpan->GetGirderTypes()->GetGirderName(gdr);

   ListDebondableStrands(strGirderName.c_str(),rFillArray,strandType,list);
}

void CBridgeAgentImp::ListDebondableStrands(LPCTSTR strGirderName,const ConfigStrandFillVector& rFillArray,pgsTypes::StrandType strandType, IIndexArray** list)
{
   CComPtr<IIndexArray> debondableStrands;  // array index = strand index, value = {0 means can't debond, 1 means can debond}
   debondableStrands.CoCreateInstance(CLSID_IndexArray);
   ATLASSERT(debondableStrands);

   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGirderEntry = pLib->GetGirderEntry( strGirderName );

   if ( strandType == pgsTypes::Straight)
   {
      ConfigStrandFillConstIterator it = rFillArray.begin();
      ConfigStrandFillConstIterator itend = rFillArray.end();

      GridIndexType gridIdx = 0;
      while(it != itend)
      {
         StrandIndexType nfill = *it;
         if (0 < nfill)
         {
            Float64 start_x, start_y, end_x, end_y;
            bool can_debond;
            pGirderEntry->GetStraightStrandCoordinates(gridIdx, &start_x, &start_y, &end_x, &end_y, &can_debond);

            debondableStrands->Add(can_debond?1:0);

            // have two strands?
            if (nfill==2)
            {
               ATLASSERT(0 < start_x && 0 < end_x);
               debondableStrands->Add(can_debond?1:0);
            }
         }

         gridIdx++;
         it++;
      }
   }
   else
   {
      // temp and harped. Redim zeros array
      debondableStrands->ReDim( PRESTRESSCONFIG::CountStrandsInFill(rFillArray) );
   }

   debondableStrands.CopyTo(list);
}

//-----------------------------------------------------------------------------
Float64 CBridgeAgentImp::GetDefaultDebondLength(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE(ILibrary,pLib);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(spanIdx);
   std::_tstring strGirderName = pSpan->GetGirderTypes()->GetGirderName(gdrIdx);

   const GirderLibraryEntry* pGirderEntry = pLib->GetGirderEntry( strGirderName.c_str() );

   return pGirderEntry->GetDefaultDebondSectionLength();
}

//-----------------------------------------------------------------------------
bool CBridgeAgentImp::IsDebondingSymmetric(SpanIndexType span,GirderIndexType gdr)
{
   StrandIndexType Ns = GetNumDebondedStrands(span,gdr,pgsTypes::Straight);
   StrandIndexType Nh = GetNumDebondedStrands(span,gdr,pgsTypes::Harped);
   StrandIndexType Nt = GetNumDebondedStrands(span,gdr,pgsTypes::Temporary);

   // if there are no debonded strands then get the heck outta here
   if ( Ns == 0 && Nh == 0 && Nt == 0)
      return true;

   Float64 length = GetGirderLength(span,gdr);

   // check the debonding to see if it is symmetric
   for ( int i = 0; i < 3; i++ )
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)(i);
      StrandIndexType nDebonded = GetNumDebondedStrands(span,gdr,strandType);
      if ( nDebonded == 0 )
         continue;

      StrandIndexType n = GetNumStrands(span,gdr,strandType);
      for ( StrandIndexType strandIdx = 0; strandIdx < n; strandIdx++ )
      {
         Float64 start, end;

         bool bIsDebonded = IsStrandDebonded(span,gdr,strandIdx,strandType,&start,&end);
         if ( bIsDebonded && !IsEqual(start,length-end) )
            return false;
      }
   }

   return true;
}

RowIndexType CBridgeAgentImp::GetNumRowsWithStrand(SpanIndexType span,GirderIndexType gdr, const PRESTRESSCONFIG& rconfig,pgsTypes::StrandType strandType )
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   CComPtr<IIndexArray> oldFill;

   RowIndexType nRows = 0;
   HRESULT hr;
   switch( strandType )
   {
   case pgsTypes::Straight:
      {
         CIndexArrayWrapper fill(rconfig.GetStrandFill(pgsTypes::Straight));
         girder->get_StraightStrandFill(&oldFill);
         girder->put_StraightStrandFill(&fill);
         hr = girder->get_StraightStrandRowsWithStrand(&nRows);
         girder->put_StraightStrandFill(oldFill);
      }
      break;

   case pgsTypes::Harped:
      {
         CIndexArrayWrapper fill(rconfig.GetStrandFill(pgsTypes::Harped));
         girder->get_HarpedStrandFill(&oldFill);
         girder->put_HarpedStrandFill(&fill);
         hr = girder->get_HarpedStrandRowsWithStrand(&nRows);
         girder->put_HarpedStrandFill(oldFill);
      }
      break;

   case pgsTypes::Temporary:
      hr = S_FALSE; // Assumed only bonded for the end 10'... PS force is constant through the debonded section
                    // this is different than strands debonded at the ends and bonded in the middle
                    // Treat this strand as bonded
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return nRows;
}

StrandIndexType CBridgeAgentImp::GetNumStrandInRow(SpanIndexType span,GirderIndexType gdr, const PRESTRESSCONFIG& rconfig,RowIndexType rowIdx,pgsTypes::StrandType strandType )
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   CComPtr<IIndexArray> oldFill;

   StrandIndexType cStrands = 0;
   HRESULT hr;
   switch( strandType )
   {
   case pgsTypes::Straight:
      {
         CIndexArrayWrapper fill(rconfig.GetStrandFill(pgsTypes::Straight));
         girder->get_StraightStrandFill(&oldFill);
         girder->put_StraightStrandFill(&fill);
         hr = girder->get_NumStraightStrandsInRow(rowIdx,&cStrands);
         girder->put_StraightStrandFill(oldFill);
      }
      break;

   case pgsTypes::Harped:
      {
         CIndexArrayWrapper fill(rconfig.GetStrandFill(pgsTypes::Harped));
         girder->get_HarpedStrandFill(&oldFill);
         girder->put_HarpedStrandFill(&fill);
         hr = girder->get_NumHarpedStrandsInRow(rowIdx,&cStrands);
         girder->put_HarpedStrandFill(oldFill);
      }
      break;

   case pgsTypes::Temporary:
      hr = S_FALSE; // Assumed only bonded for the end 10'... PS force is constant through the debonded section
                    // this is different than strands debonded at the ends and bonded in the middle
                    // Treat this strand as bonded
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return cStrands;
}

std::vector<StrandIndexType> CBridgeAgentImp::GetStrandsInRow(SpanIndexType span,GirderIndexType gdr, const PRESTRESSCONFIG& rconfig,RowIndexType rowIdx, pgsTypes::StrandType strandType )
{
   std::vector<StrandIndexType> strandIdxs;
   if ( strandType == pgsTypes::Temporary )
   {
      ATLASSERT(false); // shouldn't get here
      return strandIdxs;
   }

   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   CComPtr<IIndexArray> oldFill;
   CComPtr<IIndexArray> array;

   if ( strandType == pgsTypes::Straight )
   {
      CIndexArrayWrapper fill(rconfig.GetStrandFill(pgsTypes::Straight));
      girder->get_StraightStrandFill(&oldFill);
      girder->put_StraightStrandFill(&fill);
      girder->get_StraightStrandsInRow(rowIdx,&array);
      girder->put_StraightStrandFill(oldFill);
   }
   else
   {
      CIndexArrayWrapper fill(rconfig.GetStrandFill(pgsTypes::Harped));
      girder->get_HarpedStrandFill(&oldFill);
      girder->put_HarpedStrandFill(&fill);
      girder->get_HarpedStrandsInRow(rowIdx,&array);
      girder->put_HarpedStrandFill(oldFill);
   }

   CollectionIndexType nItems;

   array->get_Count(&nItems);
   for ( CollectionIndexType i = 0; i < nItems; i++ )
   {
      StrandIndexType strandIdx;
      array->get_Item(i,&strandIdx);
      strandIdxs.push_back(strandIdx);
   }

   return strandIdxs;
}

//-----------------------------------------------------------------------------
void CBridgeAgentImp::GetStrandPositions(const pgsPointOfInterest& poi, pgsTypes::StrandType type, IPoint2dCollection** ppPoints)
{
   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(poi.GetSpan(),poi.GetGirder(),&girder);

   HRESULT hr = S_OK;
   switch( type )
   {
   case pgsTypes::Straight:
      hr = girder->get_StraightStrandPositions(poi.GetDistFromStart(),ppPoints);
      break;

   case pgsTypes::Harped:
      hr = girder->get_HarpedStrandPositions(poi.GetDistFromStart(),ppPoints);
      break;

   case pgsTypes::Temporary:
      hr = girder->get_TempStrandPositions(poi.GetDistFromStart(),ppPoints);
      break;

   default:
      ATLASSERT(false); // shouldn't get here
   }

   ATLASSERT(SUCCEEDED(hr));
}


void CBridgeAgentImp::GetStrandPositionsEx(const pgsPointOfInterest& poi,const PRESTRESSCONFIG& rconfig, pgsTypes::StrandType type, IPoint2dCollection** ppPoints)
{
   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(poi.GetSpan(),poi.GetGirder(),&girder);

   CIndexArrayWrapper fill(rconfig.GetStrandFill(type));

   HRESULT hr = S_OK;
   switch( type )
   {
   case pgsTypes::Straight:
      hr = girder->get_StraightStrandPositionsEx(poi.GetDistFromStart(),&fill,ppPoints);
      break;

   case pgsTypes::Harped:
      hr = girder->get_HarpedStrandPositionsEx(poi.GetDistFromStart(),&fill,ppPoints);
      break;

   case pgsTypes::Temporary:
      hr = girder->get_TempStrandPositionsEx(poi.GetDistFromStart(),&fill,ppPoints);
      break;

   default:
      ATLASSERT(false); // shouldn't get here
   }

#ifdef _DEBUG
   CollectionIndexType np;
   (*ppPoints)->get_Count(&np);
   ATLASSERT(np==rconfig.GetNStrands(type));
#endif

   ATLASSERT(SUCCEEDED(hr));
}

void CBridgeAgentImp::GetStrandPositionsEx(LPCTSTR strGirderName,const PRESTRESSCONFIG& rconfig,
                                           pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType,
                                           IPoint2dCollection** ppPoints)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   CComPtr<IStrandGrid> startGrid, endGrid;
   startGrid.CoCreateInstance(CLSID_StrandGrid);
   endGrid.CoCreateInstance(CLSID_StrandGrid);

   CComQIPtr<IStrandGridFiller> gridFiller(startGrid);

   CIndexArrayWrapper fill(rconfig.GetStrandFill(strandType));

   HRESULT hr = S_OK;
   switch(strandType)
   {
   case pgsTypes::Straight:
      pGdrEntry->ConfigureStraightStrandGrid(startGrid,endGrid);
      gridFiller->GetStrandPositionsEx(&fill,ppPoints);
      break;

   case pgsTypes::Harped:
      {
      CComPtr<IStrandGrid> startHPGrid, endHPGrid;
      startHPGrid.CoCreateInstance(CLSID_StrandGrid);
      endHPGrid.CoCreateInstance(CLSID_StrandGrid);
      pGdrEntry->ConfigureHarpedStrandGrids(startGrid,startHPGrid,endHPGrid,endGrid);
      CComQIPtr<IStrandGridFiller> hpGridFiller(startHPGrid);
      gridFiller->GetStrandPositionsEx(&fill,ppPoints);
      }
      break;

   case pgsTypes::Temporary:
      pGdrEntry->ConfigureTemporaryStrandGrid(startGrid,endGrid);
      gridFiller->GetStrandPositionsEx(&fill,ppPoints);
      break;

   default:
      ATLASSERT(false); // is there a new strand type?
   }

   ATLASSERT(SUCCEEDED(hr));
}

Float64 CBridgeAgentImp::ComputeAbsoluteHarpedOffsetEnd(SpanIndexType span,GirderIndexType gdr, const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset)
{
   // returns the offset value measured from the original strand locations defined in the girder library
   // Up is positive
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);
   const CGirderTypes* pGirderTypes = pSpan->GetGirderTypes();
   const CGirderData& girderData = pGirderTypes->GetGirderData(gdr);
   Float64 absOffset = ComputeAbsoluteHarpedOffsetEnd(pGirderTypes->GetGirderName(gdr),rHarpedFillArray,measurementType,offset);

#if defined _DEBUG

//   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(span, gdr, &girder);

   Float64 increment; // if less than zero, strands cannot be adjusted
   girder->get_HarpedEndAdjustmentIncrement(&increment);

   Float64 result = 0;
   if (increment>=0.0 && AreStrandsInConfigFillVec(rHarpedFillArray))
   {
      if (measurementType==hsoLEGACY)
      {
         // legacy end offset moved top strand to highest location in strand grid and then measured down
         CComPtr<IStrandGrid> grid;
         girder->get_HarpedStrandGridEnd(etStart,&grid);

         CComPtr<IRect2d> grid_box;
         grid->GridBoundingBox(&grid_box);

         Float64 grid_bottom, grid_top;
         grid_box->get_Bottom(&grid_bottom);
         grid_box->get_Top(&grid_top);

         CIndexArrayWrapper fill(rHarpedFillArray);

         Float64 fill_top, fill_bottom;
         girder->GetHarpedEndFilledGridBoundsEx(&fill, &fill_bottom, &fill_top);

         Float64 vert_adjust;
         girder->get_HarpedStrandAdjustmentEnd(&vert_adjust);

         result = grid_top - (fill_top-vert_adjust) - offset;
      }
      else if (measurementType==hsoCGFROMTOP || measurementType==hsoCGFROMBOTTOM || measurementType==hsoECCENTRICITY)
      {
         // compute adjusted cg location
         CIndexArrayWrapper fill(rHarpedFillArray);

         CComPtr<IPoint2dCollection> points;
         girder->get_HarpedStrandPositionsEx(0.0, &fill, &points);

         Float64 cg=0.0;

         CollectionIndexType nStrands;
         points->get_Count(&nStrands);
         ATLASSERT(CountStrandsInConfigFillVec(rHarpedFillArray) == nStrands);
         for (CollectionIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++)
         {
            CComPtr<IPoint2d> point;
            points->get_Item(strandIdx,&point);
            Float64 y;
            point->get_Y(&y);

            cg += y;
         }

         cg = cg / (Float64)nStrands;

         // compute unadjusted location of cg
         Float64 vert_adjust;
         girder->get_HarpedStrandAdjustmentEnd(&vert_adjust);

         cg -= vert_adjust;

         if (measurementType==hsoCGFROMTOP)
         {
            Float64 height;
            girder->get_TopElevation(&height);

            Float64 dist = height - cg;
            result  = dist - offset;

         }
         else if ( measurementType==hsoCGFROMBOTTOM)
         {
            // bottom is a Y=0.0
            result =  offset - cg;
         }
         else if (measurementType==hsoECCENTRICITY)
         {
            Float64 Yb = GetYb(pgsTypes::CastingYard,pgsPointOfInterest(span,gdr,0.0));
            Float64 ecc = Yb-cg;

            result = ecc - offset;
         }
      }
      else if (measurementType==hsoTOP2TOP || measurementType==hsoTOP2BOTTOM)
      {
         // get strand grid positions that are filled by Nh strands
         CIndexArrayWrapper fill(rHarpedFillArray);

         // fill_top is the top of the strand positions that are actually filled
         // adjusted by the harped strand adjustment
         Float64 fill_top, fill_bottom;
         girder->GetHarpedEndFilledGridBoundsEx(&fill, &fill_bottom, &fill_top);

         // get the harped strand adjustment so its effect can be removed
         Float64 vert_adjust;
         girder->get_HarpedStrandAdjustmentEnd(&vert_adjust);

         // elevation of the top of the strand grid without an offset
         Float64 toploc = fill_top-vert_adjust;

         if (measurementType==hsoTOP2TOP)
         {
            Float64 height;
            girder->get_TopElevation(&height);

            // distance from the top of the girder to the top of the strands before offset
            Float64 dist = height - toploc;

            // distance from the top of the girder to the top of the strands after offset
            result  = dist - offset;
         }
         else  // measurementType==hsoTOP2BOTTOM
         {
            result =  offset - toploc;
         }
      }
      else if (measurementType==hsoBOTTOM2BOTTOM)
      {
         CIndexArrayWrapper fill(rHarpedFillArray);

         Float64 fill_top, fill_bottom;
         girder->GetHarpedEndFilledGridBoundsEx(&fill, &fill_bottom, &fill_top);

         Float64 vert_adjust;
         girder->get_HarpedStrandAdjustmentEnd(&vert_adjust);

         Float64 botloc = fill_bottom-vert_adjust;

         result =  offset - botloc;
      }
      else
      {
         ATLASSERT(0);
      }
    }

   ATLASSERT(IsEqual(result,absOffset));
#endif // _DEBUG
   return absOffset;
}

Float64 CBridgeAgentImp::ComputeAbsoluteHarpedOffsetEnd(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset)
{
   // returns the offset value measured from the original strand locations defined in the girder library
   // Up is positive
   if ( !AreStrandsInConfigFillVec(rHarpedFillArray) )
      return 0.0;

   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   if (!pGdrEntry->IsVerticalAdjustmentAllowedEnd())
   {
      return 0.0; // No offset allowed
   }

   CComPtr<IStrandMover> pStrandMover;
   CreateStrandMover(strGirderName,&pStrandMover);

   CComPtr<IStrandGrid> startGrid, startHPGrid, endHPGrid, endGrid;
   startGrid.CoCreateInstance(CLSID_StrandGrid);
   startHPGrid.CoCreateInstance(CLSID_StrandGrid);
   endHPGrid.CoCreateInstance(CLSID_StrandGrid);
   endGrid.CoCreateInstance(CLSID_StrandGrid);
   pGdrEntry->ConfigureHarpedStrandGrids(startGrid,startHPGrid,endHPGrid,endGrid);

   CIndexArrayWrapper fill(rHarpedFillArray);

   Float64 absOffset = 0;
   if (measurementType==hsoLEGACY)
   {
      // legacy end offset moved top strand to highest location in strand grid and then measured down
      CComPtr<IRect2d> grid_box;
      startGrid->GridBoundingBox(&grid_box);

      Float64 grid_bottom, grid_top;
      grid_box->get_Bottom(&grid_bottom);
      grid_box->get_Top(&grid_top);

      CComQIPtr<IStrandGridFiller> startGridFiller(startGrid);
      CComQIPtr<IStrandGridFiller> hpGridFiller(startHPGrid);

      Float64 fill_top, fill_bottom;
      startGridFiller->get_FilledGridBoundsEx(&fill, &fill_bottom, &fill_top);

      Float64 vert_adjust;
      startGridFiller->get_VerticalStrandAdjustment(&vert_adjust);

      absOffset = grid_top - (fill_top-vert_adjust) - offset;
   }
   else if (measurementType==hsoCGFROMTOP || measurementType==hsoCGFROMBOTTOM || measurementType==hsoECCENTRICITY)
   {
      // compute adjusted cg location
      CComQIPtr<IStrandGridFiller> startGridFiller(startGrid);
      CComQIPtr<IStrandGridFiller> hpGridFiller(startHPGrid);

      CComPtr<IPoint2dCollection> points;
      startGridFiller->GetStrandPositionsEx(&fill,&points);

      Float64 cg=0.0;

      CollectionIndexType nStrands;
      points->get_Count(&nStrands);
      ATLASSERT(CountStrandsInConfigFillVec(rHarpedFillArray) == nStrands);
      for (CollectionIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++)
      {
         CComPtr<IPoint2d> point;
         points->get_Item(strandIdx,&point);
         Float64 y;
         point->get_Y(&y);

         cg += y;
      }

      cg = cg / (Float64)nStrands;

      // compute unadjusted location of cg
      Float64 vert_adjust;
      startGridFiller->get_VerticalStrandAdjustment(&vert_adjust);

      cg -= vert_adjust;

      if (measurementType==hsoCGFROMTOP)
      {
         Float64 height;
         pStrandMover->get_TopElevation(&height);

         Float64 dist = height - cg;
         absOffset  = dist - offset;

      }
      else if ( measurementType==hsoCGFROMBOTTOM)
      {
         // bottom is a Y=0.0
         absOffset =  offset - cg;
      }
      else if (measurementType==hsoECCENTRICITY)
      {
         CComPtr<IBeamFactory> factory;
         pGdrEntry->GetBeamFactory(&factory);

         CComPtr<IGirderSection> gdrSection;
         factory->CreateGirderSection(m_pBroker,-1,INVALID_INDEX,INVALID_INDEX,pGdrEntry->GetDimensions(),&gdrSection);

         CComQIPtr<IShape> shape(gdrSection);
         CComPtr<IShapeProperties> props;
         shape->get_ShapeProperties(&props);
         Float64 Yb;
         props->get_Ybottom(&Yb);

         Float64 ecc = Yb-cg;

         absOffset = ecc - offset;
      }
   }
   else if (measurementType==hsoTOP2TOP || measurementType==hsoTOP2BOTTOM)
   {
      // get strand grid positions that are filled by Nh strands
      CComQIPtr<IStrandGridFiller> startGridFiller(startGrid);
      CComQIPtr<IStrandGridFiller> hpGridFiller(startHPGrid);

      // fill_top is the top of the strand positions that are actually filled
      // adjusted by the harped strand adjustment
      Float64 fill_top, fill_bottom;
      startGridFiller->get_FilledGridBoundsEx(&fill, &fill_bottom, &fill_top);

      // get the harped strand adjustment so its effect can be removed
      Float64 vert_adjust;
      startGridFiller->get_VerticalStrandAdjustment(&vert_adjust);

      // elevation of the top of the strand grid without an offset
      Float64 toploc = fill_top-vert_adjust;

      if (measurementType==hsoTOP2TOP)
      {
         Float64 height;
         pStrandMover->get_TopElevation(&height);

         // distance from the top of the girder to the top of the strands before offset
         Float64 dist = height - toploc;

         // distance from the top of the girder to the top of the strands after offset
         absOffset  = dist - offset;
      }
      else  // measurementType==hsoTOP2BOTTOM
      {
         absOffset =  offset - toploc;
      }
   }
   else if (measurementType==hsoBOTTOM2BOTTOM)
   {
      CComQIPtr<IStrandGridFiller> startGridFiller(startGrid);
      CComQIPtr<IStrandGridFiller> hpGridFiller(startHPGrid);

      Float64 fill_top, fill_bottom;
      startGridFiller->get_FilledGridBoundsEx(&fill,&fill_bottom,&fill_top);

      Float64 vert_adjust;
      startGridFiller->get_VerticalStrandAdjustment(&vert_adjust);

      Float64 botloc = fill_bottom-vert_adjust;

      absOffset =  offset - botloc;
   }
   else
   {
      ATLASSERT(0);
   }

   return absOffset;
}

Float64 CBridgeAgentImp::ComputeAbsoluteHarpedOffsetHp(SpanIndexType span,GirderIndexType gdr, const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset)
{
   // returns the offset value measured from the original strand locations defined in the girder library
   // Up is positive
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);
   const CGirderTypes* pGirderTypes = pSpan->GetGirderTypes();
   const CGirderData& girderData = pGirderTypes->GetGirderData(gdr);
   Float64 absOffset = ComputeAbsoluteHarpedOffsetHp(pGirderTypes->GetGirderName(gdr),rHarpedFillArray,measurementType,offset);

#if defined _DEBUG

//   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(span, gdr, &girder);

   Float64 increment; // if less than zero, strands cannot be adjusted
   girder->get_HarpedHpAdjustmentIncrement(&increment);

   Float64 result = 0;
   if ( increment>=0.0 && AreStrandsInConfigFillVec(rHarpedFillArray) )
   {
      if (measurementType==hsoLEGACY)
      {
         result = offset;
      }
      else if (measurementType==hsoCGFROMTOP || measurementType==hsoCGFROMBOTTOM || measurementType==hsoECCENTRICITY)
      {
         // compute adjusted cg location
         CIndexArrayWrapper fill(rHarpedFillArray);

         Float64 L = GetGirderLength(span,gdr); 

         CComPtr<IPoint2dCollection> points;
         girder->get_HarpedStrandPositionsEx(L/2.0, &fill, &points);

         Float64 cg=0.0;

         CollectionIndexType nStrands;
         points->get_Count(&nStrands);
         ATLASSERT(CountStrandsInConfigFillVec(rHarpedFillArray) == nStrands);
         for (CollectionIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++)
         {
            CComPtr<IPoint2d> point;
            points->get_Item(strandIdx,&point);
            Float64 y;
            point->get_Y(&y);

            cg += y;
         }

         cg = cg / (Float64)nStrands;

         // compute unadjusted location of cg
         Float64 vert_adjust;
         girder->get_HarpedStrandAdjustmentHP(&vert_adjust);

         cg -= vert_adjust;

         if (measurementType==hsoCGFROMTOP)
         {
            Float64 height;
            girder->get_TopElevation(&height);

            Float64 dist = height - cg;
            result  = dist - offset;

         }
         else if ( measurementType==hsoCGFROMBOTTOM)
         {
            // bottom is a Y=0.0
            result =  offset - cg;
         }
         else if (measurementType==hsoECCENTRICITY)
         {
            Float64 Yb = GetYb(pgsTypes::CastingYard,pgsPointOfInterest(span,gdr,0.0));
            Float64 ecc = Yb-cg;

            result = ecc - offset;
         }
      }
      else if (measurementType==hsoTOP2TOP || measurementType==hsoTOP2BOTTOM)
      {
         // get location of highest strand at zero offset
         CIndexArrayWrapper fill(rHarpedFillArray);

         Float64 fill_top, fill_bottom;
         girder->GetHarpedHpFilledGridBoundsEx(&fill, &fill_bottom, &fill_top);

         Float64 vert_adjust;
         girder->get_HarpedStrandAdjustmentHP(&vert_adjust);

         Float64 toploc = fill_top-vert_adjust;

         if (measurementType==hsoTOP2TOP)
         {
            Float64 height;
            girder->get_TopElevation(&height);

            Float64 dist = height - toploc;
            result  = dist - offset;
         }
         else  // measurementType==hsoTOP2BOTTOM
         {
            result =  offset - toploc;
         }
      }
      else if (measurementType==hsoBOTTOM2BOTTOM)
      {
         CIndexArrayWrapper fill(rHarpedFillArray);

         Float64 fill_top, fill_bottom;
         girder->GetHarpedHpFilledGridBoundsEx(&fill, &fill_bottom, &fill_top);

         Float64 vert_adjust;
         girder->get_HarpedStrandAdjustmentHP(&vert_adjust);

         Float64 botloc = fill_bottom-vert_adjust;

         result =  offset - botloc;
      }
      else
      {
         ATLASSERT(0);
      }
   }

   ATLASSERT(IsEqual(result,absOffset));
#endif // __DEBUG

   return absOffset;
}

Float64 CBridgeAgentImp::ComputeAbsoluteHarpedOffsetHp(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset)
{
   // returns the offset value measured from the original strand locations defined in the girder library
   // Up is positive
   if ( !AreStrandsInConfigFillVec(rHarpedFillArray) )
      return 0;

   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   if (!pGdrEntry->IsVerticalAdjustmentAllowedHP())
   {
      return 0.0; // No offset allowed
   }

   CComPtr<IStrandMover> pStrandMover;
   CreateStrandMover(strGirderName,&pStrandMover);

   CComPtr<IStrandGrid> startGrid, startHPGrid, endHPGrid, endGrid;
   startGrid.CoCreateInstance(CLSID_StrandGrid);
   startHPGrid.CoCreateInstance(CLSID_StrandGrid);
   endHPGrid.CoCreateInstance(CLSID_StrandGrid);
   endGrid.CoCreateInstance(CLSID_StrandGrid);
   pGdrEntry->ConfigureHarpedStrandGrids(startGrid,startHPGrid,endHPGrid,endGrid);

   Float64 absOffset = 0;
   if (measurementType==hsoLEGACY)
   {
      absOffset = offset;
   }
   else if (measurementType==hsoCGFROMTOP || measurementType==hsoCGFROMBOTTOM || measurementType==hsoECCENTRICITY)
   {
      // compute adjusted cg location
      CComQIPtr<IStrandGridFiller> hpGridFiller(startHPGrid);

      CIndexArrayWrapper fill(rHarpedFillArray);

      CComPtr<IPoint2dCollection> points;
      hpGridFiller->GetStrandPositionsEx(&fill,&points);

      Float64 cg=0.0;

      CollectionIndexType nStrands;
      points->get_Count(&nStrands);
      ATLASSERT(CountStrandsInConfigFillVec(rHarpedFillArray) == nStrands);
      for (CollectionIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++)
      {
         CComPtr<IPoint2d> point;
         points->get_Item(strandIdx,&point);
         Float64 y;
         point->get_Y(&y);

         cg += y;
      }

      cg = cg / (Float64)nStrands;

      // compute unadjusted location of cg
      Float64 vert_adjust;
      hpGridFiller->get_VerticalStrandAdjustment(&vert_adjust);

      cg -= vert_adjust;

      if (measurementType==hsoCGFROMTOP)
      {
         Float64 height;
         pStrandMover->get_TopElevation(&height);

         Float64 dist = height - cg;
         absOffset  = dist - offset;

      }
      else if ( measurementType==hsoCGFROMBOTTOM)
      {
         // bottom is a Y=0.0
         absOffset =  offset - cg;
      }
      else if (measurementType==hsoECCENTRICITY)
      {
         CComPtr<IBeamFactory> factory;
         pGdrEntry->GetBeamFactory(&factory);

         CComPtr<IGirderSection> gdrSection;
         factory->CreateGirderSection(m_pBroker,INVALID_ID,INVALID_INDEX,INVALID_INDEX,pGdrEntry->GetDimensions(),&gdrSection);

         CComQIPtr<IShape> shape(gdrSection);
         CComPtr<IShapeProperties> props;
         shape->get_ShapeProperties(&props);
         Float64 Yb;
         props->get_Ybottom(&Yb);

         Float64 ecc = Yb-cg;

         absOffset = ecc - offset;
      }
   }
   else if (measurementType==hsoTOP2TOP || measurementType==hsoTOP2BOTTOM)
   {
      // get location of highest strand at zero offset
      CComQIPtr<IStrandGridFiller> hpGridFiller(startHPGrid);

      CIndexArrayWrapper fill(rHarpedFillArray);

      Float64 fill_top, fill_bottom;
      hpGridFiller->get_FilledGridBoundsEx(&fill,&fill_bottom,&fill_top);

      Float64 vert_adjust;
      hpGridFiller->get_VerticalStrandAdjustment(&vert_adjust);

      Float64 toploc = fill_top-vert_adjust;

      if (measurementType==hsoTOP2TOP)
      {
         Float64 height;
         pStrandMover->get_TopElevation(&height);

         Float64 dist = height - toploc;
         absOffset  = dist - offset;
      }
      else  // measurementType==hsoTOP2BOTTOM
      {
         absOffset =  offset - toploc;
      }
   }
   else if (measurementType==hsoBOTTOM2BOTTOM)
   {
      CComQIPtr<IStrandGridFiller> hpGridFiller(startHPGrid);

      CIndexArrayWrapper fill(rHarpedFillArray);

      Float64 fill_top, fill_bottom;
      hpGridFiller->get_FilledGridBoundsEx(&fill,&fill_bottom,&fill_top);

      Float64 vert_adjust;
      hpGridFiller->get_VerticalStrandAdjustment(&vert_adjust);

      Float64 botloc = fill_bottom-vert_adjust;

      absOffset =  offset - botloc;
   }
   else
   {
      ATLASSERT(0);
   }

   return absOffset;
}

Float64 CBridgeAgentImp::ComputeHarpedOffsetFromAbsoluteEnd(SpanIndexType span,GirderIndexType gdr, const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset)
{
   // all we really need to know is the distance and direction between the coords, compute absolute
   // from zero
   Float64 absol = ComputeAbsoluteHarpedOffsetEnd(span, gdr, rHarpedFillArray, measurementType, 0.0);

   Float64 off = 0.0;
   // direction depends if meassured from bottom up or top down
   if (measurementType==hsoLEGACY || measurementType==hsoCGFROMTOP || measurementType==hsoTOP2TOP
      || measurementType==hsoECCENTRICITY)
   {
      off = absol - absoluteOffset ;
   }
   else if (measurementType==hsoCGFROMBOTTOM || measurementType==hsoTOP2BOTTOM || 
            measurementType==hsoBOTTOM2BOTTOM)
   {
      off = absoluteOffset - absol;
   }
   else
      ATLASSERT(0); 

   return off;
}

Float64 CBridgeAgentImp::ComputeHarpedOffsetFromAbsoluteEnd(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset)
{
   // all we really need to know is the distance and direction between the coords, compute absolute
   // from zero
   Float64 absol = ComputeAbsoluteHarpedOffsetEnd(strGirderName, rHarpedFillArray, measurementType, 0.0);

   Float64 off = 0.0;
   // direction depends if meassured from bottom up or top down
   if (measurementType==hsoLEGACY || measurementType==hsoCGFROMTOP || measurementType==hsoTOP2TOP
      || measurementType==hsoECCENTRICITY)
   {
      off = absol - absoluteOffset ;
   }
   else if (measurementType==hsoCGFROMBOTTOM || measurementType==hsoTOP2BOTTOM || 
            measurementType==hsoBOTTOM2BOTTOM)
   {
      off = absoluteOffset - absol;
   }
   else
      ATLASSERT(0); 

   return off;
}

Float64 CBridgeAgentImp::ComputeHarpedOffsetFromAbsoluteHp(SpanIndexType span,GirderIndexType gdr, const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset)
{
   Float64 absol = ComputeAbsoluteHarpedOffsetHp(span, gdr, rHarpedFillArray, measurementType, 0.0);

   Float64 off = 0.0;
   // direction depends if meassured from bottom up or top down
   if (measurementType==hsoCGFROMTOP || measurementType==hsoTOP2TOP || measurementType==hsoECCENTRICITY)
   {
      off = absol - absoluteOffset ;
   }
   else if (measurementType==hsoLEGACY || measurementType==hsoCGFROMBOTTOM || measurementType==hsoTOP2BOTTOM || 
            measurementType==hsoBOTTOM2BOTTOM )
   {
      off = absoluteOffset - absol;
   }
   else
      ATLASSERT(0);

   return off;
}

Float64 CBridgeAgentImp::ComputeHarpedOffsetFromAbsoluteHp(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset)
{
   Float64 absol = ComputeAbsoluteHarpedOffsetHp(strGirderName, rHarpedFillArray, measurementType, 0.0);

   Float64 off = 0.0;
   // direction depends if meassured from bottom up or top down
   if (measurementType==hsoCGFROMTOP || measurementType==hsoTOP2TOP || measurementType==hsoECCENTRICITY)
   {
      off = absol - absoluteOffset ;
   }
   else if (measurementType==hsoLEGACY || measurementType==hsoCGFROMBOTTOM || measurementType==hsoTOP2BOTTOM || 
            measurementType==hsoBOTTOM2BOTTOM )
   {
      off = absoluteOffset - absol;
   }
   else
      ATLASSERT(0);

   return off;
}

void CBridgeAgentImp::ComputeValidHarpedOffsetForMeasurementTypeEnd(SpanIndexType span,GirderIndexType gdr, const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange)
{
   CComPtr<IPrecastGirder> girder;
   GetGirder(span, gdr, &girder);

   CIndexArrayWrapper fill(rHarpedFillArray);

   Float64 absDown, absUp;
   HRESULT hr = girder->GetHarpedEndAdjustmentBoundsEx(&fill, &absDown, &absUp);
   ATLASSERT(SUCCEEDED(hr));

   Float64 offDown = ComputeHarpedOffsetFromAbsoluteEnd(span, gdr,  rHarpedFillArray, measurementType, absDown);
   Float64 offUp =   ComputeHarpedOffsetFromAbsoluteEnd(span, gdr,  rHarpedFillArray, measurementType, absUp);

   *lowRange = offDown;
   *highRange = offUp;
}

void CBridgeAgentImp::ComputeValidHarpedOffsetForMeasurementTypeEnd(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray,HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange)
{
   Float64 absDown, absUp;
   GetHarpedEndOffsetBoundsEx(strGirderName,rHarpedFillArray, &absDown,&absUp);

   Float64 offDown = ComputeHarpedOffsetFromAbsoluteEnd(strGirderName,  rHarpedFillArray, measurementType, absDown);
   Float64 offUp =   ComputeHarpedOffsetFromAbsoluteEnd(strGirderName,  rHarpedFillArray, measurementType, absUp);

   *lowRange = offDown;
   *highRange = offUp;
}

void CBridgeAgentImp::ComputeValidHarpedOffsetForMeasurementTypeHp(SpanIndexType span,GirderIndexType gdr,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange)
{
   CComPtr<IPrecastGirder> girder;
   GetGirder(span, gdr, &girder);

   CIndexArrayWrapper fill(rHarpedFillArray);

   Float64 absDown, absUp;
   HRESULT hr = girder->GetHarpedHpAdjustmentBoundsEx(&fill, &absDown, &absUp);
   ATLASSERT(SUCCEEDED(hr));

   Float64 offDown = ComputeHarpedOffsetFromAbsoluteHp(span, gdr,  rHarpedFillArray, measurementType, absDown);
   Float64 offUp   = ComputeHarpedOffsetFromAbsoluteHp(span, gdr,  rHarpedFillArray, measurementType, absUp);

   *lowRange = offDown;
   *highRange = offUp;
}

void CBridgeAgentImp::ComputeValidHarpedOffsetForMeasurementTypeHp(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange)
{
   Float64 absDown, absUp;
   GetHarpedHpOffsetBoundsEx(strGirderName,rHarpedFillArray, &absDown,&absUp);

   Float64 offDown = ComputeHarpedOffsetFromAbsoluteHp(strGirderName,  rHarpedFillArray, measurementType, absDown);
   Float64 offUp =   ComputeHarpedOffsetFromAbsoluteHp(strGirderName,  rHarpedFillArray, measurementType, absUp);

   *lowRange = offDown;
   *highRange = offUp;
}

Float64 CBridgeAgentImp::ConvertHarpedOffsetEnd(SpanIndexType span,GirderIndexType gdr,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType)
{
   Float64 abs_offset = ComputeAbsoluteHarpedOffsetEnd(span,gdr,rHarpedFillArray,fromMeasurementType,offset);
   Float64 result = ComputeHarpedOffsetFromAbsoluteEnd(span,gdr,rHarpedFillArray,toMeasurementType,abs_offset);
   return result;
}

Float64 CBridgeAgentImp::ConvertHarpedOffsetEnd(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType)
{
   Float64 abs_offset = ComputeAbsoluteHarpedOffsetEnd(strGirderName,rHarpedFillArray,fromMeasurementType,offset);
   Float64 result = ComputeHarpedOffsetFromAbsoluteEnd(strGirderName,rHarpedFillArray,toMeasurementType,abs_offset);
   return result;
}

Float64 CBridgeAgentImp::ConvertHarpedOffsetHp(SpanIndexType span,GirderIndexType gdr,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType)
{
   Float64 abs_offset = ComputeAbsoluteHarpedOffsetHp(span,gdr,rHarpedFillArray,fromMeasurementType,offset);
   Float64 result = ComputeHarpedOffsetFromAbsoluteHp(span,gdr,rHarpedFillArray,toMeasurementType,abs_offset);
   return result;
}

Float64 CBridgeAgentImp::ConvertHarpedOffsetHp(LPCTSTR strGirderName,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType)
{
   Float64 abs_offset = ComputeAbsoluteHarpedOffsetHp(strGirderName,rHarpedFillArray,fromMeasurementType,offset);
   Float64 result = ComputeHarpedOffsetFromAbsoluteHp(strGirderName,rHarpedFillArray,toMeasurementType,abs_offset);
   return result;
}

/////////////////////////////////////////////////////////////////////////
// IPointOfInterest
//
std::vector<pgsPointOfInterest> CBridgeAgentImp::GetPointsOfInterest(SpanIndexType span,GirderIndexType gdr)
{
   ValidatePointsOfInterest(span,gdr);

   // set up the span indices (ALL_SPANS doesn't work in loops)
   SpanIndexType startSpan = span;
   SpanIndexType nSpans = startSpan + 1;
   if ( span == ALL_SPANS )
   {
      startSpan = 0;
      nSpans = GetSpanCount_Private();
   }

   GET_IFACE(ILiveLoads,pLiveLoads);
   GET_IFACE(ILimitStateForces,pLimitStateForces);
   bool bPermit = pLimitStateForces->IsStrengthIIApplicable(span, gdr);

   // collect the desired POI into a vector
   std::vector<pgsPointOfInterest> poi;
   for ( SpanIndexType spanIdx = startSpan; spanIdx < nSpans; spanIdx++ )
   {
      // deal with girder index when there are different number of girders in each span
      GirderIndexType gdrIdx = gdr;
      GirderIndexType nGirders = GetGirderCount(spanIdx);
      if ( nGirders <= gdr )
         gdrIdx = nGirders-1;

      // Include critical sections
      pgsPointOfInterest poi_left,poi_right;
      GetCriticalSection(pgsTypes::StrengthI,spanIdx,gdrIdx,&poi_left,&poi_right);

      if ( bPermit )
         GetCriticalSection(pgsTypes::StrengthII,spanIdx,gdrIdx,&poi_left,&poi_right);

      std::vector<pgsPointOfInterest> vPoi = m_PoiMgr.GetPointsOfInterest( spanIdx, gdrIdx );
      poi.insert(poi.end(),vPoi.begin(),vPoi.end());
   }

   return poi;
}

struct PoiBefore
{
   static bool Compare(const pgsPointOfInterest& poi) 
   { 
      if ( m_SpanIdx != poi.GetSpan() || m_GirderIdx != poi.GetGirder() )
         return false;

      return poi.GetDistFromStart() < PoiBefore::m_Before && !IsEqual(PoiBefore::m_Before,poi.GetDistFromStart()); 
   }
   static SpanIndexType m_SpanIdx;
   static GirderIndexType m_GirderIdx;
   static Float64 m_Before;
};
Float64 PoiBefore::m_Before = 0;
SpanIndexType PoiBefore::m_SpanIdx = INVALID_INDEX;
GirderIndexType PoiBefore::m_GirderIdx = INVALID_INDEX;

struct PoiAfter
{
   static bool Compare(const pgsPointOfInterest& poi)
   { 
      if ( m_SpanIdx != poi.GetSpan() || m_GirderIdx != poi.GetGirder() )
         return false;

      return PoiAfter::m_After < poi.GetDistFromStart() && !IsEqual(PoiAfter::m_After,poi.GetDistFromStart()); 
   }
   static SpanIndexType m_SpanIdx;
   static GirderIndexType m_GirderIdx;
   static Float64 m_After;
};
Float64 PoiAfter::m_After = 0;
SpanIndexType PoiAfter::m_SpanIdx = INVALID_INDEX;
GirderIndexType PoiAfter::m_GirderIdx = INVALID_INDEX;

std::vector<pgsPointOfInterest> CBridgeAgentImp::GetPointsOfInterest(SpanIndexType span,GirderIndexType gdr,pgsTypes::Stage stage,PoiAttributeType attrib,Uint32 mode)
{
   // make sure we have POI before doing anything else
   ValidatePointsOfInterest(span,gdr);

   // set up the span indices (ALL_SPANS doesn't work in loops)
   SpanIndexType startSpan = span;
   SpanIndexType nSpans = startSpan + 1;
   if ( span == ALL_SPANS )
   {
      startSpan = 0;
      nSpans = GetSpanCount_Private();
   }

   Uint32 mgrMode = (mode == POIFIND_AND ? POIMGR_AND : POIMGR_OR);

   GET_IFACE(ILimitStateForces,pLimitStateForces);
   bool bPermit = pLimitStateForces->IsStrengthIIApplicable(span, gdr);

   // collect the desired POI into a vector
   std::vector<pgsPointOfInterest> poi;
   for ( SpanIndexType spanIdx = startSpan; spanIdx < nSpans; spanIdx++ )
   {
      // deal with girder index when there are different number of girders in each span
      GirderIndexType gdrIdx = gdr;
      GirderIndexType nGirders = GetGirderCount(spanIdx);
      if ( nGirders <= gdr )
         gdrIdx = nGirders-1;

      // if the request includes Critical Section, make sure the critical section POI's have been located
      if ( 0 <= StageCompare(stage,pgsTypes::BridgeSite1) && (attrib & POI_CRITSECTSHEAR1 || attrib & POI_CRITSECTSHEAR2) )
      {
         pgsPointOfInterest poi_left,poi_right;
         GetCriticalSection(pgsTypes::StrengthI,spanIdx,gdrIdx,&poi_left,&poi_right);

         if ( bPermit )
            GetCriticalSection(pgsTypes::StrengthII,spanIdx,gdrIdx,&poi_left,&poi_right);
      }

      std::vector<pgsPointOfInterest> vPoi;
      m_PoiMgr.GetPointsOfInterest( spanIdx, gdrIdx, stage, attrib, mgrMode, &vPoi );
      poi.insert(poi.end(),vPoi.begin(),vPoi.end());

      if ( StageCompare(pgsTypes::GirderPlacement,stage) <= 0 )
      {
         // remove all poi that are before the first bearing or after the last bearing
         Float64 start_dist = GetGirderStartConnectionLength(spanIdx,gdrIdx);
         Float64 end_dist   = GetGirderLength(spanIdx,gdrIdx) - GetGirderEndConnectionLength(spanIdx,gdrIdx);
         PoiBefore::m_SpanIdx = spanIdx;
         PoiBefore::m_GirderIdx = gdrIdx;
         PoiBefore::m_Before = start_dist;

         PoiAfter::m_SpanIdx = spanIdx;
         PoiAfter::m_GirderIdx = gdrIdx;
         PoiAfter::m_After   = end_dist;
         std::vector<pgsPointOfInterest>::iterator found = std::find_if(poi.begin(),poi.end(),&PoiBefore::Compare);
         while ( found != poi.end() )
         {
            poi.erase(found);
            found = std::find_if(poi.begin(),poi.end(),&PoiBefore::Compare);
         }

         found = std::find_if(poi.begin(),poi.end(),&PoiAfter::Compare);
         while ( found != poi.end() )
         {
            poi.erase(found);
            found = std::find_if(poi.begin(),poi.end(),&PoiAfter::Compare);
         }
      }
   }

   return poi;
}

std::vector<pgsPointOfInterest> CBridgeAgentImp::GetPointsOfInterest(SpanIndexType span,GirderIndexType gdr,const std::vector<pgsTypes::Stage>& stages,PoiAttributeType attrib,Uint32 mode)
{
   // make sure we have POI before doing anything else
   ValidatePointsOfInterest(span,gdr);

   // set up the span indices (ALL_SPANS doesn't work in loops)
   SpanIndexType startSpan = span;
   SpanIndexType nSpans = startSpan + 1;
   if ( span == ALL_SPANS )
   {
      startSpan = 0;
      nSpans = GetSpanCount_Private();
   }

   Uint32 mgrMode = (mode == POIFIND_AND ? POIMGR_AND : POIMGR_OR);

   GET_IFACE(ILimitStateForces,pLimitStateForces);
   bool bPermit = pLimitStateForces->IsStrengthIIApplicable(span, gdr);

   // collect the desired POI into a vector
   std::vector<pgsPointOfInterest> poi;
   for ( SpanIndexType spanIdx = startSpan; spanIdx < nSpans; spanIdx++ )
   {
      // deal with girder index when there are different number of girders in each span
      GirderIndexType gdrIdx = gdr;
      GirderIndexType nGirders = GetGirderCount(spanIdx);
      if ( nGirders <= gdr )
         gdrIdx = nGirders-1;

      // if the request includes Critical Section, make sure the critical section POI's have been located
      std::vector<pgsTypes::Stage> sortedStages(stages);
      std::sort(sortedStages.begin(),sortedStages.end());
      std::vector<pgsTypes::Stage>::iterator stageBegin(sortedStages.begin());
      std::vector<pgsTypes::Stage>::iterator stageEnd(sortedStages.end());
      
      bool bCriticalSection = false;
      if ( std::find(stageBegin,stageEnd,pgsTypes::BridgeSite1) != stageEnd ||
           std::find(stageBegin,stageEnd,pgsTypes::BridgeSite2) != stageEnd ||
           std::find(stageBegin,stageEnd,pgsTypes::BridgeSite3) != stageEnd )
      {
         bCriticalSection = true;
      }

      if ( bCriticalSection && (attrib & POI_CRITSECTSHEAR1 || attrib & POI_CRITSECTSHEAR2) )
      {
         pgsPointOfInterest poi_left,poi_right;
         GetCriticalSection(pgsTypes::StrengthI,spanIdx,gdrIdx,&poi_left,&poi_right);

         if ( bPermit )
            GetCriticalSection(pgsTypes::StrengthII,spanIdx,gdrIdx,&poi_left,&poi_right);
      }

      std::vector<pgsPointOfInterest> vPoi;
      m_PoiMgr.GetPointsOfInterest( spanIdx, gdrIdx, stages, attrib, mgrMode, &vPoi );
      poi.insert(poi.end(),vPoi.begin(),vPoi.end());
   }

   return poi;
}

std::vector<pgsPointOfInterest> CBridgeAgentImp::GetTenthPointPOIs(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr)
{
   ValidatePointsOfInterest(span,gdr);
   std::vector<pgsPointOfInterest> poi;
   m_PoiMgr.GetTenthPointPOIs( stage, span, gdr, &poi );
   return poi;
}

void CBridgeAgentImp::GetCriticalSection(pgsTypes::LimitState limitState,SpanIndexType span,GirderIndexType gdr,pgsPointOfInterest* pLeft,pgsPointOfInterest* pRight)
{
   std::vector<pgsTypes::Stage> stages;
   stages.push_back(pgsTypes::CastingYard); // for neg moment capacity at CS (need camber to build capacity model... camber starts in casting yard)
   stages.push_back(pgsTypes::BridgeSite3); // for shear

   int idx = limitState == pgsTypes::StrengthI ? 0 : 1;
   std::set<SpanGirderHashType>::iterator found;
   found = m_CriticalSectionState[idx].find( HashSpanGirder(span,gdr) );
   if ( found == m_CriticalSectionState[idx].end() )
   {
      // Critical section not previously computed.
      // Do it now.
      GET_IFACE(IShearCapacity,pShearCap);
      Float64 xl, xr; // distance from end of girder for left and right critical section
      pShearCap->GetCriticalSection(limitState,span,gdr,&xl,&xr);

      PoiAttributeType attrib = POI_SHEAR | POI_GRAPHICAL | POI_TABULAR;
      attrib |= (limitState == pgsTypes::StrengthI ? POI_CRITSECTSHEAR1 : POI_CRITSECTSHEAR2);
      m_PoiMgr.AddPointOfInterest( pgsPointOfInterest(stages,span,gdr,xl, attrib) );
      m_PoiMgr.AddPointOfInterest( pgsPointOfInterest(stages,span,gdr,xr, attrib) );

      m_CriticalSectionState[idx].insert( HashSpanGirder(span,gdr) );
   }

   std::vector<pgsPointOfInterest> vPoi;
   m_PoiMgr.GetPointsOfInterest(span,gdr,pgsTypes::BridgeSite3,(limitState == pgsTypes::StrengthI ? POI_CRITSECTSHEAR1 : POI_CRITSECTSHEAR2),POIMGR_AND,&vPoi);
   CHECK( vPoi.size() == 2 );
   std::vector<pgsPointOfInterest>::iterator iter = vPoi.begin();
   *pLeft  = *iter++;
   *pRight = *iter++;
}

void CBridgeAgentImp::GetCriticalSection(pgsTypes::LimitState limitState,SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config,pgsPointOfInterest* pLeft,pgsPointOfInterest* pRight)
{
   std::vector<pgsTypes::Stage> stages;
   stages.push_back(pgsTypes::CastingYard); // for neg moment capacity at CS (need camber to build capacity model... camber starts in casting yard)
   stages.push_back(pgsTypes::BridgeSite3); // for shear

   GET_IFACE(IShearCapacity,pShearCap);
   Float64 xl, xr; // distance from end of girder for left and right critical section
   pShearCap->GetCriticalSection(limitState,span,gdr,config,&xl,&xr);

   PoiAttributeType attrib = POI_SHEAR | POI_GRAPHICAL | POI_TABULAR;
   attrib |= (limitState == pgsTypes::StrengthI ? POI_CRITSECTSHEAR1 : POI_CRITSECTSHEAR2);
   pgsPointOfInterest csLeft( stages,span,gdr,xl,attrib);
   pgsPointOfInterest csRight(stages,span,gdr,xr,attrib);

   *pLeft  = csLeft;
   *pRight = csRight;
}

Float64 CBridgeAgentImp::GetDistanceFromFirstPier(const pgsPointOfInterest& poi,pgsTypes::Stage stage)
{
   SpanIndexType last_span = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   Float64 x = 0; // distance from first pier
   for ( SpanIndexType spanIdx = 0; spanIdx < last_span; spanIdx++ )
   {
      GirderIndexType nGirders = GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = (nGirders <= gdr ? nGirders-1 : gdr);

      Float64 L;
      if ( stage == pgsTypes::CastingYard )
         L = GetGirderLength(spanIdx,gdrIdx);
      else
         L = GetSpanLength(spanIdx,gdrIdx);

      x += L;
   }

   Float64 end_offset = 0;
   if ( stage != pgsTypes::CastingYard )
      end_offset = GetGirderStartConnectionLength(last_span,gdr);

   x += poi.GetDistFromStart() - end_offset;

   return x;
}

pgsPointOfInterest CBridgeAgentImp::GetPointOfInterest(pgsTypes::Stage stage,SpanIndexType spanIdx,GirderIndexType gdrIdx,Float64 distFromStart)
{
   ValidatePointsOfInterest(spanIdx,gdrIdx);
   return m_PoiMgr.GetPointOfInterest(stage,spanIdx,gdrIdx,distFromStart);
}

pgsPointOfInterest CBridgeAgentImp::GetPointOfInterest(SpanIndexType spanIdx,GirderIndexType gdrIdx,Float64 distFromStart)
{
   ValidatePointsOfInterest(spanIdx,gdrIdx);
   return m_PoiMgr.GetPointOfInterest(spanIdx,gdrIdx,distFromStart);
}

pgsPointOfInterest CBridgeAgentImp::GetNearestPointOfInterest(SpanIndexType spanIdx,GirderIndexType gdrIdx,Float64 distFromStart)
{
   ValidatePointsOfInterest(spanIdx,gdrIdx);
   return m_PoiMgr.GetNearestPointOfInterest(spanIdx,gdrIdx,distFromStart);
}

pgsPointOfInterest CBridgeAgentImp::GetNearestPointOfInterest(pgsTypes::Stage stage,SpanIndexType spanIdx,GirderIndexType gdrIdx,Float64 distFromStart)
{
   ValidatePointsOfInterest(spanIdx,gdrIdx);
   return m_PoiMgr.GetNearestPointOfInterest(stage,spanIdx,gdrIdx,distFromStart);
}

/////////////////////////////////////////////////////////////////////////
// ISectProp2
//
Float64 CBridgeAgentImp::GetHg(pgsTypes::Stage stage,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(stage,poi);
   Float64 Yt, Yb;
   props.ShapeProps->get_Ytop(&Yt);
   props.ShapeProps->get_Ybottom(&Yb);

   return Yt+Yb;
}

Float64 CBridgeAgentImp::GetAg(pgsTypes::Stage stage,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(stage,poi);
   Float64 area;
   props.ShapeProps->get_Area(&area);
   return area;
}

Float64 CBridgeAgentImp::GetIx(pgsTypes::Stage stage,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(stage,poi);
   Float64 ixx;
   props.ShapeProps->get_Ixx(&ixx);
   return ixx;
}

Float64 CBridgeAgentImp::GetIy(pgsTypes::Stage stage,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(stage,poi);
   Float64 iyy;
   props.ShapeProps->get_Iyy(&iyy);
   return iyy;
}

Float64 CBridgeAgentImp::GetYt(pgsTypes::Stage stage,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(stage,poi);
   Float64 yt;
   props.ShapeProps->get_Ytop(&yt);
   return yt;
}

Float64 CBridgeAgentImp::GetYb(pgsTypes::Stage stage,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(stage,poi);
   Float64 yb;
   props.ShapeProps->get_Ybottom(&yb);
   return yb;
}

Float64 CBridgeAgentImp::GetSt(pgsTypes::Stage stage,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(stage,poi);

   Float64 ixx;
   props.ShapeProps->get_Ixx(&ixx);
   
   Float64 yt;
   props.ShapeProps->get_Ytop(&yt);

   Float64 St = -ixx/yt;

   if ( stage == pgsTypes::BridgeSite2 || stage == pgsTypes::BridgeSite3 && IsCompositeDeck() )
   {
      // if there is a composite deck and the deck is on, return St (Stop deck) in terms of deck material
      Float64 Eg = GetEcGdr(poi.GetSpan(),poi.GetGirder());
      Float64 Es = GetEcSlab();

      Float64 n = Eg/Es;
      St *= n;
   }

   return St;
}

Float64 CBridgeAgentImp::GetSb(pgsTypes::Stage stage,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(stage,poi);
   
   Float64 ixx;
   props.ShapeProps->get_Ixx(&ixx);
   
   Float64 yb;
   props.ShapeProps->get_Ybottom(&yb);

   Float64 Sb = ixx/yb;

   return Sb;
}

Float64 CBridgeAgentImp::GetYtGirder(pgsTypes::Stage stage,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(stage,poi);
   return props.YtopGirder;
}

Float64 CBridgeAgentImp::GetStGirder(pgsTypes::Stage stage,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(stage,poi);
   Float64 Yt = props.YtopGirder;
   Float64 Ix;
   props.ShapeProps->get_Ixx(&Ix);
   Float64 St = -Ix/Yt;
   return St;
}

Float64 CBridgeAgentImp::GetKt(const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(pgsTypes::CastingYard,poi);

   Float64 yb;
   props.ShapeProps->get_Ybottom(&yb);
   
   Float64 area;
   props.ShapeProps->get_Area(&area);

   Float64 Ix;
   props.ShapeProps->get_Ixx(&Ix);

   Float64 k = -Ix/(area*yb);
   return k;
}

Float64 CBridgeAgentImp::GetKb(const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(pgsTypes::CastingYard,poi);

   Float64 yt;
   props.ShapeProps->get_Ytop(&yt);
   
   Float64 area;
   props.ShapeProps->get_Area(&area);

   Float64 Ix;
   props.ShapeProps->get_Ixx(&Ix);

   Float64 k = Ix/(area*yt);
   return k;
}

Float64 CBridgeAgentImp::GetEIx(pgsTypes::Stage stage,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(stage,poi);

   Float64 EIx;
   props.ElasticProps->get_EIxx(&EIx);
   return EIx;
}

Float64 CBridgeAgentImp::GetAg(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr)
{
   GET_IFACE(IGirderData,pGirderData);
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   bool ec_changed;
   Float64 E;
   if (stage==pgsTypes::CastingYard)
      E = GetGirderEci(pGirderData, span, gdr, fcgdr, &ec_changed);
   else
      E = GetGirderEc(pGirderData, span, gdr, fcgdr, &ec_changed);

   // if the "trial" girder strength is the same as the real girder strength
   // don't do a bunch of extra work. Return the properties for the real girder
   if ( !ec_changed )
      return GetAg(stage,poi);

   CComPtr<IShapeProperties> sprops;
   GetShapeProperties(stage,poi,E,&sprops);

   Float64 Ag;
   sprops->get_Area(&Ag);
   return Ag;
}

Float64 CBridgeAgentImp::GetIx(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr)
{
   GET_IFACE(IGirderData,pGirderData);
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   bool ec_changed;
   Float64 E;
   if (stage==pgsTypes::CastingYard)
      E = GetGirderEci(pGirderData, span, gdr, fcgdr, &ec_changed);
   else
      E = GetGirderEc(pGirderData, span, gdr, fcgdr, &ec_changed);

   // if the "trial" girder strength is the same as the real girder strength
   // don't do a bunch of extra work. Return the properties for the real girder
   if ( !ec_changed )
      return GetIx(stage,poi);

   CComPtr<IShapeProperties> sprops;
   GetShapeProperties(stage,poi,E,&sprops);

   Float64 Ix;
   sprops->get_Ixx(&Ix);
   return Ix;
}

Float64 CBridgeAgentImp::GetIy(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr)
{
   GET_IFACE(IGirderData,pGirderData);
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   bool ec_changed;
   Float64 E;
   if (stage==pgsTypes::CastingYard)
      E = GetGirderEci(pGirderData, span, gdr, fcgdr, &ec_changed);
   else
      E = GetGirderEc(pGirderData, span, gdr, fcgdr, &ec_changed);

   // if the "trial" girder strength is the same as the real girder strength
   // don't do a bunch of extra work. Return the properties for the real girder
   if ( !ec_changed )
      return GetIy(stage,poi);

   CComPtr<IShapeProperties> sprops;
   GetShapeProperties(stage,poi,E,&sprops);

   Float64 Iy;
   sprops->get_Iyy(&Iy);
   return Iy;
}

Float64 CBridgeAgentImp::GetYt(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr)
{
   GET_IFACE(IGirderData,pGirderData);
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   bool ec_changed;
   Float64 E;
   if (stage==pgsTypes::CastingYard)
      E = GetGirderEci(pGirderData, span, gdr, fcgdr, &ec_changed);
   else
      E = GetGirderEc(pGirderData, span, gdr, fcgdr, &ec_changed);

   // if the "trial" girder strength is the same as the real girder strength
   // don't do a bunch of extra work. Return the properties for the real girder
   if ( !ec_changed )
      return GetYt(stage,poi);

   CComPtr<IShapeProperties> sprops;
   GetShapeProperties(stage,poi,E,&sprops);

   Float64 yt;
   sprops->get_Ytop(&yt);
   return yt;
}

Float64 CBridgeAgentImp::GetYb(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr)
{
   GET_IFACE(IGirderData,pGirderData);
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   bool ec_changed;
   Float64 E;
   if (stage==pgsTypes::CastingYard)
      E = GetGirderEci(pGirderData, span, gdr, fcgdr, &ec_changed);
   else
      E = GetGirderEc(pGirderData, span, gdr, fcgdr, &ec_changed);

   // if the "trial" girder strength is the same as the real girder strength
   // don't do a bunch of extra work. Return the properties for the real girder
   if ( !ec_changed )
      return GetYb(stage,poi);

   CComPtr<IShapeProperties> sprops;
   GetShapeProperties(stage,poi,E,&sprops);

   Float64 yb;
   sprops->get_Ybottom(&yb);
   return yb;
}

Float64 CBridgeAgentImp::GetSt(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr)
{
   GET_IFACE(IGirderData,pGirderData);
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   bool ec_changed;
   Float64 E;
   if (stage==pgsTypes::CastingYard)
      E = GetGirderEci(pGirderData, span, gdr, fcgdr, &ec_changed);
   else
      E = GetGirderEc(pGirderData, span, gdr, fcgdr, &ec_changed);

   // if the "trial" girder strength is the same as the real girder strength
   // don't do a bunch of extra work. Return the properties for the real girder
   if ( !ec_changed )
      return GetSt(stage,poi);

   CComPtr<IShapeProperties> sprops;
   GetShapeProperties(stage,poi,E,&sprops);
   Float64 ixx;
   sprops->get_Ixx(&ixx);
   
   Float64 yt;
   sprops->get_Ytop(&yt);

   Float64 St = -ixx/yt;

   if ( stage == pgsTypes::BridgeSite2 || stage == pgsTypes::BridgeSite3 && IsCompositeDeck() )
   {
      // if there is a composite deck and the deck is on, return St (Stop deck) in terms of deck material
      Float64 Eg = E;
      Float64 Es = GetEcSlab();

      Float64 n = Eg/Es;
      St *= n;
   }

   return St;
}

Float64 CBridgeAgentImp::GetSb(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr)
{
   GET_IFACE(IGirderData,pGirderData);
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   bool ec_changed;
   Float64 E;
   if (stage==pgsTypes::CastingYard)
      E = GetGirderEci(pGirderData, span, gdr, fcgdr, &ec_changed);
   else
      E = GetGirderEc(pGirderData, span, gdr, fcgdr, &ec_changed);

   // if the "trial" girder strength is the same as the real girder strength
   // don't do a bunch of extra work. Return the properties for the real girder
   if ( !ec_changed )
      return GetSb(stage,poi);

   CComPtr<IShapeProperties> sprops;
   GetShapeProperties(stage,poi,E,&sprops);

   Float64 ixx;
   sprops->get_Ixx(&ixx);
   
   Float64 yb;
   sprops->get_Ybottom(&yb);

   Float64 Sb = ixx/yb;

   return Sb;
}

Float64 CBridgeAgentImp::GetYtGirder(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr)
{
   GET_IFACE(IGirderData,pGirderData);
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   bool ec_changed;
   Float64 E;
   if (stage==pgsTypes::CastingYard)
      E = GetGirderEci(pGirderData, span, gdr, fcgdr, &ec_changed);
   else
      E = GetGirderEc(pGirderData, span, gdr, fcgdr, &ec_changed);

   // if the "trial" girder strength is the same as the real girder strength
   // don't do a bunch of extra work. Return the properties for the real girder
   if ( !ec_changed )
      return GetYtGirder(stage,poi);

   CComPtr<IShapeProperties> sprops;
   GetShapeProperties(stage,poi,E,&sprops);

   Float64 YtopGirder;
   if ( StageCompare(stage,pgsTypes::BridgeSite2) < 0 )
   {
      sprops->get_Ytop(&YtopGirder);
   }
   else
   {
      Float64 Hg = GetHg(pgsTypes::BridgeSite1,poi);
      Float64 Yb = GetYb(stage,poi,fcgdr);
      YtopGirder = Hg - Yb;
   }

   return YtopGirder;
}

Float64 CBridgeAgentImp::GetStGirder(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 fcgdr)
{
   Float64 Ixx = GetIx(stage,poi,fcgdr);
   Float64 Yt  = GetYtGirder(stage,poi,fcgdr);
   return -Ixx/Yt;
}

Float64 CBridgeAgentImp::GetQSlab(const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(pgsTypes::BridgeSite2,poi);
   return props.Qslab;
}

Float64 CBridgeAgentImp::GetAcBottomHalf(const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(pgsTypes::BridgeSite2,poi);
   return props.AcBottomHalf;
}

Float64 CBridgeAgentImp::GetAcTopHalf(const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(pgsTypes::BridgeSite2,poi);
   return props.AcTopHalf;
}

Float64 CBridgeAgentImp::GetTributaryFlangeWidth(const pgsPointOfInterest& poi)
{
   Float64 tfw = 0;

   if ( IsCompositeDeck() )
   {
      SpanIndexType spanIdx = poi.GetSpan();
      GirderIndexType gdrIdx = poi.GetGirder();
      Float64 dist_from_start_of_girder = poi.GetDistFromStart();
      HRESULT hr = m_EffFlangeWidthTool->TributaryFlangeWidth(m_Bridge,spanIdx,gdrIdx,dist_from_start_of_girder,&tfw);
      ATLASSERT(SUCCEEDED(hr));
   }

   return tfw;
}

Float64 CBridgeAgentImp::GetTributaryFlangeWidthEx(const pgsPointOfInterest& poi, Float64* pLftFw, Float64* pRgtFw)
{
   Float64 tfw = 0;
   *pLftFw = 0;
   *pRgtFw = 0;

   if ( IsCompositeDeck() )
   {
      SpanIndexType spanIdx = poi.GetSpan();
      GirderIndexType gdrIdx = poi.GetGirder();
      Float64 dist_from_start_of_girder = poi.GetDistFromStart();
      HRESULT hr = m_EffFlangeWidthTool->TributaryFlangeWidthEx(m_Bridge,spanIdx,gdrIdx,dist_from_start_of_girder,pLftFw,pRgtFw,&tfw);
      ATLASSERT(SUCCEEDED(hr));
   }

   return tfw;
}

Float64 CBridgeAgentImp::GetEffectiveFlangeWidth(const pgsPointOfInterest& poi)
{
   Float64 efw = 0;

   GET_IFACE(ILibrary,       pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmLRFD )
   {
      CComQIPtr<IPGSuperEffectiveFlangeWidthTool> eff_tool(m_EffFlangeWidthTool);
      ATLASSERT(eff_tool);
      eff_tool->put_UseTributaryWidth(VARIANT_FALSE);

      if ( IsCompositeDeck() )
      {
         SpanIndexType spanIdx = poi.GetSpan();
         GirderIndexType gdrIdx = poi.GetGirder();
         Float64 dist_from_start_of_girder = poi.GetDistFromStart();
         HRESULT hr = m_EffFlangeWidthTool->EffectiveFlangeWidth(m_Bridge,spanIdx,gdrIdx,dist_from_start_of_girder,&efw);
         ATLASSERT(SUCCEEDED(hr));
      }
   }
   else
   {
      CComQIPtr<IPGSuperEffectiveFlangeWidthTool> eff_tool(m_EffFlangeWidthTool);
      ATLASSERT(eff_tool);
      eff_tool->put_UseTributaryWidth(VARIANT_TRUE);

      efw = GetTributaryFlangeWidth(poi);
   }

   return efw;
}

Float64 CBridgeAgentImp::GetEffectiveDeckArea(const pgsPointOfInterest& poi)
{
    Float64 efw   = GetEffectiveFlangeWidth(poi);
    Float64 tSlab = GetStructuralSlabDepth(poi);

    return efw*tSlab;
}

Float64 CBridgeAgentImp::GetTributaryDeckArea(const pgsPointOfInterest& poi)
{
    Float64 tfw   = GetTributaryFlangeWidth(poi);
    Float64 tSlab = GetStructuralSlabDepth(poi);

    return tfw*tSlab;
}

Float64 CBridgeAgentImp::GetGrossDeckArea(const pgsPointOfInterest& poi)
{
    Float64 tfw   = GetTributaryFlangeWidth(poi);
    Float64 tSlab = GetGrossSlabDepth(poi);

    return tfw*tSlab;
}

Float64 CBridgeAgentImp::GetDistTopSlabToTopGirder(const pgsPointOfInterest& poi)
{
   // camber effects are ignored
   VALIDATE( BRIDGE );

   // top of girder reference chord elevation
   Float64 yc = GetTopGirderReferenceChordElevation(poi);

   // get station and offset for poi
   Float64 station,offset;
   GetStationAndOffset(poi,&station,&offset);
   offset = IsZero(offset) ? 0 : offset;

   // top of alignment elevation above girder
   Float64 ya = GetElevation(station,offset);

   Float64 slab_offset = GetSlabOffset(poi);

   return ya - yc + slab_offset;
}

void CBridgeAgentImp::ReportEffectiveFlangeWidth(SpanIndexType span,GirderIndexType girder,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE(ILibrary,       pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   CComQIPtr<IPGSuperEffectiveFlangeWidthTool> eff_tool(m_EffFlangeWidthTool);
   ATLASSERT(eff_tool);
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmLRFD )
   {
      eff_tool->put_UseTributaryWidth(VARIANT_FALSE);
   }
   else
   {
      eff_tool->put_UseTributaryWidth(VARIANT_TRUE);
   }

   CComQIPtr<IReportEffectiveFlangeWidth> report(m_EffFlangeWidthTool);
   report->ReportEffectiveFlangeWidth(m_pBroker,m_Bridge,span,girder,pChapter,pDisplayUnits);
}

Float64 CBridgeAgentImp::GetPerimeter(const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(pgsTypes::CastingYard,poi);
   return props.Perimeter;
}

Float64 CBridgeAgentImp::GetSurfaceArea(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const GirderLibraryEntry* pGdrEntry = pBridgeDesc->GetSpan(spanIdx)->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);

   CComPtr<IBeamFactory> factory;
   pGdrEntry->GetBeamFactory(&factory);

   return factory->GetSurfaceArea(m_pBroker,spanIdx,gdrIdx,true);
}

Float64 CBridgeAgentImp::GetVolume(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const GirderLibraryEntry* pGdrEntry = pBridgeDesc->GetSpan(spanIdx)->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);

   CComPtr<IBeamFactory> factory;
   pGdrEntry->GetBeamFactory(&factory);

   return factory->GetVolume(m_pBroker,spanIdx,gdrIdx);
}

Float64 CBridgeAgentImp::GetBridgeEIxx(Float64 distFromStart)
{
   CComPtr<ISection> section;
   m_SectCutTool->CreateBridgeSection(m_Bridge,distFromStart,GetStageName(pgsTypes::BridgeSite3),bscStructurallyContinuousOnly,&section);

   CComPtr<IElasticProperties> eprops;
   section->get_ElasticProperties(&eprops);

   Float64 EIxx;
   eprops->get_EIxx(&EIxx);

   return EIxx;
}

Float64 CBridgeAgentImp::GetBridgeEIyy(Float64 distFromStart)
{
   CComPtr<ISection> section;
   m_SectCutTool->CreateBridgeSection(m_Bridge,distFromStart,GetStageName(pgsTypes::BridgeSite3),bscStructurallyContinuousOnly,&section);

   CComPtr<IElasticProperties> eprops;
   section->get_ElasticProperties(&eprops);

   Float64 EIyy;
   eprops->get_EIyy(&EIyy);

   return EIyy;
}

void CBridgeAgentImp::GetGirderShape(const pgsPointOfInterest& poi,bool bOrient,IShape** ppShape)
{
   SectProp& props = GetSectionProperties(pgsTypes::CastingYard,poi);

#pragma Reminder("UPDATE: Assuming section is a Composite section and beam is exactly the first piece")
   CComQIPtr<ICompositeSection> cmpsection(props.Section);
   CComPtr<ICompositeSectionItem> csi;
   cmpsection->get_Item(0,&csi); // this should be the beam
   CComPtr<IShape> shape;
   csi->get_Shape(&shape);

   shape->Clone(ppShape);

   if ( bOrient )
   {
      Float64 station,offset;
      GetStationAndOffset(poi,&station,&offset);

      CComPtr<IProfile> profile;
      GetProfile(&profile);

      Float64 cpo;
      profile->CrownPointOffset(CComVariant(station),&cpo);

      offset -= cpo; // if < 0, girder is left of crown, else right of crown

      Float64 orientation = GetOrientation(poi.GetSpan(),poi.GetGirder());

      Float64 rotation_angle = -orientation;

      CComQIPtr<IXYPosition> position(*ppShape);
      CComPtr<IPoint2d> top_center;
      position->get_LocatorPoint(lpTopCenter,&top_center);
      position->RotateEx(top_center,rotation_angle);
   }
}

void CBridgeAgentImp::GetSlabShape(Float64 station,IShape** ppShape)
{
   ShapeContainer::iterator found = m_DeckShapes.find(station);

   if ( found != m_DeckShapes.end() )
   {
      (*ppShape) = found->second;
      (*ppShape)->AddRef();
   }
   else
   {
      HRESULT hr = m_SectCutTool->CreateSlabShape(m_Bridge,station,ppShape);
      ATLASSERT(SUCCEEDED(hr));

      if ( *ppShape )
         m_DeckShapes.insert(std::make_pair(station,CComPtr<IShape>(*ppShape)));
   }
}

void CBridgeAgentImp::GetLeftTrafficBarrierShape(Float64 station,IShape** ppShape)
{
   ShapeContainer::iterator found = m_LeftBarrierShapes.find(station);
   if ( found != m_LeftBarrierShapes.end() )
   {
      found->second->Clone(ppShape);
   }
   else
   {
      CComPtr<IShape> shape;
      HRESULT hr = m_SectCutTool->CreateLeftBarrierShape(m_Bridge,station,&shape);

      if ( SUCCEEDED(hr) )
      {
         m_LeftBarrierShapes.insert(std::make_pair(station,CComPtr<IShape>(shape)));
         shape->Clone(ppShape);
      }
   }
}

void CBridgeAgentImp::GetRightTrafficBarrierShape(Float64 station,IShape** ppShape)
{
   ShapeContainer::iterator found = m_RightBarrierShapes.find(station);
   if ( found != m_RightBarrierShapes.end() )
   {
      found->second->Clone(ppShape);
   }
   else
   {
      CComPtr<IShape> shape;
      HRESULT hr = m_SectCutTool->CreateRightBarrierShape(m_Bridge,station,&shape);

      if ( SUCCEEDED(hr) )
      {
         m_RightBarrierShapes.insert(std::make_pair(station,CComPtr<IShape>(shape)));
         shape->Clone(ppShape);
      }
   }
}

Float64 CBridgeAgentImp::GetGirderWeightPerLength(SpanIndexType span,GirderIndexType gdr)
{
   Float64 ag = GetAg(pgsTypes::CastingYard,pgsPointOfInterest(span,gdr,0.00));
   Float64 dens = GetWgtDensityGdr(span,gdr);
   Float64 weight_per_length = ag * dens * unitSysUnitsMgr::GetGravitationalAcceleration();
   return weight_per_length;
}

Float64 CBridgeAgentImp::GetGirderWeight(SpanIndexType span,GirderIndexType gdr)
{
   Float64 weight_per_length = GetGirderWeightPerLength(span,gdr);
   Float64 length = GetGirderLength(span,gdr);
   return weight_per_length * length;
}

/////////////////////////////////////////////////////////////////////////
// IBarriers
//
Float64 CBridgeAgentImp::GetAtb(pgsTypes::TrafficBarrierOrientation orientation)
{
   CComPtr<IShapeProperties> props;
   GetBarrierProperties(orientation,&props);

   if ( props == NULL )
      return 0;

   Float64 area;
   props->get_Area(&area);

   return area;
}

Float64 CBridgeAgentImp::GetItb(pgsTypes::TrafficBarrierOrientation orientation)
{
   CComPtr<IShapeProperties> props;
   GetBarrierProperties(orientation,&props);

   if ( props == NULL )
      return 0;

   Float64 Ix;
   props->get_Ixx(&Ix);

   return Ix;
}

Float64 CBridgeAgentImp::GetYbtb(pgsTypes::TrafficBarrierOrientation orientation)
{
   CComPtr<IShapeProperties> props;
   GetBarrierProperties(orientation,&props);

   if ( props == NULL )
      return 0;

   Float64 Yb;
   props->get_Ybottom(&Yb);

   return Yb;
}

Float64 CBridgeAgentImp::GetSidewalkWeight(pgsTypes::TrafficBarrierOrientation orientation)
{
   VALIDATE(BRIDGE);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CRailingSystem* pRailingSystem = NULL;
   if ( orientation == pgsTypes::tboLeft )
      pRailingSystem = pBridgeDesc->GetLeftRailingSystem();
   else
      pRailingSystem = pBridgeDesc->GetRightRailingSystem();

   Float64 Wsw = 0;
   if ( pRailingSystem->bUseSidewalk )
   {
      GET_IFACE(IBarriers,pBarriers);

      // real dl width of sidwalk
      Float64 intEdge, extEdge;
      pBarriers->GetSidewalkDeadLoadEdges(orientation, &intEdge, &extEdge);

      Float64 w = fabs(intEdge-extEdge);
      Float64 tl = pRailingSystem->LeftDepth;
      Float64 tr = pRailingSystem->RightDepth;
      Float64 area = w*(tl + tr)/2;
      Float64 density = GetDensityRailing(orientation);
      Float64 mpl = area * density; // mass per length
      Float64 g = unitSysUnitsMgr::GetGravitationalAcceleration();
      Wsw = mpl * g;
   }

   return Wsw;
}

bool CBridgeAgentImp::HasSidewalk(pgsTypes::TrafficBarrierOrientation orientation)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CRailingSystem* pRailingSystem = NULL;
   if ( orientation == pgsTypes::tboLeft )
      pRailingSystem = pBridgeDesc->GetLeftRailingSystem();
   else
      pRailingSystem = pBridgeDesc->GetRightRailingSystem();

   return pRailingSystem->bUseSidewalk;
}

Float64 CBridgeAgentImp::GetExteriorBarrierWeight(pgsTypes::TrafficBarrierOrientation orientation)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CRailingSystem* pRailingSystem = NULL;
   if ( orientation == pgsTypes::tboLeft )
      pRailingSystem = pBridgeDesc->GetLeftRailingSystem();
   else
      pRailingSystem = pBridgeDesc->GetRightRailingSystem();

   Float64 Wext = 0; // weight of exterior barrier
   if ( pRailingSystem->GetExteriorRailing()->GetWeightMethod() == TrafficBarrierEntry::Compute )
   {
      CComPtr<IPolyShape> polyshape;
      pRailingSystem->GetExteriorRailing()->CreatePolyShape(orientation,&polyshape);

      CComQIPtr<IShape> shape(polyshape);
      CComPtr<IShapeProperties> props;
      shape->get_ShapeProperties(&props);
      Float64 area;
      props->get_Area(&area);

      Float64 density = GetDensityRailing(orientation);
      Float64 mplBarrier = area * density;
      Float64 g = unitSysUnitsMgr::GetGravitationalAcceleration();
      Wext = mplBarrier * g;
   }
   else
   {
      Wext = pRailingSystem->GetExteriorRailing()->GetWeight();
   }

   return Wext;
}

Float64 CBridgeAgentImp::GetInteriorBarrierWeight(pgsTypes::TrafficBarrierOrientation orientation)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CRailingSystem* pRailingSystem = NULL;
   if ( orientation == pgsTypes::tboLeft )
      pRailingSystem = pBridgeDesc->GetLeftRailingSystem();
   else
      pRailingSystem = pBridgeDesc->GetRightRailingSystem();

   Float64 Wint = 0.0; // weight of interior barrier
   if ( pRailingSystem->bUseInteriorRailing )
   {
      if ( pRailingSystem->GetInteriorRailing()->GetWeightMethod() == TrafficBarrierEntry::Compute )
      {
         CComPtr<IPolyShape> polyshape;
         pRailingSystem->GetInteriorRailing()->CreatePolyShape(orientation,&polyshape);

         CComQIPtr<IShape> shape(polyshape);
         CComPtr<IShapeProperties> props;
         shape->get_ShapeProperties(&props);
         Float64 area;
         props->get_Area(&area);

         Float64 density = GetDensityRailing(orientation);
         Float64 mplBarrier = area * density;
         Float64 g = unitSysUnitsMgr::GetGravitationalAcceleration();
         Wint = mplBarrier * g;
      }
      else
      {
         Wint = pRailingSystem->GetInteriorRailing()->GetWeight();
      }
   }

   return Wint;
}

bool CBridgeAgentImp::HasInteriorBarrier(pgsTypes::TrafficBarrierOrientation orientation)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CRailingSystem* pRailingSystem = NULL;
   if ( orientation == pgsTypes::tboLeft )
      pRailingSystem = pBridgeDesc->GetLeftRailingSystem();
   else
      pRailingSystem = pBridgeDesc->GetRightRailingSystem();

   return  pRailingSystem->bUseInteriorRailing;
}

Float64 CBridgeAgentImp::GetExteriorBarrierCgToDeckEdge(pgsTypes::TrafficBarrierOrientation orientation)
{
   // Use generic bridge - barriers have been placed properly
   CComPtr<ISidewalkBarrier> pSwBarrier;
   Float64 sign;
   if ( orientation == pgsTypes::tboLeft )
   {
      m_Bridge->get_LeftBarrier(&pSwBarrier);
      sign = 1.0;
   }
   else
   {
      m_Bridge->get_RightBarrier(&pSwBarrier);
      sign = -1.0;
   }

   CComPtr<IBarrier> pBarrier;
   pSwBarrier->get_ExteriorBarrier(&pBarrier);

   CComQIPtr<IShape> shape;
   pBarrier->get_Shape(&shape);

   CComPtr<IShapeProperties> props;
   shape->get_ShapeProperties(&props);

   CComPtr<IPoint2d> cgpoint;
   props->get_Centroid(&cgpoint);

   Float64 xcg;
   cgpoint->get_X(&xcg);

   return xcg*sign;
}

Float64 CBridgeAgentImp::GetInteriorBarrierCgToDeckEdge(pgsTypes::TrafficBarrierOrientation orientation)
{
   // Use generic bridge - barriers have been placed properly
   CComPtr<ISidewalkBarrier> pSwBarrier;
   Float64 sign;
   if ( orientation == pgsTypes::tboLeft )
   {
      m_Bridge->get_LeftBarrier(&pSwBarrier);
      sign = 1.0;
   }
   else
   {
      m_Bridge->get_RightBarrier(&pSwBarrier);
      sign = -1.0;
   }

   CComPtr<IBarrier> pBarrier;
   pSwBarrier->get_InteriorBarrier(&pBarrier);

   CComQIPtr<IShape> shape;
   pBarrier->get_Shape(&shape);

   if (shape)
   {
      CComPtr<IShapeProperties> props;
      shape->get_ShapeProperties(&props);

      CComPtr<IPoint2d> cgpoint;
      props->get_Centroid(&cgpoint);

      Float64 xcg;
      cgpoint->get_X(&xcg);

      return xcg*sign;
   }
   else
   {
      ATLASSERT(0); // client should be checking this
      return 0.0;
   }
}


Float64 CBridgeAgentImp::GetInterfaceWidth(pgsTypes::TrafficBarrierOrientation orientation)
{
   // This is the offset from the edge of deck to the curb line (basically the connection 
   // width of the barrier)
   CComPtr<ISidewalkBarrier> barrier;

   if ( orientation == pgsTypes::tboLeft )
      m_Bridge->get_LeftBarrier(&barrier);
   else
      m_Bridge->get_RightBarrier(&barrier);

   Float64 offset;
   barrier->get_ExteriorCurbWidth(&offset);
   return offset;
}


void CBridgeAgentImp::GetSidewalkDeadLoadEdges(pgsTypes::TrafficBarrierOrientation orientation, Float64* pintEdge, Float64* pextEdge)
{
   VALIDATE(BRIDGE);

   CComPtr<ISidewalkBarrier> barrier;
   if ( orientation == pgsTypes::tboLeft )
   {
      m_Bridge->get_LeftBarrier(&barrier);
   }
   else
   {
      m_Bridge->get_RightBarrier(&barrier);
   }

   VARIANT_BOOL has_sw;
   barrier->get_HasSidewalk(&has_sw);

   Float64 width = 0;
   if ( has_sw!=VARIANT_FALSE )
   {
      CComPtr<IShape> swShape;
      barrier->get_SidewalkShape(&swShape);

      // slab extends to int side of int box if it exists
      CComPtr<IRect2d> bbox;
      swShape->get_BoundingBox(&bbox);

      if ( orientation == pgsTypes::tboLeft )
      {
         bbox->get_Left(pextEdge);
         bbox->get_Right(pintEdge);
      }
      else
      {
         bbox->get_Left(pintEdge);
         bbox->get_Right(pextEdge);
      }
   }
   else
   {
      ATLASSERT(0); // client should not call this if no sidewalk
   }
}

void CBridgeAgentImp::GetSidewalkPedLoadEdges(pgsTypes::TrafficBarrierOrientation orientation, Float64* pintEdge, Float64* pextEdge)
{
   VALIDATE(BRIDGE);

   CComPtr<ISidewalkBarrier> swbarrier;
   if ( orientation == pgsTypes::tboLeft )
   {
      m_Bridge->get_LeftBarrier(&swbarrier);
   }
   else
   {
      m_Bridge->get_RightBarrier(&swbarrier);
   }

   VARIANT_BOOL has_sw;
   swbarrier->get_HasSidewalk(&has_sw);
   if(has_sw!=VARIANT_FALSE)
   {
      CComPtr<IBarrier> extbarrier;
      swbarrier->get_ExteriorBarrier(&extbarrier);

      // Sidewalk width for ped - sidewalk goes from interior edge of exterior barrier to sw width
      CComPtr<IShape> pextbarshape;
      extbarrier->get_Shape(&pextbarshape);
      CComPtr<IRect2d> bbox;
      pextbarshape->get_BoundingBox(&bbox);

      // exterior edge
      if ( orientation == pgsTypes::tboLeft )
      {
         bbox->get_Right(pextEdge);
      }
      else
      {
         bbox->get_Left(pextEdge);
         *pextEdge *= -1.0;
      }

      Float64 width;
      swbarrier->get_SidewalkWidth(&width);

      *pintEdge = *pextEdge + width;
   }
   else
   {
      ATLASSERT(0); // client should not call this if no sidewalk
      *pintEdge = 0.0;
      *pextEdge = 0.0;
   }
}

pgsTypes::TrafficBarrierOrientation CBridgeAgentImp::GetNearestBarrier(SpanIndexType span,GirderIndexType gdr)
{
   GirderIndexType nGirders = GetGirderCount(span);
   if ( gdr < nGirders/2 )
      return pgsTypes::tboLeft;
   else
      return pgsTypes::tboRight;
}

/////////////////////////////////////////////////////////////////////////
// IGirder
//
bool CBridgeAgentImp::IsPrismatic(pgsTypes::Stage stage,SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   VALIDATE( BRIDGE );
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(spanIdx);
   const GirderLibraryEntry* pGirderEntry = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);

#if defined _DEBUG
   std::_tstring strGirderName = pSpan->GetGirderTypes()->GetGirderName(gdrIdx);
   ATLASSERT( strGirderName == pGirderEntry->GetName() );
#endif

   CComPtr<IBeamFactory> beamFactory;
   pGirderEntry->GetBeamFactory(&beamFactory);

   bool bPrismaticGirder = beamFactory->IsPrismatic(m_pBroker,spanIdx,gdrIdx);
   if ( !bPrismaticGirder )
      return false; // if the bare girder is non-prismiatc... it will always be non-prismatic

   if ( IsCompositeDeck() )
   {
      // there is a composite deck... compare the stage when the deck is made composite
      // against the stage we are evaluating prismatic-ness to
      CComPtr<IBridgeDeck> deck;
      m_Bridge->get_Deck(&deck);
      CComBSTR bstrCompositeStage;
      deck->get_CompositeStage(&bstrCompositeStage);

      pgsTypes::Stage compositeStage = GetStageType(bstrCompositeStage);

      // if the stage we are evaluating is before the composite stage then
      // we just have a bare girder... return its prismatic-ness
      if ( StageCompare(stage,compositeStage) < 0 )
         return bPrismaticGirder;
   }

   // we have a prismatic made composite and are evaluating the girder in a composite stage.
   // check to see if the composite section is non-prismatic

   // check the direction of each girder... if they aren't parallel, then the composite properties
   // are non-prismatic
   // for exterior girders, if the deck edge offset isn't the same at each end of the span,
   // then the girder will be considered to be non-prismatic as well.
   //
   // this may not be the best way to figure it out, but it mostly works well and there isn't
   // any harm in being wrong except that section properties will be reported verbosely for
   // prismatic beams when a compact reporting would suffice.
   CComPtr<IDirection> dirLeftGirder, dirThisGirder, dirRightGirder;
   Float64 startOverhang = -1;
   Float64 endOverhang = -1;
   Float64 midspanOverhang = -1;
   Float64 leftDir,thisDir,rightDir;
   bool bIsExterior = IsExteriorGirder(spanIdx,gdrIdx);
   if ( bIsExterior )
   {
      GirderIndexType nGirders = GetGirderCount(spanIdx);
      PierIndexType prevPierIdx = spanIdx;
      PierIndexType nextPierIdx = prevPierIdx+1;
      Float64 prevPierStation = GetPierStation(prevPierIdx);
      Float64 nextPierStation = GetPierStation(nextPierIdx);
      Float64 spanLength = nextPierStation - prevPierStation;

      if ( nGirders == 1 )
      {
         // if there is only one girder, then consider the section prismatic if the left and right 
         // overhand match at both ends and mid-span of the span
         Float64 startOverhangLeft = GetLeftSlabOverhang(prevPierIdx);
         Float64 endOverhangLeft   = GetLeftSlabOverhang(nextPierIdx);
         Float64 midspanOverhangLeft = GetLeftSlabOverhang(spanIdx,spanLength/2);

         bool bLeftEqual = IsEqual(startOverhangLeft,midspanOverhangLeft) && IsEqual(midspanOverhangLeft,endOverhangLeft);


         Float64 startOverhangRight = GetRightSlabOverhang(prevPierIdx);
         Float64 endOverhangRight   = GetRightSlabOverhang(nextPierIdx);
         Float64 midspanOverhangRight = GetRightSlabOverhang(spanIdx,spanLength/2);

         bool bRightEqual = IsEqual(startOverhangRight,midspanOverhangRight) && IsEqual(midspanOverhangRight,endOverhangRight);

         return (bLeftEqual && bRightEqual);
      }
      else
      {
         if ( gdrIdx == 0 )
         {
            // left exterior girder
            startOverhang = GetLeftSlabOverhang(prevPierIdx);
            endOverhang   = GetLeftSlabOverhang(nextPierIdx);
            midspanOverhang = GetLeftSlabOverhang(spanIdx,spanLength/2);

            GetGirderBearing(spanIdx,gdrIdx,  &dirThisGirder);
            GetGirderBearing(spanIdx,gdrIdx+1,&dirRightGirder);
            dirThisGirder->get_Value(&thisDir);
            dirRightGirder->get_Value(&rightDir);
         }
         else
         {
            // right exterior girder
            GetGirderBearing(spanIdx,gdrIdx-1,&dirLeftGirder);
            GetGirderBearing(spanIdx,gdrIdx,  &dirThisGirder);
            dirLeftGirder->get_Value(&leftDir);
            dirThisGirder->get_Value(&thisDir);

            startOverhang = GetRightSlabOverhang(prevPierIdx);
            endOverhang   = GetRightSlabOverhang(nextPierIdx);
            midspanOverhang = GetRightSlabOverhang(spanIdx,spanLength/2);
         }
      }
   }
   else
   {
      // if interior girders are not parallel, then they aren't going to be
      // prismatic if they have a composite deck
      GetGirderBearing(spanIdx,gdrIdx-1,&dirLeftGirder);
      GetGirderBearing(spanIdx,gdrIdx,  &dirThisGirder);
      GetGirderBearing(spanIdx,gdrIdx+1,&dirRightGirder);

      dirLeftGirder->get_Value(&leftDir);
      dirThisGirder->get_Value(&thisDir);
      dirRightGirder->get_Value(&rightDir);
   }

   if ( dirLeftGirder == NULL )
   {
      return (IsEqual(thisDir,rightDir) && IsEqual(startOverhang,midspanOverhang) && IsEqual(midspanOverhang,endOverhang) ? true : false);
   }
   else if ( dirRightGirder == NULL )
   {
      return (IsEqual(leftDir,thisDir) && IsEqual(startOverhang,midspanOverhang) && IsEqual(midspanOverhang,endOverhang) ? true : false);
   }
   else
   {
      return (IsEqual(leftDir,thisDir) && IsEqual(thisDir,rightDir) ? true : false);
   }
}

bool CBridgeAgentImp::IsSymmetric(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr)
{
   // need to look at girder spacing to see if it is constant
   if ( stage == pgsTypes::BridgeSite3 )
      return false;

#pragma Reminder("UPDATE: Figure out if girder is symmetric")
   // other places in the program can gain huge speed efficiencies if the 
   // girder is symmetric (only half the work needs to be done)
   Float64 start_length = GetGirderStartConnectionLength(span,gdr);
   Float64 end_length   = GetGirderEndConnectionLength(span,gdr);
   
   if ( !IsEqual(start_length,end_length) )
      return false; // different end lengths

   Float64 girder_length = GetGirderLength(span,gdr);

   Float64 left_hp, right_hp;
   GetHarpingPointLocations(span,gdr,&left_hp,&right_hp);
   if ( !IsEqual(left_hp,girder_length - right_hp) )
      return false; // different harping point locations

   if ( !IsDebondingSymmetric(span,gdr) )
      return false;

   return true;
}

MatingSurfaceIndexType CBridgeAgentImp::GetNumberOfMatingSurfaces(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(pgsPointOfInterest(spanIdx,gdrIdx,0.00),&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   FlangeIndexType count;
   girder_section->get_MatingSurfaceCount(&count);

   return count;
}

Float64 CBridgeAgentImp::GetMatingSurfaceLocation(const pgsPointOfInterest& poi,MatingSurfaceIndexType webIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 location;
   girder_section->get_MatingSurfaceLocation(webIdx,&location);
   
   return location;
}

Float64 CBridgeAgentImp::GetMatingSurfaceWidth(const pgsPointOfInterest& poi,MatingSurfaceIndexType webIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 width;
   girder_section->get_MatingSurfaceWidth(webIdx,&width);

   return width;
}

FlangeIndexType CBridgeAgentImp::GetNumberOfTopFlanges(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(pgsPointOfInterest(spanIdx,gdrIdx,0.00),&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   FlangeIndexType nFlanges;
   girder_section->get_TopFlangeCount(&nFlanges);
   return nFlanges;
}

Float64 CBridgeAgentImp::GetTopFlangeLocation(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 location;
   girder_section->get_TopFlangeLocation(flangeIdx,&location);
   return location;
}

Float64 CBridgeAgentImp::GetTopFlangeWidth(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 width;
   girder_section->get_TopFlangeWidth(flangeIdx,&width);
   return width;
}

Float64 CBridgeAgentImp::GetTopFlangeThickness(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 thickness;
   girder_section->get_TopFlangeThickness(flangeIdx,&thickness);
   return thickness;
}

Float64 CBridgeAgentImp::GetTopFlangeSpacing(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 spacing;
   girder_section->get_TopFlangeSpacing(flangeIdx,&spacing);
   return spacing;
}

Float64 CBridgeAgentImp::GetTopFlangeWidth(const pgsPointOfInterest& poi)
{
   MatingSurfaceIndexType nMS = GetNumberOfMatingSurfaces(poi.GetSpan(),poi.GetGirder());
   Float64 wtf = 0;
   for ( MatingSurfaceIndexType msIdx = 0; msIdx < nMS; msIdx++ )
   {
      Float64 msw;
      msw = GetMatingSurfaceWidth(poi,msIdx);
      wtf += msw;
}

   return wtf;
}

Float64 CBridgeAgentImp::GetTopWidth(const pgsPointOfInterest& poi)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 width;
   girder_section->get_TopWidth(&width);

   return width;
}

FlangeIndexType CBridgeAgentImp::GetNumberOfBottomFlanges(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(pgsPointOfInterest(spanIdx,gdrIdx,0.00),&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   FlangeIndexType nFlanges;
   girder_section->get_BottomFlangeCount(&nFlanges);
   return nFlanges;
}

Float64 CBridgeAgentImp::GetBottomFlangeLocation(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 location;
   girder_section->get_BottomFlangeLocation(flangeIdx,&location);
   return location;
}

Float64 CBridgeAgentImp::GetBottomFlangeWidth(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 width;
   girder_section->get_BottomFlangeWidth(flangeIdx,&width);
   return width;
}

Float64 CBridgeAgentImp::GetBottomFlangeThickness(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 thickness;
   girder_section->get_BottomFlangeThickness(flangeIdx,&thickness);
   return thickness;
}

Float64 CBridgeAgentImp::GetBottomFlangeSpacing(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 spacing;
   girder_section->get_BottomFlangeSpacing(flangeIdx,&spacing);
   return spacing;
}

Float64 CBridgeAgentImp::GetBottomFlangeWidth(const pgsPointOfInterest& poi)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 width = 0;
   FlangeIndexType nFlanges;
   girder_section->get_BottomFlangeCount(&nFlanges);
   for ( FlangeIndexType idx = 0; idx < nFlanges; idx++ )
   {
      Float64 w;
      girder_section->get_BottomFlangeWidth(idx,&w);
      width += w;
   }

   return width;
}

Float64 CBridgeAgentImp::GetBottomWidth(const pgsPointOfInterest& poi)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 width;
   girder_section->get_BottomWidth(&width);

   return width;
}

Float64 CBridgeAgentImp::GetMinWebWidth(const pgsPointOfInterest& poi)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 width;
   girder_section->get_MinWebThickness(&width);

   return width;
}

Float64 CBridgeAgentImp::GetMinTopFlangeThickness(const pgsPointOfInterest& poi)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 ttf;
   girder_section->get_MinTopFlangeThickness(&ttf);
   return ttf;
}

Float64 CBridgeAgentImp::GetMinBottomFlangeThickness(const pgsPointOfInterest& poi)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 tbf;
   girder_section->get_MinBottomFlangeThickness(&tbf);
   return tbf;
}

Float64 CBridgeAgentImp::GetHeight(const pgsPointOfInterest& poi)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 height;
   girder_section->get_GirderHeight(&height);

   return height;
}

Float64 CBridgeAgentImp::GetShearWidth(const pgsPointOfInterest& poi)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 shear_width;
   girder_section->get_ShearWidth(&shear_width);
   return shear_width;
}

Float64 CBridgeAgentImp::GetShearInterfaceWidth(const pgsPointOfInterest& poi)
{
   VALIDATE( BRIDGE );

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 wMating = 0; // sum of mating surface widths... less deck panel support width
   if ( pDeck->DeckType == pgsTypes::sdtCompositeCIP || pDeck->DeckType == pgsTypes::sdtCompositeOverlay )
   {
      SpanIndexType span  = poi.GetSpan();
      GirderIndexType gdr = poi.GetGirder();
      MatingSurfaceIndexType nMatingSurfaces = GetNumberOfMatingSurfaces(span,gdr);
      for ( MatingSurfaceIndexType i = 0; i < nMatingSurfaces; i++ )
      {
         wMating += GetMatingSurfaceWidth(poi,i);
      }
   }
   else if ( pDeck->DeckType == pgsTypes::sdtCompositeSIP )
   {
      // SIP Deck Panel System... Area beneath the deck panesl aren't part of the
      // shear transfer area

      SpanIndexType span  = poi.GetSpan();
      GirderIndexType gdr = poi.GetGirder();
      MatingSurfaceIndexType nMatingSurfaces = GetNumberOfMatingSurfaces(span,gdr);
      Float64 panel_support = pDeck->PanelSupport;
      for ( MatingSurfaceIndexType i = 0; i < nMatingSurfaces; i++ )
      {
         if ( IsExteriorGirder(span,gdr) && 
              ((gdr == 0 && i == 0) || // Left exterior girder
               (gdr == GetGirderCount(span)-1 && i == nMatingSurfaces-1))  // right exterior girder
            )
            wMating += GetMatingSurfaceWidth(poi,i) - panel_support;
         else
            wMating += GetMatingSurfaceWidth(poi,i) - 2*panel_support;
      }

      if ( wMating < 0 )
      {
         wMating = 0;

         CString strMsg;
         strMsg.Format(_T("Span %d Girder %s, Deck panel support width exceeds half the width of the supporting flange. An interface shear width of 0.0 will be used"),LABEL_SPAN(span),LABEL_GIRDER(gdr));
         pgsBridgeDescriptionStatusItem* pStatusItem = 
            new pgsBridgeDescriptionStatusItem(m_StatusGroupID,m_scidBridgeDescriptionWarning,3,strMsg);

         GET_IFACE(IEAFStatusCenter,pStatusCenter);
         pStatusCenter->Add(pStatusItem);

      }
   }
   else
   {
      // all other deck types are non-composite so there is no
      // interface width for horizontal shear
      wMating = 0;
   }

   return wMating;
}

WebIndexType CBridgeAgentImp::GetNumberOfWebs(SpanIndexType span,GirderIndexType gdr)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(pgsPointOfInterest(span,gdr,0.00),&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   WebIndexType count;
   girder_section->get_WebCount(&count);

   return count;
}

Float64 CBridgeAgentImp::GetWebLocation(const pgsPointOfInterest& poi,WebIndexType webIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 location;
   girder_section->get_WebLocation(webIdx,&location);
   
   return location;
}

Float64 CBridgeAgentImp::GetWebSpacing(const pgsPointOfInterest& poi,SpacingIndexType spaceIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 spacing;
   girder_section->get_WebSpacing(spaceIdx,&spacing);

   return spacing;
}

Float64 CBridgeAgentImp::GetWebThickness(const pgsPointOfInterest& poi,WebIndexType webIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 thickness;
   girder_section->get_WebThickness(webIdx,&thickness);
   
   return thickness;
}

Float64 CBridgeAgentImp::GetCL2ExteriorWebDistance(const pgsPointOfInterest& poi)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   GET_IFACE(IBarriers,         pBarriers);
   pgsTypes::TrafficBarrierOrientation side = pBarriers->GetNearestBarrier(poi.GetSpan(),poi.GetGirder());
   DirectionType dtside = (side==pgsTypes::tboLeft) ? qcbLeft : qcbRight;

   Float64 dist;
   girder_section->get_CL2ExteriorWebDistance(dtside, &dist);
   
   return dist;
}

void CBridgeAgentImp::GetGirderEndPoints(SpanIndexType span,GirderIndexType gdr,IPoint2d** pntPier1,IPoint2d** pntEnd1,IPoint2d** pntBrg1,IPoint2d** pntBrg2,IPoint2d** pntEnd2,IPoint2d** pntPier2)
{
   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   girder->GetEndPoints(pntPier1,pntEnd1,pntBrg1,pntBrg2,pntEnd2,pntPier2);
}

Float64 CBridgeAgentImp::GetOrientation(SpanIndexType span,GirderIndexType gdr)
{
   ValidateGirderOrientation(span,gdr);

   CComPtr<ISuperstructureMember> ssmbr;
   GetSuperstructureMember(span,gdr,&ssmbr);

   CComPtr<ISegment> segment;
   ssmbr->get_Segment(0,&segment);

   Float64 orientation;
   segment->get_Orientation(&orientation);


   return orientation;
}

Float64 CBridgeAgentImp::GetTopGirderReferenceChordElevation(const pgsPointOfInterest& poi)
{
   // top of girder elevation at the poi, ignoring camber effects
   // camber effects are ignored
   VALIDATE( BRIDGE );

   SpanIndexType span = poi.GetSpan();
   GirderIndexType gdr  = poi.GetGirder();

   Float64 end_size = GetGirderStartConnectionLength(span,gdr);

   // get station at offset at start bearing
   Float64 station, offset;
   GetStationAndOffset(pgsPointOfInterest(span,gdr,end_size),&station,&offset);

   // the girder reference line passes through the deck at this station and offset
   Float64 Y_girder_ref_line_left_bearing = GetElevation(station,offset);

   // slope of the girder in the plane of the girder
   Float64 girder_slope = GetGirderSlope(span,gdr);

   // top of girder elevation (ignoring camber effects)
   Float64 dist_from_left_bearing = poi.GetDistFromStart() - end_size;
   Float64 yc = Y_girder_ref_line_left_bearing + dist_from_left_bearing*girder_slope;

   return yc;
}

Float64 CBridgeAgentImp::GetTopGirderElevation(const pgsPointOfInterest& poi,MatingSurfaceIndexType matingSurfaceIndex)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   GDRCONFIG dummy_config;

   return GetTopGirderElevation(poi,false,dummy_config,matingSurfaceIndex);
}

Float64 CBridgeAgentImp::GetTopGirderElevation(const pgsPointOfInterest& poi,const GDRCONFIG& config,MatingSurfaceIndexType matingSurfaceIdx)
{
   return GetTopGirderElevation(poi,true,config,matingSurfaceIdx);
}

Float64 CBridgeAgentImp::GetTopGirderElevation(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,MatingSurfaceIndexType matingSurfaceIdx)
{
   VALIDATE(GIRDER);

   SpanIndexType span = poi.GetSpan();
   GirderIndexType gdr  = poi.GetGirder();
   Float64 end_size = GetGirderStartConnectionLength(span,gdr);

   // girder offset at start bearing
   Float64 station, zs;
   GetStationAndOffset(pgsPointOfInterest(span,gdr,end_size),&station,&zs);

   Float64 webOffset = (matingSurfaceIdx == INVALID_INDEX ? 0 : GetMatingSurfaceLocation(poi,matingSurfaceIdx));

   // roadway surface elevation at start bearing, directly above web centerline
   Float64 Ysb = GetElevation(station,zs + webOffset);

   Float64 girder_slope = GetGirderSlope(span,gdr);

   Float64 dist_from_start_bearing = poi.GetDistFromStart() - end_size;

   Float64 slab_offset_at_start = GetSlabOffset(span,gdr,pgsTypes::metStart);

   // get the camber
   GET_IFACE(ICamber,pCamber);
   Float64 excess_camber = (bUseConfig ? pCamber->GetExcessCamber(poi,config,CREEP_MAXTIME)
                                       : pCamber->GetExcessCamber(poi,CREEP_MAXTIME) );

   Float64 top_gdr_elev = Ysb - slab_offset_at_start + girder_slope*dist_from_start_bearing + excess_camber;
   return top_gdr_elev;
}

void CBridgeAgentImp::GetProfileShape(SpanIndexType spanIdx,GirderIndexType gdrIdx,IShape** ppShape)
{
   VALIDATE(GIRDER);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(spanIdx);
   GirderIndexType nGirders = pSpan->GetGirderCount();
   gdrIdx = min(nGirders-1,gdrIdx);
   const GirderLibraryEntry* pGirderEntry = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);

#if defined _DEBUG
   std::_tstring strGirderName = pSpan->GetGirderTypes()->GetGirderName(gdrIdx);
   ATLASSERT( strGirderName == pGirderEntry->GetName() );
#endif

   CComPtr<IBeamFactory> beamFactory;
   pGirderEntry->GetBeamFactory(&beamFactory);

   beamFactory->CreateGirderProfile(m_pBroker,m_StatusGroupID,spanIdx,gdrIdx,pGirderEntry->GetDimensions(),ppShape);
}

bool CBridgeAgentImp::HasShearKey(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::SupportedBeamSpacing spacingType)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(spanIdx);
   GirderIndexType nGirders = pSpan->GetGirderCount();
   gdrIdx = min(nGirders-1,gdrIdx);
   const GirderLibraryEntry* pGirderEntry = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);
   CComPtr<IBeamFactory> beamFactory;
   pGirderEntry->GetBeamFactory(&beamFactory);

   return beamFactory->IsShearKey(pGirderEntry->GetDimensions(), spacingType);
}

void CBridgeAgentImp::GetShearKeyAreas(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(spanIdx);
   GirderIndexType nGirders = pSpan->GetGirderCount();
   gdrIdx = min(nGirders-1,gdrIdx);
   const GirderLibraryEntry* pGirderEntry = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);
   CComPtr<IBeamFactory> beamFactory;
   pGirderEntry->GetBeamFactory(&beamFactory);

   beamFactory->GetShearKeyAreas(pGirderEntry->GetDimensions(), spacingType, uniformArea, areaPerJoint);
}


////////////////////////////////////////////////////////////////////////
// IGirderLiftingPointsOfInterest
std::vector<pgsPointOfInterest> CBridgeAgentImp::GetLiftingPointsOfInterest(SpanIndexType span,GirderIndexType gdr,PoiAttributeType attrib,Uint32 mode)
{
   ValidatePointsOfInterest(span,gdr);

   Uint32 mgrMode = (mode == POIFIND_AND ? POIMGR_AND : POIMGR_OR);

   std::vector<pgsPointOfInterest> poi;
   m_PoiMgr.GetPointsOfInterest(span, gdr,  pgsTypes::Lifting, attrib, mgrMode, &poi );
   return poi;
}

std::vector<pgsPointOfInterest> CBridgeAgentImp::GetLiftingDesignPointsOfInterest(SpanIndexType span,GirderIndexType gdr,Float64 overhang,PoiAttributeType attrib,Uint32 mode)
{
   // Generates points of interest for the supplied overhang.
   pgsPoiMgr poiMgr;
   LayoutHandlingPoi(pgsTypes::Lifting,span,gdr,10,overhang,overhang,attrib,POI_PICKPOINT,&poiMgr);

   Uint32 mgrMode = (mode == POIFIND_AND ? POIMGR_AND : POIMGR_OR);
   std::vector<pgsPointOfInterest> poi;
   poiMgr.GetPointsOfInterest( span, gdr, pgsTypes::Lifting, attrib, mgrMode, &poi );
   return poi;
}

////////////////////////////////////////////////////////////////////////
// IGirderHaulingPointsOfInterest
std::vector<pgsPointOfInterest> CBridgeAgentImp::GetHaulingPointsOfInterest(SpanIndexType span,GirderIndexType gdr,PoiAttributeType attrib,Uint32 mode)
{
   ValidatePointsOfInterest(span,gdr);

   Uint32 mgrMode = (mode == POIFIND_AND ? POIMGR_AND : POIMGR_OR);

   std::vector<pgsPointOfInterest> poi;
   m_PoiMgr.GetPointsOfInterest( span, gdr, pgsTypes::Hauling, attrib, mgrMode, &poi );
   return poi;
}

std::vector<pgsPointOfInterest> CBridgeAgentImp::GetHaulingDesignPointsOfInterest(SpanIndexType span,GirderIndexType gdr,Float64 leftOverhang,Float64 rightOverhang,PoiAttributeType attrib,Uint32 mode)
{
   // Generates points of interest for the supplied overhang.
   pgsPoiMgr poiMgr;
   LayoutHandlingPoi(pgsTypes::Hauling,span,gdr,10,leftOverhang,rightOverhang,attrib,POI_BUNKPOINT,&poiMgr);

   Uint32 mgrMode = (mode == POIFIND_AND ? POIMGR_AND : POIMGR_OR);
   std::vector<pgsPointOfInterest> poi;
   poiMgr.GetPointsOfInterest( span, gdr, pgsTypes::Hauling, attrib, mgrMode, &poi );
   return poi;
}

Float64 CBridgeAgentImp::GetMinimumOverhang(SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE(IBridge,pBridge);
   Float64 gdrLength = pBridge->GetGirderLength(span,gdr);

   GET_IFACE(IGirderHaulingSpecCriteria,pCriteria);
   Float64 maxDistBetweenSupports = pCriteria->GetAllowableDistanceBetweenSupports();

   Float64 minOverhang = (gdrLength - maxDistBetweenSupports)/2;
   if ( minOverhang < 0 )
      minOverhang = 0;

   return minOverhang;
}

////////////////////////////////////////////////////////////////////////
// IUserDefinedLoads
bool CBridgeAgentImp::DoUserLoadsExist(SpanIndexType span,GirderIndexType gdr)
{
   SpanIndexType startSpan = (span == ALL_SPANS ? 0 : span);
   SpanIndexType endSpan   = (span == ALL_SPANS ? GetSpanCount() : startSpan+1);

   for ( SpanIndexType spanIdx = startSpan; spanIdx < endSpan; spanIdx++ )
   {
      const std::vector<IUserDefinedLoads::UserPointLoad>* uplv = GetPointLoads(pgsTypes::BridgeSite1, spanIdx, gdr);
      if (uplv!=NULL && !uplv->empty())
         return true;

      uplv = GetPointLoads(pgsTypes::BridgeSite2, spanIdx, gdr);
      if (uplv!=NULL && !uplv->empty())
         return true;

      uplv = GetPointLoads(pgsTypes::BridgeSite3, spanIdx, gdr);
      if (uplv!=NULL && !uplv->empty())
         return true;

      const std::vector<IUserDefinedLoads::UserDistributedLoad>* udlv = GetDistributedLoads(pgsTypes::BridgeSite1, spanIdx, gdr);
      if (udlv!=NULL && !udlv->empty())
         return true;

      udlv = GetDistributedLoads(pgsTypes::BridgeSite2, spanIdx, gdr);
      if (udlv!=NULL && !udlv->empty())
         return true;

      udlv = GetDistributedLoads(pgsTypes::BridgeSite3, spanIdx, gdr);
      if (udlv!=NULL && !udlv->empty())
         return true;


      const std::vector<IUserDefinedLoads::UserMomentLoad>* umlv = GetMomentLoads(pgsTypes::BridgeSite1, spanIdx, gdr);
      if (umlv!=NULL && !umlv->empty())
         return true;

      umlv = GetMomentLoads(pgsTypes::BridgeSite2, spanIdx, gdr);
      if (umlv!=NULL && !umlv->empty())
         return true;

      umlv = GetMomentLoads(pgsTypes::BridgeSite3, spanIdx, gdr);
      if (umlv!=NULL && !umlv->empty())
         return true;
   }

   return false;
}

const std::vector<IUserDefinedLoads::UserPointLoad>* CBridgeAgentImp::GetPointLoads(pgsTypes::Stage stage, SpanIndexType span,GirderIndexType gdr)
{
   std::vector<UserPointLoad>* vec = GetUserPointLoads(stage, span, gdr);
   return vec;
}

const std::vector<IUserDefinedLoads::UserDistributedLoad>* CBridgeAgentImp::GetDistributedLoads(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr)
{
   std::vector<UserDistributedLoad>* vec = GetUserDistributedLoads(stage, span, gdr);
   return vec;
}

const std::vector<IUserDefinedLoads::UserMomentLoad>* CBridgeAgentImp::GetMomentLoads(pgsTypes::Stage stage, SpanIndexType span,GirderIndexType gdr)
{
   std::vector<UserMomentLoad>* vec = GetUserMomentLoads(stage, span, gdr);
   return vec;
}


std::vector<IUserDefinedLoads::UserPointLoad>* CBridgeAgentImp::GetUserPointLoads(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr)
{
   ValidateUserLoads();

   if (stage!=pgsTypes::CastingYard)
   {
      SpanGirderHashType hash = HashSpanGirder(span, gdr);

      PointLoadCollection::iterator it = m_PointLoads[stage-1].find(hash);

      if (it!= m_PointLoads[stage-1].end())
      {
         return &(it->second);
      }
   }

   return 0;
}


std::vector<IUserDefinedLoads::UserDistributedLoad>* CBridgeAgentImp::GetUserDistributedLoads(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr)
{
   ValidateUserLoads();

   if (stage!=pgsTypes::CastingYard)
   {
      SpanGirderHashType hash = HashSpanGirder(span, gdr);

      DistributedLoadCollection::iterator it = m_DistributedLoads[stage-1].find(hash);

      if (it!= m_DistributedLoads[stage-1].end())
      {
         return &(it->second);
      }
   }

   return 0;
}


std::vector<IUserDefinedLoads::UserMomentLoad>* CBridgeAgentImp::GetUserMomentLoads(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr)
{
   ValidateUserLoads();

   if (stage!=pgsTypes::CastingYard)
   {
      SpanGirderHashType hash = HashSpanGirder(span, gdr);

      MomentLoadCollection::iterator it = m_MomentLoads[stage-1].find(hash);

      if (it!= m_MomentLoads[stage-1].end())
      {
         return &(it->second);
      }
   }

   return 0;
}

////////////////////////////////////////////////////////////////////////
// IBridgeDescriptionEventSink
//
HRESULT CBridgeAgentImp::OnBridgeChanged()
{
//   LOG(_T("OnBridgeChanged Event Received"));
   INVALIDATE( CLEAR_ALL );
   return S_OK;
}

HRESULT CBridgeAgentImp::OnGirderFamilyChanged()
{
//   LOG(_T("OnGirderFamilyChanged Event Received"));
   INVALIDATE( CLEAR_ALL );
   return S_OK;
}

HRESULT CBridgeAgentImp::OnGirderChanged(SpanIndexType span,GirderIndexType gdr,Uint32 lHint)
{
   INVALIDATE( CLEAR_ALL );
   return S_OK;
}

HRESULT CBridgeAgentImp::OnLiveLoadChanged()
{
   // No changes necessary to bridge model
   LOG(_T("OnLiveLoadChanged Event Received"));
   return S_OK;
}

HRESULT CBridgeAgentImp::OnLiveLoadNameChanged(LPCTSTR strOldName,LPCTSTR strNewName)
{
   // No changes necessary to bridge model
   LOG(_T("OnLiveLoadNameChanged Event Received"));
   return S_OK;
}

HRESULT CBridgeAgentImp::OnConstructionLoadChanged()
{
   LOG(_T("OnConstructionLoadChanged Event Received"));
   return S_OK;
}

////////////////////////////////////////////////////////////////////////
// ISpecificationEventSink
HRESULT CBridgeAgentImp::OnSpecificationChanged()
{
//   LOG(_T("OnSpecificationChanged Event Received"));
   INVALIDATE( CLEAR_ALL );
   return S_OK;
}

HRESULT CBridgeAgentImp::OnAnalysisTypeChanged()
{
   // nothing to do here!
   return S_OK;
}

void CBridgeAgentImp::GetAlignment(IAlignment** ppAlignment)
{
   VALIDATE( COGO_MODEL );

   CComPtr<ICogoModel> cogomodel;
   m_Bridge->get_CogoModel(&cogomodel);

   CComPtr<IPathCollection> alignments;
   cogomodel->get_Alignments(&alignments);

   CComPtr<ICogoInfo> cogoinfo;
   m_Bridge->get_CogoInfo(&cogoinfo);

   CogoElementKey alignmentkey;
   cogoinfo->get_AlignmentKey(&alignmentkey);

   CComPtr<IPath> path;
   alignments->get_Item(alignmentkey,&path);
   path->QueryInterface(ppAlignment);
}

void CBridgeAgentImp::GetProfile(IProfile** ppProfile)
{
   CComPtr<IAlignment> alignment;
   GetAlignment(&alignment);

   alignment->get_Profile(ppProfile);
}

void CBridgeAgentImp::GetBarrierProperties(pgsTypes::TrafficBarrierOrientation orientation,IShapeProperties** props)
{
   CComPtr<ISidewalkBarrier> barrier;
   if ( orientation == pgsTypes::tboLeft )
      m_Bridge->get_LeftBarrier(&barrier);
   else
      m_Bridge->get_RightBarrier(&barrier);

   if ( barrier == NULL )
   {
      *props = 0;
      return;
   }

   CComPtr<IShape> shape;
   barrier->get_Shape(&shape);

   shape->get_ShapeProperties(props);
}

CBridgeAgentImp::SectProp CBridgeAgentImp::GetSectionProperties(pgsTypes::Stage stage,const pgsPointOfInterest& poi)
{
   VALIDATE(BRIDGE);

   USES_CONVERSION;

   if ( StageCompare(stage,pgsTypes::BridgeSite2) < 0 )
   {
      // before BridgeSite2, the girder is non-composite
      // and the properties are the same for all stages
      // store everything for the casting yard stage
      stage = pgsTypes::CastingYard;
   }
   else
   {
      // for BridgeSite2 and 3, the girder is composite (if it has a composite deck)
      // store everything as BridgeSite 3
      stage = pgsTypes::BridgeSite3;
   }

   // Find properties and return... if not found, compute them now
   PoiStageKey key(stage,poi);
   SectPropContainer::iterator found = m_SectProps.find(key);
   if ( found != m_SectProps.end() )
   {
      return (*found).second;
   }

   //
   //   ... not found ... compute, store, return stored value
   //

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();
   Float64 distFromStartOfGirder = poi.GetDistFromStart();

   GET_IFACE(ILibrary,       pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth )
   {
      CComQIPtr<IPGSuperEffectiveFlangeWidthTool> eff_tool(m_EffFlangeWidthTool);
      ATLASSERT(eff_tool);
      eff_tool->put_UseTributaryWidth(VARIANT_TRUE);
   }

   // use tool to create section
   CComBSTR bstrStage = (stage == pgsTypes::CastingYard ? GetStageName(pgsTypes::BridgeSite1) : GetStageName(stage));
   CComPtr<ISection> section;
   HRESULT hr = m_SectCutTool->CreateGirderSection(m_Bridge,span,gdr,distFromStartOfGirder,bstrStage,&section);
   ATLASSERT(SUCCEEDED(hr));

   // get mass properties
   CComPtr<IMassProperties> massprops;
   section->get_MassProperties(&massprops);

   // get elastic properties of section
   CComPtr<IElasticProperties> eprop;
   section->get_ElasticProperties(&eprop);

   // transform section properties
   CComPtr<IShapeProperties> shapeprops;
   Float64 Egdr = GetEcGdr(span,gdr);
   eprop->TransformProperties(Egdr,&shapeprops);

   SectProp props; 
   props.Section      = section;
   props.ElasticProps = eprop;
   props.ShapeProps   = shapeprops;
   props.MassProps    = massprops;

   Float64 Ag;
   shapeprops->get_Area(&Ag);
   LOG(_T("Stage = ") << stage << _T(" span = ") << span << _T(" gdr = ") << gdr << _T(" x = ") << ::ConvertFromSysUnits(poi.GetDistFromStart(),unitMeasure::Feet) << _T(" ft") << _T(" Ag = ") << ::ConvertFromSysUnits(Ag,unitMeasure::Inch2) << _T(" in2") << _T(" Eg = ") << ::ConvertFromSysUnits(Egdr,unitMeasure::KSI) << _T(" KSI"));

#pragma Reminder("UPDATE: Assuming section is a Composite section and beam is exactly the first piece")
   CComQIPtr<ICompositeSection> cmpsection(section);
   CComPtr<ICompositeSectionItem> csi;
   cmpsection->get_Item(0,&csi); // this should be the beam
   CComPtr<IShape> shape;
   csi->get_Shape(&shape);

   if ( StageCompare(stage,pgsTypes::BridgeSite2) < 0 )
   {
      // Non-composite girder
      Float64 Yt;
      shapeprops->get_Ytop(&Yt);

      props.YtopGirder = Yt;
      props.Qslab = 0;
      
      shape->get_Perimeter(&props.Perimeter);
   }
   else
   {
      props.Perimeter = 0;

      Float64 Yb;
      shapeprops->get_Ybottom(&Yb);

      // Q slab (from procedure in WBFL Sections library documentation)
      CComQIPtr<IXYPosition> position(shape);
      CComPtr<IPoint2d> top_center;
      position->get_LocatorPoint(lpTopCenter,&top_center);

      // Ytop Girder
      CComPtr<IShapeProperties> beamprops;
      shape->get_ShapeProperties(&beamprops);
      Float64 Ytbeam, Ybbeam;
      beamprops->get_Ytop(&Ytbeam);
      beamprops->get_Ybottom(&Ybbeam);
      Float64 hg = Ytbeam + Ybbeam;
      props.YtopGirder = hg - Yb;

      // Create clipping line through beam/slab interface
      CComPtr<ILine2d> line;
      line.CoCreateInstance(CLSID_Line2d);
      CComPtr<IPoint2d> p;
      CComPtr<IVector2d> v;
      line->GetExplicit(&p,&v);
      Float64 x,y;
      top_center->Location(&x,&y);
      p->Move(x,y);
      v->put_Direction(M_PI); // direct line to the left so the beam is clipped out
      v->put_Magnitude(1.0);
      line->SetExplicit(p,v);

      CComPtr<ISection> slabsection;
      section->ClipWithLine(line,&slabsection);

      CComPtr<IElasticProperties> epslab;
      slabsection->get_ElasticProperties(&epslab);

      Float64 EAslab;
      epslab->get_EA(&EAslab);
      Float64 Aslab = EAslab / Egdr; // Transformed to equivalent girder material

      CComPtr<IPoint2d> cg_section;
      shapeprops->get_Centroid(&cg_section);

      CComPtr<IPoint2d> cg_slab;
      epslab->get_Centroid(&cg_slab);

      Float64 Yg, Ys;
      cg_slab->get_Y(&Ys);
      cg_section->get_Y(&Yg);

      Float64 Qslab = Aslab*(Ys - Yg);
      props.Qslab = Qslab;

      // Area on bottom half of composite section for LRFD Fig 5.8.3.4.2-3
      Float64 h = GetHalfElevation(poi);

      CComPtr<IPoint2d> bottom_center;
      position->get_LocatorPoint(lpBottomCenter,&bottom_center);
      bottom_center->get_Y(&y);
      h += y;

      CComPtr<IPoint2d> p1, p2;
      p1.CoCreateInstance(CLSID_Point2d);
      p2.CoCreateInstance(CLSID_Point2d);

      p1->Move(-99999,h);
      p2->Move( 99999,h);

      line->ThroughPoints(p1,p2);

      CComPtr<ISection> clipped_section;
      props.Section->ClipWithLine(line,&clipped_section);

      CComPtr<IElasticProperties> bheprops;
      clipped_section->get_ElasticProperties(&bheprops);

      CComPtr<IShapeProperties> bhsprops;
      bheprops->TransformProperties(GetEcGdr(span,gdr),&bhsprops);

      Float64 area;
      bhsprops->get_Area(&area);
      props.AcBottomHalf = area;

      // Area on top half of composite section for LRFD Fig. 5.8.3.4.2-3
      bhsprops.Release();
      props.ElasticProps->TransformProperties(GetEcGdr(span,gdr),&bhsprops);
      bhsprops->get_Area(&area);
      props.AcTopHalf = area - props.AcBottomHalf;
   }

   // don't store if not a real POI
   if ( poi.GetID() == INVALID_ID )
      return props;

   // Store the properties
   std::pair<SectPropContainer::iterator,bool> result;
   result = m_SectProps.insert(std::make_pair(key,props));
   ATLASSERT(result.second == true);
   found = result.first; 
   ATLASSERT(found == m_SectProps.find(key));
   ATLASSERT(found != m_SectProps.end());
   return (*found).second;
}

Float64 CBridgeAgentImp::GetHalfElevation(const pgsPointOfInterest& poi)
{
   Float64 deck_thickness = GetStructuralSlabDepth(poi);
   Float64 girder_depth = GetHeight(poi);
   Float64 td2 = (deck_thickness + girder_depth)/2.;
   return td2;    // line cut is at 1/2 of composite section depth
}

HRESULT CBridgeAgentImp::GetSlabOverhangs(Float64 distFromStartOfBridge,Float64* pLeft,Float64* pRight)
{
   Float64 station = GetPierStation(0);
   station += distFromStartOfBridge;
   m_BridgeGeometryTool->DeckOverhang(m_Bridge,station,NULL,qcbLeft,pLeft);
   m_BridgeGeometryTool->DeckOverhang(m_Bridge,station,NULL,qcbRight,pRight);

   return S_OK;
}

HRESULT CBridgeAgentImp::GetGirderSection(const pgsPointOfInterest& poi,IGirderSection** gdrSection)
{
   CComPtr<ISuperstructureMember> ssmbr;
   HRESULT hr = GetSuperstructureMember(poi.GetSpan(),poi.GetGirder(),&ssmbr);
   ATLASSERT(SUCCEEDED(hr));

#if defined _DEBUG
   CollectionIndexType nSegments;
   ssmbr->get_SegmentCount(&nSegments);
   ATLASSERT(nSegments == 1); // this is the assumption
#endif

   CComPtr<ISegment> segment;
   ssmbr->get_Segment(0,&segment);

   CComPtr<IShape> girder_shape;
   segment->get_Shape(poi.GetDistFromStart(),&girder_shape);

   CComQIPtr<IGirderSection> girder_section(girder_shape);

   ASSERT(girder_section != NULL);

   (*gdrSection) = girder_section;
   (*gdrSection)->AddRef();

   return S_OK;
}

HRESULT CBridgeAgentImp::GetSuperstructureMember(SpanIndexType spanIdx,GirderIndexType gdrIdx,ISuperstructureMember* *ssmbr)
{
   VALIDATE(BRIDGE);
   return ::GetSuperstructureMember(m_Bridge,spanIdx,gdrIdx,ssmbr);
}

HRESULT CBridgeAgentImp::GetGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx,IPrecastGirder** girder)
{
   VALIDATE(BRIDGE);
   return ::GetGirder(m_Bridge,spanIdx,gdrIdx,girder);
}

HRESULT CBridgeAgentImp::GetConnections(SpanIndexType spanIdx,GirderIndexType gdrIdx,IConnection** startConnection,IConnection** endConnection)
{
   CComPtr<ISuperstructureMember> ssmbr;
   HRESULT hr = GetSuperstructureMember(spanIdx,gdrIdx,&ssmbr);
   ATLASSERT(SUCCEEDED(hr));

   CComQIPtr<IItemData> item_data(ssmbr);

   CComPtr<IUnknown> punk_start, punk_end;
   hr = item_data->GetItemData(CComBSTR(STARTCONNECTION),&punk_start);
   ATLASSERT(SUCCEEDED(hr));
   hr = item_data->GetItemData(CComBSTR(ENDCONNECTION),  &punk_end);
   ATLASSERT(SUCCEEDED(hr));

   hr = punk_start->QueryInterface(startConnection);
   ATLASSERT(SUCCEEDED(hr));
   hr = punk_end->QueryInterface(endConnection);
   ATLASSERT(SUCCEEDED(hr));

   return S_OK;
}

Float64 CBridgeAgentImp::GetGrossSlabDepth()
{
   // Private method for getting gross slab depth
   // Gross slab depth is needed during validation of bridge model,
   // but public method causes validation. This causes a recusion problem
#pragma Reminder("REVIEW: Assumes constant thickness deck")
   CComPtr<IBridgeDeck> deck;
   m_Bridge->get_Deck(&deck);

   CComQIPtr<ICastSlab>    cip(deck);
   CComQIPtr<IPrecastSlab> sip(deck);
   CComQIPtr<IOverlaySlab> overlay(deck);

   Float64 deck_thickness;
   if ( cip != NULL )
   {
      cip->get_GrossDepth(&deck_thickness);
   }
   else if ( sip != NULL )
   {
      Float64 cast_depth;
      sip->get_CastDepth(&cast_depth);

      Float64 panel_depth;
      sip->get_PanelDepth(&panel_depth);

      deck_thickness = cast_depth + panel_depth;
   }
   else if ( overlay != NULL )
   {
      overlay->get_GrossDepth(&deck_thickness);
   }
   else
   {
      ATLASSERT(false); // bad deck type
      deck_thickness = 0;
   }

   return deck_thickness;
}

Float64 CBridgeAgentImp::GetCastDepth()
{
#pragma Reminder("REVIEW: Assumes constant thickness deck")
   CComPtr<IBridgeDeck> deck;
   m_Bridge->get_Deck(&deck);

   CComQIPtr<ICastSlab>    cip(deck);
   CComQIPtr<IPrecastSlab> sip(deck);
   CComQIPtr<IOverlaySlab> overlay(deck);

   Float64 cast_depth;
   if ( cip != NULL )
   {
      cip->get_GrossDepth(&cast_depth);
   }
   else if ( sip != NULL )
   {
      sip->get_CastDepth(&cast_depth);
   }
   else if ( overlay != NULL )
   {
      overlay->get_GrossDepth(&cast_depth);
   }
   else
   {
      ATLASSERT(false); // bad deck type
      cast_depth = 0;
   }

   return cast_depth;
}

Float64 CBridgeAgentImp::GetPanelDepth()
{
   // Private method for getting gross slab depth
   // Gross slab depth is needed during validation of bridge model,
   // but public method causes validation. This causes a recusion problem
#pragma Reminder("REVIEW: Assumes constant thickness deck")
   CComPtr<IBridgeDeck> deck;
   m_Bridge->get_Deck(&deck);

   CComQIPtr<ICastSlab>    cip(deck);
   CComQIPtr<IPrecastSlab> sip(deck);
   CComQIPtr<IOverlaySlab> overlay(deck);

   Float64 panel_depth;
   if ( cip != NULL )
   {
      panel_depth = 0;
   }
   else if ( sip != NULL )
   {
      sip->get_PanelDepth(&panel_depth);
   }
   else if ( overlay != NULL )
   {
      panel_depth = 0;
   }
   else
   {
      ATLASSERT(false); // bad deck type
      panel_depth = 0;
   }

   return panel_depth;
}


Float64 CBridgeAgentImp::GetSlabOverhangDepth()
{
   CComPtr<IBridgeDeck> deck;
   m_Bridge->get_Deck(&deck);

   CComQIPtr<ICastSlab>    cip(deck);
   CComQIPtr<IPrecastSlab> sip(deck);
   CComQIPtr<IOverlaySlab> overlay(deck);

   Float64 overhang_depth;
   if ( cip != NULL )
   {
      cip->get_OverhangDepth(&overhang_depth);
   }
   else if ( sip != NULL )
   {
      sip->get_CastDepth(&overhang_depth);
   }
   else if ( overlay != NULL )
   {
      overlay->get_GrossDepth(&overhang_depth);
   }
   else
   {
      ATLASSERT(false); // bad deck type
      // Not implemented yet... need to return the top flange thickness of exterior girder if there is not slab
      overhang_depth = 0;
   }

   return overhang_depth;
}

void CBridgeAgentImp::LayoutGirderRebar(SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE(ILongitudinalRebar,pLongRebar);
   CLongitudinalRebarData lrd = pLongRebar->GetLongitudinalRebarData(span,gdr);

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   CComPtr<IRebarLayout> rebar_layout;
   girder->get_RebarLayout(&rebar_layout);

   Float64 gdr_length = GetGirderLength(span,gdr);
   pgsPointOfInterest startPoi(span,gdr,0.00);
   pgsPointOfInterest endPoi(span,gdr,gdr_length);

   CComPtr<IShape> startShape, endShape;
   GetGirderShapeDirect(startPoi,&startShape);
   GetGirderShapeDirect(endPoi,  &endShape);

   CComPtr<IRect2d> bbStart, bbEnd;
   startShape->get_BoundingBox(&bbStart);
   endShape->get_BoundingBox(&bbEnd);

   CComPtr<IPoint2d> topCenter, bottomCenter;
   bbStart->get_TopCenter(&topCenter);
   bbStart->get_BottomCenter(&bottomCenter);

   Float64 startTopY, startBottomY;
   topCenter->get_Y(&startTopY);
   bottomCenter->get_Y(&startBottomY);

   topCenter.Release();
   bottomCenter.Release();

   bbEnd->get_TopCenter(&topCenter);
   bbEnd->get_BottomCenter(&bottomCenter);

   Float64 endTopY, endBottomY;
   topCenter->get_Y(&endTopY);
   bottomCenter->get_Y(&endBottomY);

   const std::vector<CLongitudinalRebarData::RebarRow>& rebar_rows = lrd.RebarRows;

   std::_tstring strRebarName = pLongRebar->GetLongitudinalRebarMaterial(span,gdr);
   matRebar::Grade grade;
   matRebar::Type type;
   pLongRebar->GetLongitudinalRebarMaterial(span,gdr,type,grade);
   MaterialSpec matSpec = (type == matRebar::A615 ? msA615 : msA706);
   RebarGrade matGrade;
   switch(grade)
   {
   case matRebar::Grade40: matGrade = Grade40; break;
   case matRebar::Grade60: matGrade = Grade60; break;
   case matRebar::Grade75: matGrade = Grade75; break;
   case matRebar::Grade80: matGrade = Grade80; break;
   default:
      ATLASSERT(false);
   }

   CComPtr<IRebarFactory> rebar_factory;
   rebar_factory.CoCreateInstance(CLSID_RebarFactory);

   if ( 0 < rebar_rows.size() )
   {
//#pragma Reminder("The unitserver below leaks memory because it is referenced to strongly by all of its unit types, but has a weak ref to them. Not easy to resolve")
      CComPtr<IUnitServer> unitServer;
      unitServer.CoCreateInstance(CLSID_UnitServer);
      HRESULT hr = ConfigureUnitServer(unitServer);
      ATLASSERT(SUCCEEDED(hr));

      CComPtr<IUnitConvert> unit_convert;
      unitServer->get_UnitConvert(&unit_convert);


      // The library entries are built on the assumption that rebars run full length of the girder
      CComPtr<IFlexRebarLayoutItem> flex_layout_item;
      flex_layout_item.CoCreateInstance(CLSID_FlexRebarLayoutItem);

      flex_layout_item->put_Position(lpCenter);
      flex_layout_item->put_LengthFactor(1.0);
      flex_layout_item->putref_Girder(girder);

      CComQIPtr<IRebarLayoutItem> rebar_layout_item(flex_layout_item);

      for ( RowIndexType idx = 0; idx < rebar_rows.size(); idx++ )
      {
         const CLongitudinalRebarData::RebarRow& info = rebar_rows[idx];

         if ( 0 < info.BarSize && 0 < info.NumberOfBars )
         {
            BarSize bar_size = GetBarSize(info.BarSize);
            CComPtr<IRebar> rebar;
            rebar_factory->CreateRebar(matSpec,matGrade,bar_size,unit_convert,&rebar);

            Float64 db;
            rebar->get_NominalDiameter(&db);

            CComPtr<IRebarRowPattern> row_pattern;
            row_pattern.CoCreateInstance(CLSID_RebarRowPattern);

            Float64 yStart, yEnd;
            if (info.Face == pgsTypes::GirderBottom)
            {
               yStart = info.Cover + db/2;
               yEnd   = yStart;
            }
            else
            {
               yStart = startTopY - info.Cover - db/2 - startBottomY;
               yEnd   = endTopY   - info.Cover - db/2 - endBottomY;
            }

            CComPtr<IPoint2d> startAnchor;
            startAnchor.CoCreateInstance(CLSID_Point2d);
            startAnchor->Move(0,yStart);

            CComPtr<IPoint2d> endAnchor;
            endAnchor.CoCreateInstance(CLSID_Point2d);
            endAnchor->Move(0,yEnd);

            row_pattern->putref_Rebar(rebar);
            row_pattern->put_AnchorPoint(etStart,startAnchor);
            row_pattern->put_AnchorPoint(etEnd,  endAnchor);
            row_pattern->put_Count( info.NumberOfBars );
            row_pattern->put_Spacing( info.BarSpacing );
            row_pattern->put_Orientation( rroHCenter );

            rebar_layout_item->AddRebarPattern(row_pattern);
         }
      }

      rebar_layout->Add(rebar_layout_item);
   }
}

void CBridgeAgentImp::CheckBridge()
{
   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(ILibrary,pLib);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // make sure all girders have positive lengths
   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();
   for( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      GirderIndexType nGirders = pBridgeDesc->GetSpan(spanIdx)->GetGirderCount();
      for( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CComPtr<IPrecastGirder> girder;
         this->GetGirder(spanIdx, gdrIdx, &girder);

         Float64 sLength;
         girder->get_SpanLength(&sLength);

         if ( sLength <= 0 )
         {
            std::_tostringstream os;
            os << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx)
               << _T(" does not have a positive length.") << std::endl;

            pgsBridgeDescriptionStatusItem* pStatusItem = 
               new pgsBridgeDescriptionStatusItem(m_StatusGroupID,m_scidBridgeDescriptionError,0,os.str().c_str());

            pStatusCenter->Add(pStatusItem);

            os << _T("See Status Center for Details");
            THROW_UNWIND(os.str().c_str(),XREASON_NEGATIVE_GIRDER_LENGTH);
         }

         // Check location of temporary strands... usually in the top half of the girder
         const GirderLibraryEntry* pGirderEntry = pBridgeDesc->GetSpan(spanIdx)->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);
         Float64 h_start = pGirderEntry->GetBeamHeight(pgsTypes::metStart);
         Float64 h_end   = pGirderEntry->GetBeamHeight(pgsTypes::metEnd);

         CComPtr<IStrandGrid> strand_grid;
         girder->get_TemporaryStrandGrid(etStart,&strand_grid);
         GridIndexType nPoints;
         strand_grid->get_GridPointCount(&nPoints);
         for ( GridIndexType idx = 0; idx < nPoints; idx++ )
         {
            CComPtr<IPoint2d> point;
            strand_grid->get_GridPoint(idx,&point);

            Float64 Y;
            point->get_Y(&Y);

            if ( Y < h_start/2 || Y < h_end/2 )
            {
               CString strMsg;
               strMsg.Format(_T("Span %d Girder %s, Temporary strands are not in the top half of the girder"),LABEL_SPAN(spanIdx),LABEL_GIRDER(gdrIdx));
               pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID,m_scidInformationalWarning,strMsg);
               pStatusCenter->Add(pStatusItem);
            }
         }
      }
   }

#if defined _DEBUG
   // Dumps the cogo model to a file so it can be viewed/debugged
   CComPtr<IStructuredSave2> save;
   save.CoCreateInstance(CLSID_StructuredSave2);
   save->Open(CComBSTR(_T("CogoModel.cogo")));

   save->BeginUnit(CComBSTR(_T("Cogo")),1.0);

   CComPtr<ICogoModel> cogo_model;
   m_Bridge->get_CogoModel(&cogo_model);
   CComQIPtr<IStructuredStorage2> ss(cogo_model);
   ss->Save(save);

   save->EndUnit();
   save->Close();
#endif _DEBUG
}

SpanIndexType CBridgeAgentImp::GetSpanCount_Private()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   return pBridgeDesc->GetSpanCount();
}

bool CBridgeAgentImp::ComputeNumPermanentStrands(StrandIndexType totalPermanent,SpanIndexType span,GirderIndexType gdr,StrandIndexType* numStraight,StrandIndexType* numHarped)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   CContinuousStrandFiller* pFiller = GetContinuousStrandFiller(span,gdr);

   HRESULT hr = pFiller->ComputeNumPermanentStrands(girder, totalPermanent, numStraight, numHarped);
   ATLASSERT(SUCCEEDED(hr));

   return hr==S_OK;
}

bool CBridgeAgentImp::ComputeNumPermanentStrands(StrandIndexType totalPermanent,LPCTSTR strGirderName, StrandIndexType* numStraight, StrandIndexType* numHarped)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGirderEntry = pLib->GetGirderEntry( strGirderName );
   return pGirderEntry->GetPermStrandDistribution(totalPermanent, numStraight, numHarped);
}

StrandIndexType CBridgeAgentImp::GetMaxNumPermanentStrands(SpanIndexType span,GirderIndexType gdr)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   CContinuousStrandFiller* pFiller = GetContinuousStrandFiller(span,gdr);

   StrandIndexType maxNum;
   HRESULT hr = pFiller->GetMaxNumPermanentStrands(girder, &maxNum);
   ATLASSERT(SUCCEEDED(hr));

   return maxNum;
}

StrandIndexType CBridgeAgentImp::GetMaxNumPermanentStrands(LPCTSTR strGirderName)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGirderEntry = pLib->GetGirderEntry( strGirderName );
   std::vector<GirderLibraryEntry::PermanentStrandType> permStrands = pGirderEntry->GetPermanentStrands();
   return permStrands.size()-1;
}

StrandIndexType CBridgeAgentImp::GetPreviousNumPermanentStrands(SpanIndexType span,GirderIndexType gdr,StrandIndexType curNum)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   CContinuousStrandFiller* pFiller = GetContinuousStrandFiller(span,gdr);

   StrandIndexType nextNum;
   HRESULT hr = pFiller->GetPreviousNumberOfPermanentStrands(girder, curNum, &nextNum);
   ATLASSERT(SUCCEEDED(hr));

   return nextNum;
}

StrandIndexType CBridgeAgentImp::GetPreviousNumPermanentStrands(LPCTSTR strGirderName,StrandIndexType curNum)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   CStrandFiller strandFiller;
   strandFiller.Init(pGdrEntry);

   StrandIndexType prevNum;
   strandFiller.GetPreviousNumberOfPermanentStrands(NULL,curNum,&prevNum);

   return prevNum;
}

StrandIndexType CBridgeAgentImp::GetNextNumPermanentStrands(SpanIndexType span,GirderIndexType gdr,StrandIndexType curNum)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   CContinuousStrandFiller* pFiller = GetContinuousStrandFiller(span,gdr);

   StrandIndexType nextNum;
   HRESULT hr = pFiller->GetNextNumberOfPermanentStrands(girder, curNum, &nextNum);
   ATLASSERT(SUCCEEDED(hr));

   return nextNum;
}

StrandIndexType CBridgeAgentImp::GetNextNumPermanentStrands(LPCTSTR strGirderName,StrandIndexType curNum)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   CStrandFiller strandFiller;
   strandFiller.Init(pGdrEntry);

   StrandIndexType nextNum;
   strandFiller.GetNextNumberOfPermanentStrands(NULL,curNum,&nextNum);

   return nextNum;
}

bool CBridgeAgentImp::ComputePermanentStrandIndices(LPCTSTR strGirderName,const PRESTRESSCONFIG& rconfig, pgsTypes::StrandType strType, IIndexArray** permIndices)
{
   CComPtr<IIndexArray> permStrands;  // array index = permanent strand for each strand of type
   permStrands.CoCreateInstance(CLSID_IndexArray);
   ATLASSERT(permStrands);

   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   const ConfigStrandFillVector& rStraightFillVec( rconfig.GetStrandFill(pgsTypes::Straight) );
   const ConfigStrandFillVector& rHarpedFillVec(   rconfig.GetStrandFill(pgsTypes::Harped) );

   GridIndexType maxStraightGrid = pGdrEntry->GetNumStraightStrandCoordinates();
   GridIndexType maxHarpedGrid = pGdrEntry->GetNumHarpedStrandCoordinates();
   ATLASSERT(maxStraightGrid == rStraightFillVec.size()); // this function won't play well without this
   ATLASSERT(maxHarpedGrid == rHarpedFillVec.size());

   GridIndexType maxPermGrid = pGdrEntry->GetPermanentStrandGridSize();
   // Loop over all available permanent strands and add index for strType if it's filled
   StrandIndexType permIdc = 0;
   for (GridIndexType idxPermGrid=0; idxPermGrid<maxPermGrid; idxPermGrid++)
   {
      GridIndexType localGridIdx;
      GirderLibraryEntry::psStrandType type;
      pGdrEntry->GetGridPositionFromPermStrandGrid(idxPermGrid, &type, &localGridIdx);

      if (type==pgsTypes::Straight)
      {
         // If filled, use count from fill vec, otherwise use x coordinate
         StrandIndexType strCnt;
         bool isFilled = rStraightFillVec[localGridIdx] > 0;
         if (isFilled)
         {
            strCnt = rStraightFillVec[localGridIdx];
         }
         else
         {
            Float64 start_x, start_y, end_x, end_y;
            bool can_debond;
            pGdrEntry->GetStraightStrandCoordinates(localGridIdx, &start_x, &start_y, &end_x, &end_y, &can_debond);
            strCnt = (start_x > 0.0 || end_x > 0.0) ? 2 : 1;
         }

         permIdc += strCnt;

         // if strand is filled, add its permanent index to the collection
         if (isFilled && strType==pgsTypes::Straight)
         {
            if (strCnt==1)
            {
               permStrands->Add(permIdc-1);
            }
            else
            {
               permStrands->Add(permIdc-2);
               permStrands->Add(permIdc-1);
            }
         }
      }
      else
      {
         ATLASSERT(type==pgsTypes::Harped);
         // If filled, use count from fill vec, otherwise use x coordinate
         StrandIndexType strCnt;
         bool isFilled = rHarpedFillVec[localGridIdx] > 0;
         if (isFilled)
         {
            strCnt = rHarpedFillVec[localGridIdx];
         }
         else
         {
            Float64 startx, starty, hpx, hpy, endx, endy;
            pGdrEntry->GetHarpedStrandCoordinates(localGridIdx, &startx, &starty, &hpx, &hpy, &endx, &endy);
            strCnt = (startx > 0.0 || hpx > 0.0 || endx > 0.0) ? 2 : 1;
         }

         permIdc += strCnt;

         // if strand is filled, add its permanent index to the collection
         if (isFilled && strType==pgsTypes::Harped)
         {
            permStrands->Add(permIdc-1);

            if (strCnt==2)
            {
               permStrands->Add(permIdc-2);
            }
         }
      }
   }

   permStrands.CopyTo(permIndices);
   return true;
}

bool CBridgeAgentImp::IsValidNumStrands(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType type,StrandIndexType curNum)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   CContinuousStrandFiller* pFiller = GetContinuousStrandFiller(span,gdr);

   VARIANT_BOOL bIsValid;
   HRESULT hr = E_FAIL;
   switch( type )
   {
   case pgsTypes::Straight:
      hr = pFiller->IsValidNumStraightStrands(girder, curNum, &bIsValid);
      break;

   case pgsTypes::Harped:
      hr = pFiller->IsValidNumHarpedStrands(girder, curNum, &bIsValid);
      break;

   case pgsTypes::Temporary:
      hr = pFiller->IsValidNumTemporaryStrands(girder, curNum, &bIsValid);
      break;
   }
   ATLASSERT(SUCCEEDED(hr));

   return bIsValid == VARIANT_TRUE ? true : false;
}

bool CBridgeAgentImp::IsValidNumStrands(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType curNum)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   CStrandFiller strandFiller;
   strandFiller.Init(pGdrEntry);

   VARIANT_BOOL isvalid(VARIANT_FALSE);
   HRESULT hr(E_FAIL);
   switch( type )
   {
   case pgsTypes::Straight:
      {
         CComPtr<IStrandGrid> startGrid, endGrid;
         startGrid.CoCreateInstance(CLSID_StrandGrid);
         endGrid.CoCreateInstance(CLSID_StrandGrid);
         pGdrEntry->ConfigureStraightStrandGrid(startGrid,endGrid);

         CComQIPtr<IStrandGridFiller> gridFiller(startGrid);

         hr = strandFiller.IsValidNumStraightStrands(gridFiller,curNum,&isvalid);
      }
      break;

   case pgsTypes::Harped:
      {
         CComPtr<IStrandGrid> startGrid, startHPGrid, endHPGrid, endGrid;
         startGrid.CoCreateInstance(CLSID_StrandGrid);
         startHPGrid.CoCreateInstance(CLSID_StrandGrid);
         endHPGrid.CoCreateInstance(CLSID_StrandGrid);
         endGrid.CoCreateInstance(CLSID_StrandGrid);
         pGdrEntry->ConfigureHarpedStrandGrids(startGrid,startHPGrid,endHPGrid,endGrid);

         CComQIPtr<IStrandGridFiller> startFiller(startGrid);
         CComQIPtr<IStrandGridFiller> endFiller(endGrid);

         hr = strandFiller.IsValidNumHarpedStrands(pGdrEntry->OddNumberOfHarpedStrands(),startFiller,endFiller,curNum,&isvalid);
      }
      break;

   case pgsTypes::Temporary:
      {
         CComPtr<IStrandGrid> startGrid, endGrid;
         startGrid.CoCreateInstance(CLSID_StrandGrid);
         endGrid.CoCreateInstance(CLSID_StrandGrid);
         pGdrEntry->ConfigureTemporaryStrandGrid(startGrid,endGrid);

         CComQIPtr<IStrandGridFiller> gridFiller(startGrid);

         hr = strandFiller.IsValidNumTemporaryStrands(gridFiller,curNum,&isvalid);
      }
      break;
   }
   ATLASSERT(SUCCEEDED(hr));

   return isvalid==VARIANT_TRUE;
}

StrandIndexType CBridgeAgentImp::GetNextNumStraightStrands(SpanIndexType span,GirderIndexType gdr,StrandIndexType curNum)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   CContinuousStrandFiller* pFiller = GetContinuousStrandFiller(span,gdr);

   StrandIndexType nextNum;
   HRESULT hr = pFiller->GetNextNumberOfStraightStrands(girder, curNum, &nextNum);
   ATLASSERT(SUCCEEDED(hr));

   return nextNum;
}

StrandIndexType CBridgeAgentImp::GetNextNumStraightStrands(LPCTSTR strGirderName,StrandIndexType curNum)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   CStrandFiller strandFiller;
   strandFiller.Init(pGdrEntry);

   CComPtr<IStrandGrid> startGrid, endGrid;
   startGrid.CoCreateInstance(CLSID_StrandGrid);
   endGrid.CoCreateInstance(CLSID_StrandGrid);
   pGdrEntry->ConfigureStraightStrandGrid(startGrid,endGrid);

   CComQIPtr<IStrandGridFiller> gridFiller(startGrid);

   StrandIndexType nextNum;
   strandFiller.GetNextNumberOfStraightStrands(gridFiller,curNum,&nextNum);

   return nextNum;
}

StrandIndexType CBridgeAgentImp::GetNextNumHarpedStrands(SpanIndexType span,GirderIndexType gdr,StrandIndexType curNum)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   CContinuousStrandFiller* pFiller = GetContinuousStrandFiller(span,gdr);

   StrandIndexType nextNum;
   HRESULT hr = pFiller->GetNextNumberOfHarpedStrands(girder, curNum, &nextNum);
   ATLASSERT(SUCCEEDED(hr));

   return nextNum;
}

StrandIndexType CBridgeAgentImp::GetNextNumHarpedStrands(LPCTSTR strGirderName,StrandIndexType curNum)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   CStrandFiller strandFiller;
   strandFiller.Init(pGdrEntry);


   CComPtr<IStrandGrid> startGrid, startHPGrid, endHPGrid, endGrid;
   startGrid.CoCreateInstance(CLSID_StrandGrid);
   startHPGrid.CoCreateInstance(CLSID_StrandGrid);
   endHPGrid.CoCreateInstance(CLSID_StrandGrid);
   endGrid.CoCreateInstance(CLSID_StrandGrid);
   pGdrEntry->ConfigureHarpedStrandGrids(startGrid,startHPGrid,endHPGrid,endGrid);

   CComQIPtr<IStrandGridFiller> gridFiller(startGrid);

   StrandIndexType nextNum;
   strandFiller.GetNextNumberOfHarpedStrands(pGdrEntry->OddNumberOfHarpedStrands(),gridFiller,curNum,&nextNum);

   return nextNum;
}

StrandIndexType CBridgeAgentImp::GetNextNumTempStrands(SpanIndexType span,GirderIndexType gdr,StrandIndexType curNum)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);
   
   CContinuousStrandFiller* pFiller = GetContinuousStrandFiller(span,gdr);

   StrandIndexType nextNum;
   HRESULT hr = pFiller->GetNextNumberOfTemporaryStrands(girder, curNum, &nextNum);
   ATLASSERT(SUCCEEDED(hr));

   return nextNum;
}

StrandIndexType CBridgeAgentImp::GetNextNumTempStrands(LPCTSTR strGirderName,StrandIndexType curNum)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   CStrandFiller strandFiller;
   strandFiller.Init(pGdrEntry);

   CComPtr<IStrandGrid> startGrid, endGrid;
   startGrid.CoCreateInstance(CLSID_StrandGrid);
   endGrid.CoCreateInstance(CLSID_StrandGrid);
   pGdrEntry->ConfigureTemporaryStrandGrid(startGrid,endGrid);

   CComQIPtr<IStrandGridFiller> gridFiller(startGrid);

   StrandIndexType nextNum;
   strandFiller.GetNextNumberOfTemporaryStrands(gridFiller,curNum,&nextNum);

   return nextNum;
}

StrandIndexType CBridgeAgentImp::GetPrevNumStraightStrands(SpanIndexType span,GirderIndexType gdr,StrandIndexType curNum)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   CContinuousStrandFiller* pFiller = GetContinuousStrandFiller(span,gdr);

   StrandIndexType nextNum;
   HRESULT hr = pFiller->GetPreviousNumberOfStraightStrands(girder, curNum, &nextNum);
   ATLASSERT(SUCCEEDED(hr));

   return nextNum;
}

StrandIndexType CBridgeAgentImp::GetPrevNumStraightStrands(LPCTSTR strGirderName,StrandIndexType curNum)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   CStrandFiller strandFiller;
   strandFiller.Init(pGdrEntry);

   CComPtr<IStrandGrid> startGrid, endGrid;
   startGrid.CoCreateInstance(CLSID_StrandGrid);
   endGrid.CoCreateInstance(CLSID_StrandGrid);
   pGdrEntry->ConfigureStraightStrandGrid(startGrid,endGrid);

   CComQIPtr<IStrandGridFiller> gridFiller(startGrid);

   StrandIndexType prevNum;
   strandFiller.GetPrevNumberOfStraightStrands(gridFiller,curNum,&prevNum);

   return prevNum;
}

StrandIndexType CBridgeAgentImp::GetPrevNumHarpedStrands(SpanIndexType span,GirderIndexType gdr,StrandIndexType curNum)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);
   
   CContinuousStrandFiller* pFiller = GetContinuousStrandFiller(span,gdr);

   StrandIndexType nextNum;
   HRESULT hr = pFiller->GetPreviousNumberOfHarpedStrands(girder, curNum, &nextNum);
   ATLASSERT(SUCCEEDED(hr));

   return nextNum;
}

StrandIndexType CBridgeAgentImp::GetPrevNumHarpedStrands(LPCTSTR strGirderName,StrandIndexType curNum)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   CStrandFiller strandFiller;
   strandFiller.Init(pGdrEntry);

   CComPtr<IStrandGrid> startGrid, startHPGrid, endHPGrid, endGrid;
   startGrid.CoCreateInstance(CLSID_StrandGrid);
   startHPGrid.CoCreateInstance(CLSID_StrandGrid);
   endHPGrid.CoCreateInstance(CLSID_StrandGrid);
   endGrid.CoCreateInstance(CLSID_StrandGrid);
   pGdrEntry->ConfigureHarpedStrandGrids(startGrid,startHPGrid,endHPGrid,endGrid);

   CComQIPtr<IStrandGridFiller> gridFiller(startGrid);

   StrandIndexType prevNum;
   strandFiller.GetPrevNumberOfHarpedStrands(pGdrEntry->OddNumberOfHarpedStrands(),gridFiller,curNum,&prevNum);

   return prevNum;
}

StrandIndexType CBridgeAgentImp::GetPrevNumTempStrands(SpanIndexType span,GirderIndexType gdr,StrandIndexType curNum)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);
   
   CContinuousStrandFiller* pFiller = GetContinuousStrandFiller(span,gdr);

   StrandIndexType nextNum;
   HRESULT hr = pFiller->GetPreviousNumberOfTemporaryStrands(girder, curNum, &nextNum);
   ATLASSERT(SUCCEEDED(hr));

   return nextNum;
}

StrandIndexType CBridgeAgentImp::GetPrevNumTempStrands(LPCTSTR strGirderName,StrandIndexType curNum)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   CStrandFiller strandFiller;
   strandFiller.Init(pGdrEntry);

   CComPtr<IStrandGrid> startGrid, endGrid;
   startGrid.CoCreateInstance(CLSID_StrandGrid);
   endGrid.CoCreateInstance(CLSID_StrandGrid);
   pGdrEntry->ConfigureTemporaryStrandGrid(startGrid,endGrid);

   CComQIPtr<IStrandGridFiller> gridFiller(startGrid);

   StrandIndexType prevNum;
   strandFiller.GetPrevNumberOfTemporaryStrands(gridFiller,curNum,&prevNum);

   return prevNum;
}

Float64 CBridgeAgentImp::GetCutLocation(const pgsPointOfInterest& poi)
{
   GET_IFACE(IBridge, pBridge);
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();
   Float64 dist_from_start_of_girder = poi.GetDistFromStart();

   Float64 start_brg_offset = pBridge->GetGirderStartBearingOffset(span,gdr);
   Float64 start_end_size   = pBridge->GetGirderStartConnectionLength(span,gdr);
   Float64 dist_from_start_pier = dist_from_start_of_girder - start_end_size + start_brg_offset;

   return dist_from_start_pier;
}

void CBridgeAgentImp::GetGirderShapeDirect(const pgsPointOfInterest& poi,IShape** ppShape)
{
   CComPtr<ISection> section;
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();
   Float64 distFromStartOfGirder = poi.GetDistFromStart();


   CComBSTR bstrStageName = GetStageName(pgsTypes::BridgeSite1);
   HRESULT hr = m_SectCutTool->CreateGirderSection(m_Bridge,span,gdr,distFromStartOfGirder,bstrStageName,&section);
   ATLASSERT(SUCCEEDED(hr));
   CComQIPtr<ICompositeSection> cmpsection(section);
   CComPtr<ICompositeSectionItem> csi;
   cmpsection->get_Item(0,&csi); // this should be the beam
   csi->get_Shape(ppShape);
}

BarSize CBridgeAgentImp::GetBarSize(matRebar::Size size)
{
   switch(size)
   {
   case matRebar::bs3:  return bs3;
   case matRebar::bs4:  return bs4;
   case matRebar::bs5:  return bs5;
   case matRebar::bs6:  return bs6;
   case matRebar::bs7:  return bs7;
   case matRebar::bs8:  return bs8;
   case matRebar::bs9:  return bs9;
   case matRebar::bs10: return bs10;
   case matRebar::bs11: return bs11;
   case matRebar::bs14: return bs14;
   case matRebar::bs18: return bs18;
   default:
      ATLASSERT(false); // should not get here
   }
   
   ATLASSERT(false); // should not get here
   return bs3;
}


Float64 CBridgeAgentImp::GetAsTensionSideOfGirder(const pgsPointOfInterest& poi,bool bDevAdjust,bool bTensionTop)
{
   CComPtr<IRebarSection> rebar_section;
   GetRebars(poi,&rebar_section);

   CComPtr<IEnumRebarSectionItem> enum_items;
   rebar_section->get__EnumRebarSectionItem(&enum_items);

   Float64 cl = GetHalfElevation(poi);

   Float64 As = 0;

   CComPtr<IRebarSectionItem> item;
   while ( enum_items->Next(1,&item,NULL) != S_FALSE )
   {
      CComPtr<IRebar> rebar;
      item->get_Rebar(&rebar);

      Float64 as;
      rebar->get_NominalArea(&as);

      CComPtr<IPoint2d> location;
      item->get_Location(&location);

      Float64 y;
      location->get_Y(&y);

      // include bar if tension on top and y is greater than centerline or if
      // tension is not top (tension is bottom) and y is less than centerline
      bool bIncludeBar = ( (bTensionTop && cl < y) || (!bTensionTop && y <= cl) ) ? true : false;

      if ( bIncludeBar )
      {
         Float64 fra = 1.0;
         if ( bDevAdjust )
         {
            fra = GetDevLengthFactor(poi.GetSpan(),poi.GetGirder(),item);
         }

         As += fra*as;
      }

      item.Release();
   }

   return As;
}

Float64 CBridgeAgentImp::GetApsTensionSide(const pgsPointOfInterest& poi,DevelopmentAdjustmentType devAdjust,bool bTensionTop)
{
   GDRCONFIG dummy_config;
   return GetApsTensionSide(poi,false,dummy_config,devAdjust,bTensionTop);
}

Float64 CBridgeAgentImp::GetApsTensionSide(const pgsPointOfInterest& poi, const GDRCONFIG& config,DevelopmentAdjustmentType devAdjust,bool bTensionTop)
{
   return GetApsTensionSide(poi,true,config,devAdjust,bTensionTop);
}

Float64 CBridgeAgentImp::GetApsTensionSide(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,DevelopmentAdjustmentType devAdjust,bool bTensionTop)
{
   VALIDATE( GIRDER );

   GET_IFACE(IPrestressForce,pPSForce);
   GET_IFACE(IStrandGeometry,pStrandGeom);

   Float64 cl = GetHalfElevation(poi);
   Float64 Aps = 0.0;

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   CComPtr<IPrecastGirder> girder;
   GetGirder(span,gdr,&girder);

   const matPsStrand* pStrand = GetStrand(span,gdr,pgsTypes::Straight);
   Float64 aps = pStrand->GetNominalArea();

   Float64 dist_from_start = poi.GetDistFromStart();
   Float64 gdr_length = GetGirderLength(span,gdr);

   // For approximate development length adjustment, take development length information at mid span and use for entire girder
   // adjusted for distance to ends of strands
   STRANDDEVLENGTHDETAILS dla_det;
   if(devAdjust==dlaApproximate)
   {
      std::vector<pgsPointOfInterest> vPoi;
      vPoi = GetPointsOfInterest(span,gdr,pgsTypes::BridgeSite3,POI_MIDSPAN);
      const pgsPointOfInterest& rpoi = vPoi.front();

      if ( bUseConfig )
         dla_det = pPSForce->GetDevLengthDetails(rpoi, config, false);
      else
         dla_det = pPSForce->GetDevLengthDetails(rpoi, false);
   }


   // Get straight strand locations
   CComPtr<IPoint2dCollection> strand_points;
   if( bUseConfig )
   {
      CIndexArrayWrapper strand_fill(config.PrestressConfig.GetStrandFill(pgsTypes::Straight));
      girder->get_StraightStrandPositionsEx(dist_from_start,&strand_fill,&strand_points);
   }
   else
   {
      girder->get_StraightStrandPositions(dist_from_start,&strand_points);
   }

   StrandIndexType Ns;
   strand_points->get_Count(&Ns);

   StrandIndexType strandIdx;
   for(strandIdx = 0; strandIdx < Ns; strandIdx++)
   {
      CComPtr<IPoint2d> strand_point;
      strand_points->get_Item(strandIdx, &strand_point);

      Float64 y;
      strand_point->get_Y(&y);

      // include bar if tension on top and y is greater than centerline or if
      // tension is not top (tension is bottom) and y is less than centerline
      bool bIncludeBar = ( (bTensionTop && cl < y) || (!bTensionTop && y <= cl) ) ? true : false;

      if ( bIncludeBar )
      {
         Float64 debond_factor;
         if(devAdjust==dlaNone)
         {
            debond_factor = 1.0;
         }
         else if(devAdjust==dlaApproximate)
         {
            // Use mid-span development length details to approximate debond factor
            // determine minimum bonded length from poi
            Float64 bond_start, bond_end;
            bool bDebonded;
            if (bUseConfig)
               bDebonded = pStrandGeom->IsStrandDebonded(span,gdr,strandIdx,pgsTypes::Straight,config.PrestressConfig,&bond_start,&bond_end);
            else
               bDebonded = pStrandGeom->IsStrandDebonded(span,gdr,strandIdx,pgsTypes::Straight,&bond_start,&bond_end);

            Float64 left_bonded_length, right_bonded_length;
            if ( bDebonded )
            {
               // measure bonded length
               left_bonded_length = dist_from_start - bond_start;
               right_bonded_length = bond_end - dist_from_start;
            }
            else
            {
               // no debonding, bond length is to ends of girder
               left_bonded_length  = dist_from_start;
               right_bonded_length = gdr_length - dist_from_start;
            }

            Float64 bonded_length = min(left_bonded_length, right_bonded_length);

            debond_factor = GetDevLengthAdjustment(bonded_length, dla_det.ld, dla_det.lt, dla_det.fps, dla_det.fpe);
         }
         else
         {
            // Full adjustment for development
            if ( bUseConfig )
               debond_factor = pPSForce->GetStrandBondFactor(poi,config,strandIdx,pgsTypes::Straight);
            else
               debond_factor = pPSForce->GetStrandBondFactor(poi,strandIdx,pgsTypes::Straight);
         }

         Aps += debond_factor*aps;
      }
   }

   // harped strands
   pStrand = GetStrand(span,gdr,pgsTypes::Harped);
   aps = pStrand->GetNominalArea();

   StrandIndexType Nh = (bUseConfig ? config.PrestressConfig.GetNStrands(pgsTypes::Harped) : pStrandGeom->GetNumStrands(span,gdr,pgsTypes::Harped));

   strand_points.Release();
   if(bUseConfig)
   {
      CIndexArrayWrapper strand_fill(config.PrestressConfig.GetStrandFill(pgsTypes::Harped));

      // save and restore precast girders shift values, before/after geting point locations
      Float64 t_end_shift, t_hp_shift;
      girder->get_HarpedStrandAdjustmentEnd(&t_end_shift);
      girder->get_HarpedStrandAdjustmentHP(&t_hp_shift);

      girder->put_HarpedStrandAdjustmentEnd(config.PrestressConfig.EndOffset);
      girder->put_HarpedStrandAdjustmentHP(config.PrestressConfig.HpOffset);

      girder->get_HarpedStrandPositionsEx(dist_from_start, &strand_fill, &strand_points);

      girder->put_HarpedStrandAdjustmentEnd(t_end_shift);
      girder->put_HarpedStrandAdjustmentHP(t_hp_shift);
   }
   else
   {
      girder->get_HarpedStrandPositions(dist_from_start,&strand_points);
   }

   StrandIndexType nstst; // reality test
   strand_points->get_Count(&nstst);
   ATLASSERT(nstst==Nh);

   for(strandIdx = 0; strandIdx < Nh; strandIdx++)
   {
      CComPtr<IPoint2d> strand_point;
      strand_points->get_Item(strandIdx, &strand_point);

      Float64 y;
      strand_point->get_Y(&y);

      // include bar if tension on top and y is greater than centerline or if
      // tension is not top (tension is bottom) and y is less than centerline
      bool bIncludeBar = ( (bTensionTop && cl < y) || (!bTensionTop && y <= cl) ) ? true : false;

      if ( bIncludeBar )
      {
         Float64 debond_factor = 1.;
         if ( bUseConfig )
            debond_factor = (devAdjust==dlaAccurate ? pPSForce->GetStrandBondFactor(poi,config,strandIdx,pgsTypes::Harped) : 1.0);
         else
            debond_factor = (devAdjust==dlaAccurate ? pPSForce->GetStrandBondFactor(poi,strandIdx,pgsTypes::Harped) : 1.0);

         Aps += debond_factor*aps;
      }
   }

   return Aps;
}

Float64 CBridgeAgentImp::GetAsDeckMats(const pgsPointOfInterest& poi,ILongRebarGeometry::DeckRebarType drt,bool bTopMat,bool bBottomMat)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();
   const CDeckRebarData& rebarData = pDeck->DeckRebarData;

   Float64 Weff = GetEffectiveFlangeWidth(poi);

   //
   // full length reinforcement
   //
   const matRebar* pBar;
   Float64 As_Top    = 0;
   Float64 As_Bottom = 0;

   // top mat
   if ( drt == ILongRebarGeometry::Primary || drt == ILongRebarGeometry::All )
   {
      if ( bTopMat )
      {
         if ( rebarData.TopRebarSize != matRebar::bsNone )
         {
            pBar = lrfdRebarPool::GetInstance()->GetRebar( rebarData.TopRebarType, rebarData.TopRebarGrade, rebarData.TopRebarSize );
            As_Top = pBar->GetNominalArea()/rebarData.TopSpacing;
         }

         As_Top += rebarData.TopLumpSum;
      }

      // bottom mat
      if ( bBottomMat )
      {
         if ( rebarData.BottomRebarSize != matRebar::bsNone )
         {
            pBar = lrfdRebarPool::GetInstance()->GetRebar( rebarData.BottomRebarType, rebarData.BottomRebarGrade, rebarData.BottomRebarSize );
            As_Bottom = pBar->GetNominalArea()/rebarData.BottomSpacing;
         }

         As_Bottom += rebarData.BottomLumpSum;
      }
   }

   if ( drt == ILongRebarGeometry::Supplemental || drt == ILongRebarGeometry::All )
   {
      //
      // negative moment reinforcement
      //
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

      Float64 start_end_size = GetGirderStartConnectionLength(span,gdr);
      Float64 end_end_size   = GetGirderEndConnectionLength(span,gdr);
      Float64 start_offset   = GetGirderStartBearingOffset(span,gdr);
      Float64 end_offset     = GetGirderEndBearingOffset(span,gdr);

      Float64 dist_from_cl_prev_pier = poi.GetDistFromStart() + start_offset - start_end_size;
      Float64 dist_to_cl_next_pier   = GetGirderLength(span,gdr) - poi.GetDistFromStart() + end_offset - end_end_size;

      PierIndexType prev_pier = span;
      PierIndexType next_pier = prev_pier + 1;

      std::vector<CDeckRebarData::NegMomentRebarData>::const_iterator iter;
      for ( iter = rebarData.NegMomentRebar.begin(); iter != rebarData.NegMomentRebar.end(); iter++ )
      {
         const CDeckRebarData::NegMomentRebarData& nmRebarData = *iter;

         if ( (bTopMat    && (nmRebarData.Mat == CDeckRebarData::TopMat)) ||
              (bBottomMat && (nmRebarData.Mat == CDeckRebarData::BottomMat)) )
         {
            if ( ( nmRebarData.PierIdx == prev_pier && IsLE(dist_from_cl_prev_pier,nmRebarData.RightCutoff) ) ||
                 ( nmRebarData.PierIdx == next_pier && IsLE(dist_to_cl_next_pier,  nmRebarData.LeftCutoff)  ) )
            {
               if ( nmRebarData.RebarSize != matRebar::bsNone )
               {
                  pBar = lrfdRebarPool::GetInstance()->GetRebar( nmRebarData.RebarType, nmRebarData.RebarGrade, nmRebarData.RebarSize);

                  if (nmRebarData.Mat == CDeckRebarData::TopMat)
                     As_Top += pBar->GetNominalArea()/nmRebarData.Spacing;
                  else
                     As_Bottom += pBar->GetNominalArea()/nmRebarData.Spacing;
               }

               if (nmRebarData.Mat == CDeckRebarData::TopMat)
                  As_Top += nmRebarData.LumpSum;
               else
                  As_Bottom += nmRebarData.LumpSum;
            }
         }
      }
   }

   Float64 As = (As_Top + As_Bottom)*Weff;

   return As;
}

void CBridgeAgentImp::GetShapeProperties(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 Ecgdr,IShapeProperties** ppShapeProps)
{
   SectProp props = GetSectionProperties(stage,poi);
   CComPtr<ISection> s;
   props.Section->Clone(&s);

#pragma Reminder("UPDATE: Assuming section is a Composite section and beam is exactly the first piece")
   CComQIPtr<ICompositeSection> cmpsection(s);
   CComPtr<ICompositeSectionItem> csi;
   cmpsection->get_Item(0,&csi); // this should be the beam

   //Update E for the girder
   csi->put_E(Ecgdr);

   CComPtr<IElasticProperties> eprops;
   s->get_ElasticProperties(&eprops);

   eprops->TransformProperties(Ecgdr,ppShapeProps);
}

void CBridgeAgentImp::GetSlabEdgePoint(Float64 station, IDirection* direction,DirectionType side,IPoint2d** point)
{
   VALIDATE(BRIDGE);

   HRESULT hr = m_BridgeGeometryTool->DeckEdgePoint(m_Bridge,station,direction,side,point);
   ATLASSERT(SUCCEEDED(hr));
}

void CBridgeAgentImp::GetSlabEdgePoint(Float64 station, IDirection* direction,DirectionType side,IPoint3d** point)
{
   VALIDATE(BRIDGE);

   CComPtr<IPoint2d> pnt2d;
   GetSlabEdgePoint(station,direction,side,&pnt2d);

   Float64 x,y;
   pnt2d->Location(&x,&y);

   Float64 normal_station,offset;
   GetStationAndOffset(pnt2d,&normal_station,&offset);

   Float64 elev = GetElevation(normal_station,offset);

   CComPtr<IPoint3d> pnt3d;
   pnt3d.CoCreateInstance(CLSID_Point3d);
   pnt3d->Move(x,y,elev);

   (*point) = pnt3d;
   (*point)->AddRef();
}

/// Strand filler-related functions
CContinuousStrandFiller* CBridgeAgentImp::GetContinuousStrandFiller(SpanIndexType span,GirderIndexType gdr)
{
   SpanGirderHashType hash = HashSpanGirder(span, gdr);
   StrandFillerCollection::iterator its = m_StrandFillerColl.find(hash);
   if (its != m_StrandFillerColl.end())
   {
      CStrandFiller& pcfill = its->second;
      CContinuousStrandFiller* pfiller = dynamic_cast<CContinuousStrandFiller*>(&(pcfill));
      return pfiller;
   }
   else
   {
      ATLASSERT(0); // This will go badly. Filler should have been created already
      return NULL;
   }
}

CDirectStrandFiller* CBridgeAgentImp::GetDirectStrandFiller(SpanIndexType span,GirderIndexType gdr)
{
   SpanGirderHashType hash = HashSpanGirder(span, gdr);
   StrandFillerCollection::iterator its = m_StrandFillerColl.find(hash);
   if (its != m_StrandFillerColl.end())
   {
      CStrandFiller& pcfill = its->second;
      CDirectStrandFiller* pfiller = dynamic_cast<CDirectStrandFiller*>(&(pcfill));
      return pfiller;
   }
   else
   {
      ATLASSERT(0); // This will go badly. Filler should have been created already
      return NULL;
   }
}

void CBridgeAgentImp::InitializeStrandFiller(const GirderLibraryEntry* pGirderEntry, SpanIndexType span, GirderIndexType gdr)
{
   SpanGirderHashType hash = HashSpanGirder(span, gdr);
   StrandFillerCollection::iterator its = m_StrandFillerColl.find(hash);
   if (its != m_StrandFillerColl.end())
   {
      its->second.Init(pGirderEntry);
   }
   else
   {
      CStrandFiller filler;
      filler.Init(pGirderEntry);
      std::pair<StrandFillerCollection::iterator, bool>  sit = m_StrandFillerColl.insert(std::make_pair(hash,filler));
      ATLASSERT(sit.second);
   }
}
