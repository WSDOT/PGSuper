///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include "BridgeGeometryModelBuilder.h"
#include "..\PGSuperException.h"
#include "DeckEdgeBuilder.h"

#include <PsgLib\LibraryManager.h>
#include <PsgLib\GirderLibraryEntry.h>
#include <PsgLib\ConnectionLibraryEntry.h>
#include <PsgLib\UnitServer.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\DeckDescription2.h>
#include <PgsExt\PierData2.h>
#include <PgsExt\SpanData2.h>
#include <PgsExt\PrecastSegmentData.h>
#include <PgsExt\TemporarySupportData.h>
#include <PgsExt\ClosureJointData.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <PsgLib\ShearData.h>
#include <PgsExt\LongitudinalRebarData.h>
#include <PgsExt\PointLoadData.h>
#include <PgsExt\DistributedLoadData.h>
#include <PgsExt\MomentLoadData.h>
#include <PgsExt\StatusItem.h>
#include <PgsExt\GirderLabel.h>

#include <MathEx.h>
#include <Math\Polynomial2d.h>
#include <Math\CompositeFunction2d.h>
#include <Math\LinFunc2d.h>
#include <System\Flags.h>
#include <Material\Material.h>
#include <GeomModel\ShapeUtils.h>

#include <IFace\DrawBridgeSettings.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\ShearCapacity.h>
#include <IFace\GirderHandling.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\StatusCenter.h>
#include <IFace\BeamFactory.h>
#include <IFace\EditByUI.h>
#include <IFace\MomentCapacity.h>
#include <IFace\AgeAdjustedMaterial.h>
#include <IFace\DocumentType.h>

#include <PgsExt\DesignConfigUtil.h>

#include <algorithm>
#include <cctype>

#include <afxext.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


DECLARE_LOGFILE;

#if defined _DEBUG
//#define CHECK_POI_CONVERSIONS // if this is defined the POI conversion methods are checked
#endif

CogoObjectID g_AlignmentID = 999;


#define CLEAR_ALL       0
#define COGO_MODEL      1
#define BRIDGE          2
#define GIRDER          3
#define LOADS           4

// Capacity problem validation states
#define CAPPROB_NC        0x0001
#define CAPPROB_CMP_INT   0x0002
#define CAPPROB_CMP_EXT   0x0004


#define VALIDATE(x) {if ( !Validate((x)) ) THROW_SHUTDOWN(_T("Fatal Error in Bridge Agent"),XREASON_AGENTVALIDATIONFAILURE,true);}
#define INVALIDATE(x) Invalidate((x))

#define STARTCONNECTION _T("Left Connection")  // must be this value for the PrecastGirder model
#define ENDCONNECTION   _T("Right Connection") // must be this value for the PrecastGirder model

#define LEFT_DECK_EDGE_LAYOUT_LINE_ID  -500
#define RIGHT_DECK_EDGE_LAYOUT_LINE_ID -501

#define DECK_REBAR_OFFSET 0.0015 // distance from the deck rebar cutoff point to the adjacent POI that is offset just a little so we capture jumps

class PoiNotInSpan
{
public:
   PoiNotInSpan(CBridgeAgentImp* pBridgeAgent,const CSpanKey& spanKey) :
      m_pBridgeAgent(pBridgeAgent),m_SpanKey(spanKey)
      {
         m_Lspan = spanKey.spanIndex == ALL_SPANS ? -1 : m_pBridgeAgent->GetSpanLength(spanKey);
      };

   bool operator()(const pgsPointOfInterest& poi) const
   {
      if ( !poi.HasAttribute(POI_SPAN) )
      {
         return true; // does not have a POI_SPAN attribute so we don't want to keep it
      }

      if ( m_SpanKey.spanIndex == ALL_SPANS )
      {
         // poi has a POI_SPAN attribute and we are matching all spans, so keep it
         return false;
      }

      CSpanKey spanKey;
      Float64 Xspan;
      m_pBridgeAgent->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

      if ( m_SpanKey.spanIndex == spanKey.spanIndex && IsEqual(Xspan,m_Lspan) )
      {
         // if poi is at the end of the span, the poi is in the target span
         // otherwise it is the first poi in the next span
         return poi.HasAttribute(POI_SPAN | POI_10L) ? false : true;
      }

      if ( poi.HasAttribute(POI_SPAN | POI_0L) && m_SpanKey.spanIndex == spanKey.spanIndex+1 )
      {
         // poi has start of span attribute and it was found to be in the previous span
         // if it is right at the end of the previous span, then it is actually the first poi
         // in the target span
         Float64 LprevSpan = m_pBridgeAgent->GetSpanLength(spanKey);
         if ( IsEqual(Xspan,LprevSpan) )
         {
            // poi is in the target span
            return false;
         }
      }

      return (spanKey.spanIndex != m_SpanKey.spanIndex ? true /* poi is NOT in the target span*/ : false /* poi is in the target span*/);
   };

private:
   CBridgeAgentImp* m_pBridgeAgent;
   CSpanKey m_SpanKey; // target span
   Float64 m_Lspan;
};

//function to translate project loads to bridge loads

IUserDefinedLoads::UserDefinedLoadCase Project2BridgeLoads(UserLoads::LoadCase plc)
{
   if (plc==UserLoads::DC)
   {
      return IUserDefinedLoads::userDC;
   }
   else if (plc==UserLoads::DW)
   {
      return IUserDefinedLoads::userDW;
   }
   else if (plc==UserLoads::LL_IM)
   {
      return IUserDefinedLoads::userLL_IM;
   }
   else
   {
      ATLASSERT(false);
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

static const Float64 gs_PoiTolerance = ::ConvertToSysUnits(1.0,unitMeasure::Millimeter);

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
   while(it != itend)
   {
      if(0 < *it)
      {
         return true;
      }

      it++;
   }

   return false;
}

StrandIndexType CountStrandsInConfigFillVec(const ConfigStrandFillVector& rHarpedFillArray)
{
   StrandIndexType cnt(0);

   ConfigStrandFillConstIterator it    = rHarpedFillArray.begin();
   ConfigStrandFillConstIterator itend = rHarpedFillArray.end();
   while(it != itend)
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
   { ATLASSERT(false); return E_FAIL;}
   virtual ULONG STDMETHODCALLTYPE AddRef( void)
   { return 1;}
   virtual ULONG STDMETHODCALLTYPE Release( void)
   {return 1;}

   // IIndexArray
	STDMETHOD(Find)(/*[in]*/CollectionIndexType value, /*[out,retval]*/CollectionIndexType* fndIndex)
   {
      ATLASSERT(false);
      return E_FAIL;
   }
	STDMETHOD(ReDim)(/*[in]*/CollectionIndexType size)
   {
      ATLASSERT(false);
      return E_FAIL;
   }

	STDMETHOD(Clone)(/*[out,retval]*/IIndexArray* *clone)
   {
      ATLASSERT(false);
      return E_FAIL;
   }
	STDMETHOD(get_Count)(/*[out, retval]*/ CollectionIndexType *pVal)
   {
	   *pVal = m_Values.size();
	   return S_OK;
   }
	STDMETHOD(Clear)()
   {
      ATLASSERT(false);
      return E_FAIL;
   }
	STDMETHOD(Reserve)(/*[in]*/CollectionIndexType count)
   {
      ATLASSERT(false);
      return E_FAIL;
   }
	STDMETHOD(Insert)(/*[in]*/CollectionIndexType relPosition, /*[in]*/CollectionIndexType item)
   {
      ATLASSERT(false);
      return E_FAIL;
   }
	STDMETHOD(Remove)(/*[in]*/CollectionIndexType relPosition)
   {
      ATLASSERT(false);
      return E_FAIL;
   }
	STDMETHOD(Add)(/*[in]*/CollectionIndexType item)
   {
      ATLASSERT(false);
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
         ATLASSERT(false);
         return E_INVALIDARG;
      }
	   return S_OK;
   }
	STDMETHOD(put_Item)(/*[in]*/CollectionIndexType relPosition, /*[in]*/ CollectionIndexType newVal)
   {
      ATLASSERT(false);
      return E_FAIL;
   }
	STDMETHOD(get__NewEnum)(struct IUnknown ** )
   {
      ATLASSERT(false);
      return E_FAIL;
   }
	STDMETHOD(get__EnumElements)(struct IEnumIndexArray ** )
   {
      ATLASSERT(false);
      return E_FAIL;
   }
   STDMETHOD(Assign)(/*[in]*/CollectionIndexType numElements, /*[in]*/CollectionIndexType value)
   {
      ATLASSERT(false);
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
// There is a special case during design where the inital adjustable strand type is straight, but the designer
// wants to do a harped design. For this case, we need to swap out the istrandmover and harping boundaries,
// so the precast girder can deal with vertical adjustment correctly. 
// This is definitly a hack and very inefficient, but seems the best of many hacks
// The class below does the swap in an exception safe manner
class CStrandMoverSwapper
{
public:
   CStrandMoverSwapper(const CSegmentKey& segmentKey, const PRESTRESSCONFIG& rconfig,
      CBridgeAgentImp* pBridgeAgent, IPrecastGirder* girder, IBridgeDescription* pIBridgeDesc):
   m_Girder(girder)
   {
   // Save in constructor and restore in destructor - stand mover and adjustment shift values
   if (rconfig.AdjustableStrandType == pgsTypes::asHarped)
   {
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
      const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);
      pgsTypes::AdjustableStrandType adjType = pSegment->Strands.GetAdjustableStrandType();
      if (adjType == pgsTypes::asStraight)
      {
         // mover will be reset later
         girder->get_StrandMover(&m_Strandmover);

         CComPtr<IStrandMover> temp_strandmover;
         pBridgeAgent->CreateStrandMover(pGirder->GetGirderName(), pgsTypes::asHarped, &temp_strandmover);

         girder->putref_StrandMover(temp_strandmover);
      }
   }

   // save and restore precast girders shift values, before/after geting point locations
   girder->get_HarpedStrandAdjustmentEnd(&m_EndShift);
   girder->get_HarpedStrandAdjustmentHP(&m_HpShift);

   girder->put_HarpedStrandAdjustmentEnd(rconfig.EndOffset);
   girder->put_HarpedStrandAdjustmentHP(rconfig.HpOffset);
}

~CStrandMoverSwapper()
{
   // Restore girder back to its original state
   if (m_Strandmover)
   {
      m_Girder->putref_StrandMover(m_Strandmover);
   }

   m_Girder->put_HarpedStrandAdjustmentEnd(m_EndShift);
   m_Girder->put_HarpedStrandAdjustmentHP(m_HpShift);
}

private:
   Float64 m_EndShift, m_HpShift;
   CComPtr<IStrandMover> m_Strandmover;
   IPrecastGirder* m_Girder;
};

PierType GetPierType(const CPierData2* pPierData)
{
   if ( pPierData->IsBoundaryPier() )
   {
      switch( pPierData->GetBoundaryConditionType() )
      {
      case pgsTypes::bctHinge:
      case pgsTypes::bctRoller:
         return ptExpansion;

      case pgsTypes::bctContinuousAfterDeck:
      case pgsTypes::bctContinuousBeforeDeck:
         return ptContinuous;

      case pgsTypes::bctIntegralAfterDeck:
      case pgsTypes::bctIntegralBeforeDeck:
         return ptIntegral;

      case pgsTypes::bctIntegralAfterDeckHingeBack:
      case pgsTypes::bctIntegralBeforeDeckHingeBack:
      case pgsTypes::bctIntegralAfterDeckHingeAhead:
      case pgsTypes::bctIntegralBeforeDeckHingeAhead:
         return ptIntegral;
      }

      ATLASSERT(false); // should never get here
      return ptIntegral;
   }
   else
   {
      switch ( pPierData->GetSegmentConnectionType() )
      {
      case pgsTypes::psctContinousClosureJoint:
      case pgsTypes::psctContinuousSegment:
         return ptContinuous;

      case pgsTypes::psctIntegralClosureJoint:
      case pgsTypes::psctIntegralSegment:
         return ptIntegral;
      }

      ATLASSERT(false); // should never get here
      return ptIntegral;
   }
}


/////////////////////////////////////////////////////////////////////////////
// CBridgeAgentImp
void CBridgeAgentImp::InvalidateSectionProperties(pgsTypes::SectionPropertyType sectPropType)
{
   SectPropContainer* pOldSectProps = m_pSectProps[sectPropType].release();
   m_pSectProps[sectPropType] = std::auto_ptr<SectPropContainer>(new SectPropContainer);

#if defined _USE_MULTITHREADING
   m_ThreadManager.CreateThread(CBridgeAgentImp::DeleteSectionProperties,(LPVOID)(pOldSectProps));
#else
   CBridgeAgentImp::DeleteSectionProperties((LPVOID)(pOldSectProps));
#endif
}

UINT CBridgeAgentImp::DeleteSectionProperties(LPVOID pParam)
{
   WATCH(_T("Begin: DeleteSectionProperties"));
   
   SectPropContainer* pSectionProperties = (SectPropContainer*)pParam;
   pSectionProperties->clear();
   delete pSectionProperties;

   WATCH(_T("End: DeleteSectionProperties"));

   return 0;
}

pgsPoiMgr* CBridgeAgentImp::CreatePoiManager()
{
   pgsPoiMgr* pPoiMgr = new pgsPoiMgr;
   pPoiMgr->SetTolerance(gs_PoiTolerance);
   return pPoiMgr;
}

void CBridgeAgentImp::InvalidatePointsOfInterest()
{
   pgsPoiMgr* pOldPoiMgr = m_pPoiMgr.release();
   m_pPoiMgr.reset(CreatePoiManager());

#if defined _USE_MULTITHREADING
   m_ThreadManager.CreateThread(CBridgeAgentImp::DeletePoiManager,(LPVOID)(pOldPoiMgr));
#else
   CBridgeAgentImp::DeletePoiManager((LPVOID)(pOldPoiMgr));
#endif
}

UINT CBridgeAgentImp::DeletePoiManager(LPVOID pParam)
{
   WATCH(_T("Begin: Delete Poi Manager"));
   pgsPoiMgr* pPoiMgr = (pgsPoiMgr*)pParam;
   delete pPoiMgr;
   WATCH(_T("End: Delete Poi Manager"));
   return 0;
}

pgsTypes::SectionPropertyType CBridgeAgentImp::GetSectionPropertiesType()
{
   pgsTypes::SectionPropertyType spType = (GetSectionPropertiesMode() == pgsTypes::spmGross ? pgsTypes::sptGross : pgsTypes::sptTransformed);
   return spType;
}

HRESULT CBridgeAgentImp::FinalConstruct()
{
   m_pPoiMgr.reset(CreatePoiManager());

   HRESULT hr = m_BridgeGeometryTool.CoCreateInstance(CLSID_BridgeGeometryTool);
   if ( FAILED(hr) )
   {
      return hr;
   }

   hr = m_CogoEngine.CoCreateInstance(CLSID_CogoEngine);
   if ( FAILED(hr) )
   {
      return hr;
   }

   for ( int i = 0; i < pgsTypes::sptSectionPropertyTypeCount; i++ )
   {
      pgsTypes::SectionPropertyType spType = (pgsTypes::SectionPropertyType)(i);
      m_pSectProps[spType] = std::auto_ptr<SectPropContainer>(new SectPropContainer);
   }

   return S_OK;
}

void CBridgeAgentImp::FinalRelease()
{
   m_BridgeGeometryTool.Release();
   m_CogoEngine.Release();
}

void CBridgeAgentImp::Invalidate( Uint16 level )
{
//   LOG(_T("Invalidating"));

   if ( level <= BRIDGE )
   {
//      LOG(_T("Invalidating Bridge Model"));

      m_CogoModel.Release();
      m_Bridge.Release();
      m_HorzCurveKeys.clear();
      m_VertCurveKeys.clear();


      // Must be valided at least past COGO_MODEL
      if ( COGO_MODEL < level )
      {
         level = COGO_MODEL;
      }

      m_ConcreteManager.Reset();

      m_ElevationAdjustmentEquations.clear();

      // If bridge is invalid, so are points of interest
      m_ValidatedPoi.clear();

      InvalidatePointsOfInterest();

      m_CriticalSectionState[0].clear();
      m_CriticalSectionState[1].clear();

      // there are usually a lot of a lot of cached section properties
      // don't make the user wait on deleting them. use a worker thread 
      for ( int i = 0; i < pgsTypes::sptSectionPropertyTypeCount; i++ )
      {
         pgsTypes::SectionPropertyType spType = (pgsTypes::SectionPropertyType)(i);
         InvalidateSectionProperties(spType);
      }

      InvalidateUserLoads();

      InvalidateDeckParameters();

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
   {
      m_Level -= 1;
   }

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

      if ( m_Level < level )
      {
         VALIDATE_TO_LEVEL( COGO_MODEL,   BuildCogoModel );
         VALIDATE_TO_LEVEL( BRIDGE,       BuildBridgeModel );
         VALIDATE_AND_CHECK_TO_LEVEL( GIRDER,       BuildGirder,    ValidateGirder );
      }

  
      if (GIRDER <= level)
      {
         ValidateUserLoads();
      }

//      LOG(_T("Validation complete - BridgeAgent at level ") << m_Level );
   }

      return m_Level;
}

void CBridgeAgentImp::ValidatePointsOfInterest(const CGirderKey& girderKey)
{
   // Bridge model, up to and including girders, must be valid before we can layout the poi
   VALIDATE(GIRDER);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // Create a collection of girder keys for the girders who's POI need to be validated
   std::vector<CGirderKey> girderKeyList;

   GroupIndexType firstGrpIdx;
   GroupIndexType lastGrpIdx;
   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      // validate all groups
      firstGrpIdx = 0;
      lastGrpIdx  = pBridgeDesc->GetGirderGroupCount()-1;
   }
   else
   {
      firstGrpIdx = girderKey.groupIndex;
      lastGrpIdx  = firstGrpIdx;
   }

   for ( GroupIndexType grpIdx = firstGrpIdx; grpIdx <= lastGrpIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);

      GirderIndexType firstGdrIdx;
      GirderIndexType lastGdrIdx;
      if ( girderKey.girderIndex == ALL_GIRDERS )
      {
         firstGdrIdx = 0;
         lastGdrIdx = pGroup->GetGirderCount() - 1;
      }
      else
      {
         firstGdrIdx = girderKey.girderIndex;
         lastGdrIdx  = firstGdrIdx;
      }

      GirderIndexType nGirdersThisGroup = pGroup->GetGirderCount();
      if ( nGirdersThisGroup <= lastGdrIdx )
      {
         lastGdrIdx = nGirdersThisGroup-1;
      }

      for ( GirderIndexType gdrIdx = firstGdrIdx; gdrIdx <= lastGdrIdx; gdrIdx++ )
      {
         CGirderKey key(grpIdx,gdrIdx);
         girderKeyList.push_back(key);
      } // girder loop
   } // group loop

   
   // Validate the POI
   std::vector<CGirderKey>::iterator keyIter(girderKeyList.begin());
   std::vector<CGirderKey>::iterator keyIterEnd(girderKeyList.end());
   for ( ; keyIter != keyIterEnd; keyIter++ )
   {
      CGirderKey& key = *keyIter;
      std::set<CGirderKey>::iterator found( m_ValidatedPoi.find( key ) );
      if ( found == m_ValidatedPoi.end() )
      {
         LayoutPointsOfInterest( key );
         m_ValidatedPoi.insert(key);
      }
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
   if (!m_bUserLoadsValidated)
   {
      // first make sure our data is wiped
      m_PointLoads.clear();
      m_DistributedLoads.clear();
      m_MomentLoads.clear();

      ValidatePointLoads();
      ValidateDistributedLoads();
      ValidateMomentLoads();

      m_bUserLoadsValidated = true;
   }
}

void CBridgeAgentImp::ValidatePointLoads()
{
   SpanIndexType nSpans = GetSpanCount();

   GET_IFACE_NOCHECK(IEAFStatusCenter, pStatusCenter);
   GET_IFACE(IUserDefinedLoadData, pLoadData );
   GET_IFACE(IBridgeDescription, pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CollectionIndexType nLoads = pLoadData->GetPointLoadCount();
   for(CollectionIndexType loadIdx = 0; loadIdx < nLoads; loadIdx++)
   {
      const CPointLoadData* pPointLoad = pLoadData->GetPointLoad(loadIdx);

      // need to loop over all spans if that is what is selected - use a vector to store span numbers
      std::vector<SpanIndexType> spans;
      if (pPointLoad->m_SpanKey.spanIndex == ALL_SPANS)
      {
         for (SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++)
         {
            spans.push_back(spanIdx);
         }
      }
      else
      {
         if(nSpans <= pPointLoad->m_SpanKey.spanIndex)
         {
            CString strMsg;
            strMsg.Format(_T("Span %d for point load is out of range. Max span number is %d. This load will be ignored."), LABEL_SPAN(pPointLoad->m_SpanKey.spanIndex),nSpans);
            pgsPointLoadStatusItem* pStatusItem = new pgsPointLoadStatusItem(loadIdx,m_LoadStatusGroupID,m_scidPointLoadWarning,strMsg,pPointLoad->m_SpanKey);
            pStatusCenter->Add(pStatusItem);
            continue; // break out of this cycle
         }
         else
         {
            spans.push_back(pPointLoad->m_SpanKey.spanIndex);
         }
      }

      std::vector<SpanIndexType>::iterator spanIter(spans.begin());
      std::vector<SpanIndexType>::iterator spanIterEnd(spans.end());
      for ( ; spanIter != spanIterEnd; spanIter++ )
      {
         SpanIndexType span = *spanIter;

         GirderIndexType nGirders = GetGirderCountBySpan(span);

         std::vector<GirderIndexType> girders;
         if (pPointLoad->m_SpanKey.girderIndex == ALL_GIRDERS)
         {
            for (GirderIndexType i = 0; i < nGirders; i++)
            {
               girders.push_back(i);
            }
         }
         else
         {
            if(nGirders <= pPointLoad->m_SpanKey.girderIndex)
            {
               CString strMsg;
               strMsg.Format(_T("Girder %s for point load is out of range. Max girder number is %s. This load will be ignored."), LABEL_GIRDER(pPointLoad->m_SpanKey.girderIndex), LABEL_GIRDER(nGirders-1));
               pgsPointLoadStatusItem* pStatusItem = new pgsPointLoadStatusItem(loadIdx,m_LoadStatusGroupID,m_scidPointLoadWarning,strMsg,pPointLoad->m_SpanKey);
               pStatusCenter->Add(pStatusItem);
               continue;
            }
            else
            {
               girders.push_back(pPointLoad->m_SpanKey.girderIndex);
            }
         }

         std::vector<GirderIndexType>::iterator gdrIter(girders.begin());
         std::vector<GirderIndexType>::iterator gdrIterEnd(girders.end());
         for ( ; gdrIter != gdrIterEnd; gdrIter++ )
         {
            GirderIndexType girder = *gdrIter;

            CSpanKey thisSpanKey(span,girder);

            UserPointLoad upl;
            upl.m_bLoadOnStartCantilever = pPointLoad->m_bLoadOnCantilever[pgsTypes::metStart];
            upl.m_bLoadOnEndCantilever   = pPointLoad->m_bLoadOnCantilever[pgsTypes::metEnd];

            upl.m_LoadCase = Project2BridgeLoads(pPointLoad->m_LoadCase);
            upl.m_Description = pPointLoad->m_Description;

            // only a light warning for zero loads - don't bail out
            if (IsZero(pPointLoad->m_Magnitude))
            {
               CString strMsg;
               strMsg.Format(_T("Magnitude of point load is zero"));
               pgsPointLoadStatusItem* pStatusItem = new pgsPointLoadStatusItem(loadIdx,m_LoadStatusGroupID,m_scidPointLoadWarning,strMsg,thisSpanKey);
               pStatusCenter->Add(pStatusItem);
            }

            upl.m_Magnitude = pPointLoad->m_Magnitude;

            Float64 start_cantilever_length = GetCantileverLength(thisSpanKey,pgsTypes::metStart);
            Float64 end_cantilever_length = GetCantileverLength(thisSpanKey,pgsTypes::metEnd);
            Float64 span_length = GetSpanLength(thisSpanKey);

            if ( upl.m_bLoadOnStartCantilever )
            {
               const CPierData2* pPier = pBridgeDesc->GetPier(thisSpanKey.spanIndex);
               if ( !pPier->HasCantilever() )
               {
                  CString strMsg;
                  strMsg.Format(_T("Load is located on span cantilever, however span is not cantilevered. This load will be ignored."));
                  pgsPointLoadStatusItem* pStatusItem = new pgsPointLoadStatusItem(loadIdx,m_LoadStatusGroupID,m_scidPointLoadWarning,strMsg,thisSpanKey);
                  pStatusCenter->Add(pStatusItem);
                  continue;
               }
            }

            if ( upl.m_bLoadOnEndCantilever )
            {
               const CPierData2* pPier = pBridgeDesc->GetPier(thisSpanKey.spanIndex+1);
               if ( !pPier->HasCantilever() )
               {
                  CString strMsg;
                  strMsg.Format(_T("Load is located on span cantilever, however span is not cantilevered. This load will be ignored."));
                  pgsPointLoadStatusItem* pStatusItem = new pgsPointLoadStatusItem(loadIdx,m_LoadStatusGroupID,m_scidPointLoadWarning,strMsg,thisSpanKey);
                  pStatusCenter->Add(pStatusItem);
                  continue;
               }
            }

            if (pPointLoad->m_Fractional)
            {
               if( ::InRange(0.0,pPointLoad->m_Location,1.0) )
               {
                  if ( upl.m_bLoadOnStartCantilever )
                  {
                     upl.m_Location = pPointLoad->m_Location * start_cantilever_length;
                  }
                  else if ( upl.m_bLoadOnEndCantilever )
                  {
                     upl.m_Location = pPointLoad->m_Location * end_cantilever_length;
                  }
                  else
                  {
                     upl.m_Location = pPointLoad->m_Location * span_length;
                  }
               }
               else
               {
                  CString strMsg;
                  strMsg.Format(_T("Fractional location value for point load is out of range. Value must range from 0.0 to 1.0. This load will be ignored."));
                  pgsPointLoadStatusItem* pStatusItem = new pgsPointLoadStatusItem(loadIdx,m_LoadStatusGroupID,m_scidPointLoadWarning,strMsg,thisSpanKey);
                  pStatusCenter->Add(pStatusItem);
                  continue;
               }
            }
            else
            {
               if ( upl.m_bLoadOnStartCantilever )
               {
                  if ( ::InRange(0.0,pPointLoad->m_Location,start_cantilever_length) )
                  {
                     upl.m_Location = pPointLoad->m_Location;
                  }
                  else
                  {
                     CString strMsg;
                     strMsg.Format(_T("Location value for point load is out of range. Value must range from 0.0 to start cantilever length. This load will be ignored."));
                     pgsPointLoadStatusItem* pStatusItem = new pgsPointLoadStatusItem(loadIdx,m_LoadStatusGroupID,m_scidPointLoadWarning,strMsg,thisSpanKey);
                     pStatusCenter->Add(pStatusItem);
                     continue;
                  }
               }
               else if ( upl.m_bLoadOnEndCantilever )
               {
                  if ( ::InRange(0.0,pPointLoad->m_Location,end_cantilever_length) )
                  {
                     upl.m_Location = pPointLoad->m_Location;
                  }
                  else
                  {
                     CString strMsg;
                     strMsg.Format(_T("Location value for point load is out of range. Value must range from 0.0 to end cantilever length. This load will be ignored."));
                     pgsPointLoadStatusItem* pStatusItem = new pgsPointLoadStatusItem(loadIdx,m_LoadStatusGroupID,m_scidPointLoadWarning,strMsg,thisSpanKey);
                     pStatusCenter->Add(pStatusItem);
                     continue;
                  }
               }
               else
               {
                  if ( ::InRange(0.0,pPointLoad->m_Location,span_length) )
                  {
                     upl.m_Location = pPointLoad->m_Location;
                  }
                  else
                  {
                     CString strMsg;
                     strMsg.Format(_T("Location value for point load is out of range. Value must range from 0.0 to span length. This load will be ignored."));
                     pgsPointLoadStatusItem* pStatusItem = new pgsPointLoadStatusItem(loadIdx,m_LoadStatusGroupID,m_scidPointLoadWarning,strMsg,thisSpanKey);
                     pStatusCenter->Add(pStatusItem);
                     continue;
                  }
               }
            }

            // add a point of interest at this load location in every interval
            CSegmentKey segmentKey;
            Float64 Xs;
            ConvertSpanPointToSegmentCoordiante(thisSpanKey,pPointLoad->m_Location,&segmentKey,&Xs);

            Float64 segLength = GetSegmentLength(segmentKey);
            Xs = ::ForceIntoRange(0.0,Xs,segLength);

            pgsPointOfInterest poi(segmentKey,Xs,POI_CONCLOAD);
            m_pPoiMgr->AddPointOfInterest( poi );

            // put load into our collection
            IntervalIndexType intervalIdx;
            if ( pPointLoad->m_LoadCase == UserLoads::LL_IM )
            {
               intervalIdx = m_IntervalManager.GetLiveLoadInterval();
            }
            else
            {
               intervalIdx = m_IntervalManager.GetUserLoadInterval(thisSpanKey,pPointLoad->m_LoadCase,pPointLoad->m_EventIndex);
            }
            ATLASSERT(intervalIdx != INVALID_INDEX);

            CUserLoadKey key(thisSpanKey,intervalIdx);
            std::map<CUserLoadKey,std::vector<UserPointLoad>>::iterator found(m_PointLoads.find(key));
            if ( found == m_PointLoads.end() )
            {
               std::vector<UserPointLoad> pointLoads;
               pointLoads.push_back(upl);
               m_PointLoads.insert(std::make_pair(key,pointLoads));
            }
            else
            {
               std::vector<UserPointLoad>& pointLoads = found->second;
               pointLoads.push_back(upl);
            }
         }
      }
   }
}

void CBridgeAgentImp::ValidateDistributedLoads()
{
   SpanIndexType nSpans = GetSpanCount();

   GET_IFACE_NOCHECK(IEAFStatusCenter,pStatusCenter);
   GET_IFACE( IUserDefinedLoadData, pUdl );

   CollectionIndexType nLoads = pUdl->GetDistributedLoadCount();
   for(CollectionIndexType loadIdx = 0; loadIdx < nLoads; loadIdx++)
   {
      const CDistributedLoadData* pDistLoad = pUdl->GetDistributedLoad(loadIdx);

      // need to loop over all spans if that is what is selected - user a vector to store span numbers
      std::vector<SpanIndexType> spans;
      if (pDistLoad->m_SpanKey.spanIndex == ALL_SPANS)
      {
         for (SpanIndexType i = 0; i < nSpans; i++)
         {
            spans.push_back(i);
         }
      }
      else
      {
         if(nSpans <= pDistLoad->m_SpanKey.spanIndex)
         {
            CString strMsg;
            strMsg.Format(_T("Span %d for Distributed load is out of range. Max span number is %d. This load will be ignored."), LABEL_SPAN(pDistLoad->m_SpanKey.spanIndex),nSpans);
            pgsDistributedLoadStatusItem* pStatusItem = new pgsDistributedLoadStatusItem(loadIdx,m_LoadStatusGroupID,m_scidDistributedLoadWarning,strMsg,pDistLoad->m_SpanKey);
            pStatusCenter->Add(pStatusItem);
            continue; // break out of this cycle
         }
         else
         {
            spans.push_back(pDistLoad->m_SpanKey.spanIndex);
         }
      }

      std::vector<SpanIndexType>::iterator spanIter(spans.begin());
      std::vector<SpanIndexType>::iterator spanIterEnd(spans.end());
      for (; spanIter != spanIterEnd; spanIter++)
      {
         SpanIndexType span = *spanIter;

         GirderIndexType nGirders = GetGirderCountBySpan(span);

         std::vector<GirderIndexType> girders;
         if (pDistLoad->m_SpanKey.girderIndex == ALL_GIRDERS)
         {
            for (GirderIndexType i = 0; i < nGirders; i++)
            {
               girders.push_back(i);
            }
         }
         else
         {
            if(nGirders <= pDistLoad->m_SpanKey.girderIndex)
            {
               CString strMsg;
               strMsg.Format(_T("Girder %s for Distributed load is out of range. Max girder number is %s. This load will be ignored."), LABEL_GIRDER(pDistLoad->m_SpanKey.girderIndex), LABEL_GIRDER(nGirders-1));
               pgsDistributedLoadStatusItem* pStatusItem = new pgsDistributedLoadStatusItem(loadIdx,m_LoadStatusGroupID,m_scidDistributedLoadWarning,strMsg,pDistLoad->m_SpanKey);
               pStatusCenter->Add(pStatusItem);
               continue;
            }
            else
            {
               girders.push_back(pDistLoad->m_SpanKey.girderIndex);
            }
         }

         std::vector<GirderIndexType>::iterator girderIter(girders.begin());
         std::vector<GirderIndexType>::iterator girderIterEnd(girders.end());
         for ( ; girderIter != girderIterEnd; girderIter++)
         {
            GirderIndexType girder = *girderIter;

            CSpanKey thisSpanKey(span,girder);

            Float64 span_length = GetSpanLength(span,girder); // span length measured along CL girder

            UserDistributedLoad upl;
            upl.m_LoadCase = Project2BridgeLoads(pDistLoad->m_LoadCase);
            upl.m_Description = pDistLoad->m_Description;

            if (pDistLoad->m_Type == UserLoads::Uniform)
            {
               if (IsZero(pDistLoad->m_WStart) && IsZero(pDistLoad->m_WEnd))
               {
                  CString strMsg;
                  strMsg.Format(_T("Magnitude of Distributed load is zero"));
                  pgsDistributedLoadStatusItem* pStatusItem = new pgsDistributedLoadStatusItem(loadIdx,m_LoadStatusGroupID,m_scidDistributedLoadWarning,strMsg,thisSpanKey);
                  pStatusCenter->Add(pStatusItem);
               }

               // uniform load
               upl.m_WStart = pDistLoad->m_WStart;
               upl.m_WEnd   = pDistLoad->m_WStart;

               upl.m_StartLocation = 0.0;
               upl.m_EndLocation   = span_length;
            }
            else
            {
               // only a light warning for zero loads - don't bail out
               if (IsZero(pDistLoad->m_WStart) && IsZero(pDistLoad->m_WEnd))
               {
                  CString strMsg;
                  strMsg.Format(_T("Magnitude of Distributed load is zero"));
                  pgsDistributedLoadStatusItem* pStatusItem = new pgsDistributedLoadStatusItem(loadIdx,m_LoadStatusGroupID,m_scidDistributedLoadWarning,strMsg,thisSpanKey);
                  pStatusCenter->Add(pStatusItem);
               }

               upl.m_WStart = pDistLoad->m_WStart;
               upl.m_WEnd   = pDistLoad->m_WEnd;

               // location
               if( pDistLoad->m_EndLocation <= pDistLoad->m_StartLocation )
               {
                  CString strMsg;
                  strMsg.Format(_T("Start locaton of distributed load is greater than end location. This load will be ignored."));
                  pgsDistributedLoadStatusItem* pStatusItem = new pgsDistributedLoadStatusItem(loadIdx,m_LoadStatusGroupID,103,strMsg,thisSpanKey);
                  pStatusCenter->Add(pStatusItem);
                  continue;
               }

               if (pDistLoad->m_Fractional)
               {
                  if( (0.0 <= pDistLoad->m_StartLocation && pDistLoad->m_StartLocation <= 1.0) &&
                      (0.0 <= pDistLoad->m_EndLocation   && pDistLoad->m_EndLocation   <= 1.0) )
                  {
                     upl.m_StartLocation = pDistLoad->m_StartLocation * span_length;
                     upl.m_EndLocation   = pDistLoad->m_EndLocation * span_length;
                  }
                  else
                  {
                     CString strMsg;
                     strMsg.Format(_T("Fractional location value for Distributed load is out of range. Value must range from 0.0 to 1.0. This load will be ignored."));
                     pgsDistributedLoadStatusItem* pStatusItem = new pgsDistributedLoadStatusItem(loadIdx,m_LoadStatusGroupID,m_scidDistributedLoadWarning,strMsg,thisSpanKey);
                     pStatusCenter->Add(pStatusItem);
                     continue;
                  }
               }
               else
               {
                  if( 0.0 <= pDistLoad->m_StartLocation && pDistLoad->m_StartLocation <= span_length &&
                      0.0 <= pDistLoad->m_EndLocation   && pDistLoad->m_EndLocation   <= span_length+TOL)
                  {
                     upl.m_StartLocation = pDistLoad->m_StartLocation;

                     // fudge a bit if user entered a slightly high value
                     if (pDistLoad->m_EndLocation < span_length)
                     {
                        upl.m_EndLocation   = pDistLoad->m_EndLocation;
                     }
                     else
                     {
                        upl.m_EndLocation   = span_length;
                     }

                  }
                  else
                  {
                     CString strMsg;
                     strMsg.Format(_T("Location value for Distributed load is out of range. Value must range from 0.0 to span length. This load will be ignored."));
                     pgsDistributedLoadStatusItem* pStatusItem = new pgsDistributedLoadStatusItem(loadIdx,m_LoadStatusGroupID,m_scidDistributedLoadWarning,strMsg,thisSpanKey);
                     pStatusCenter->Add(pStatusItem);
                     continue;
                  }
               }
            }

            // add point of interests at start and end of this load
            CSegmentKey segmentKey;
            Float64 Xs;
            ConvertSpanPointToSegmentCoordiante(thisSpanKey,upl.m_StartLocation,&segmentKey,&Xs);

            Float64 segLength = GetSegmentLength(segmentKey);
            Xs = ::ForceIntoRange(0.0,Xs,segLength);

            pgsPointOfInterest poiStart(segmentKey,Xs);
            m_pPoiMgr->AddPointOfInterest( poiStart );

            ConvertSpanPointToSegmentCoordiante(thisSpanKey,upl.m_EndLocation,&segmentKey,&Xs);

            segLength = GetSegmentLength(segmentKey);
            Xs = ::ForceIntoRange(0.0,Xs,segLength);

            pgsPointOfInterest poiEnd(segmentKey,Xs);
            m_pPoiMgr->AddPointOfInterest( poiEnd );

            IntervalIndexType intervalIdx;
            if ( pDistLoad->m_LoadCase == UserLoads::LL_IM )
            {
               intervalIdx = m_IntervalManager.GetLiveLoadInterval();
            }
            else
            {
               intervalIdx = m_IntervalManager.GetUserLoadInterval(thisSpanKey,pDistLoad->m_LoadCase,pDistLoad->m_EventIndex);
            }
            ATLASSERT(intervalIdx != INVALID_INDEX);

            CUserLoadKey key(thisSpanKey,intervalIdx);
            std::map<CUserLoadKey,std::vector<UserDistributedLoad>>::iterator found(m_DistributedLoads.find(key));
            if ( found == m_DistributedLoads.end() )
            {
               std::vector<UserDistributedLoad> distributedLoads;
               distributedLoads.push_back(upl);
               m_DistributedLoads.insert(std::make_pair(key,distributedLoads));
            }
            else
            {
               std::vector<UserDistributedLoad>& distributedLoads = found->second;
               distributedLoads.push_back(upl);
            }
         }
      }
   }
}

void CBridgeAgentImp::ValidateMomentLoads()
{
   SpanIndexType nSpans = GetSpanCount();

   GET_IFACE_NOCHECK(IEAFStatusCenter, pStatusCenter);
   GET_IFACE(IUserDefinedLoadData, pLoadData );
   GET_IFACE(IBridgeDescription, pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CollectionIndexType nLoads = pLoadData->GetMomentLoadCount();
   for(CollectionIndexType loadIdx = 0; loadIdx < nLoads; loadIdx++)
   {
      const CMomentLoadData* pMomentLoad = pLoadData->GetMomentLoad(loadIdx);

      // need to loop over all spans if that is what is selected - user a vector to store span numbers
      std::vector<SpanIndexType> spans;
      if (pMomentLoad->m_SpanKey.spanIndex == ALL_SPANS)
      {
         for (SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++)
         {
            spans.push_back(spanIdx);
         }
      }
      else
      {
         if(nSpans <= pMomentLoad->m_SpanKey.spanIndex)
         {
            CString strMsg;
            strMsg.Format(_T("Span %d for moment load is out of range. Max span number is %d. This load will be ignored."), LABEL_SPAN(pMomentLoad->m_SpanKey.spanIndex),nSpans);
            pgsMomentLoadStatusItem* pStatusItem = new pgsMomentLoadStatusItem(loadIdx,m_LoadStatusGroupID,m_scidMomentLoadWarning,strMsg,pMomentLoad->m_SpanKey);
            pStatusCenter->Add(pStatusItem);
            continue; // break out of this cycle
         }
         else
         {
            spans.push_back(pMomentLoad->m_SpanKey.spanIndex);
         }
      }

      std::vector<SpanIndexType>::iterator spanIter(spans.begin());
      std::vector<SpanIndexType>::iterator spanIterEnd(spans.end());
      for ( ; spanIter != spanIterEnd; spanIter++ )
      {
         SpanIndexType span = *spanIter;

         GirderIndexType nGirders = GetGirderCountBySpan(span);

         std::vector<GirderIndexType> girders;
         if (pMomentLoad->m_SpanKey.girderIndex == ALL_GIRDERS)
         {
            for (GirderIndexType i = 0; i < nGirders; i++)
            {
               girders.push_back(i);
            }
         }
         else
         {
            if(nGirders <= pMomentLoad->m_SpanKey.girderIndex)
            {
               CString strMsg;
               strMsg.Format(_T("Girder %s for moment load is out of range. Max girder number is %s. This load will be ignored."), LABEL_GIRDER(pMomentLoad->m_SpanKey.girderIndex), LABEL_GIRDER(nGirders-1));
               pgsMomentLoadStatusItem* pStatusItem = new pgsMomentLoadStatusItem(loadIdx,m_LoadStatusGroupID,m_scidMomentLoadWarning,strMsg,pMomentLoad->m_SpanKey);
               pStatusCenter->Add(pStatusItem);
               continue;
            }
            else
            {
               girders.push_back(pMomentLoad->m_SpanKey.girderIndex);
            }
         }

         std::vector<GirderIndexType>::iterator gdrIter(girders.begin());
         std::vector<GirderIndexType>::iterator gdrIterEnd(girders.end());
         for ( ; gdrIter != gdrIterEnd; gdrIter++ )
         {
            GirderIndexType girder = *gdrIter;

            CSpanKey thisSpanKey(span,girder);

            UserPointLoad upl;
            upl.m_LoadCase = Project2BridgeLoads(pMomentLoad->m_LoadCase);
            upl.m_Description = pMomentLoad->m_Description;

            // only a light warning for zero loads - don't bail out
            if (IsZero(pMomentLoad->m_Magnitude))
            {
               CString strMsg;
               strMsg.Format(_T("Magnitude of moment load is zero"));
               pgsMomentLoadStatusItem* pStatusItem = new pgsMomentLoadStatusItem(loadIdx,m_LoadStatusGroupID,m_scidMomentLoadWarning,strMsg,thisSpanKey);
               pStatusCenter->Add(pStatusItem);
            }

            upl.m_Magnitude = pMomentLoad->m_Magnitude;

            Float64 span_length = GetSpanLength(span,girder); // span length measured along CL girder

            if (pMomentLoad->m_Fractional)
            {
               if(0.0 <= pMomentLoad->m_Location && pMomentLoad->m_Location <= 1.0)
               {
                  upl.m_Location = pMomentLoad->m_Location * span_length;
               }
               else
               {
                  CString strMsg;
                  strMsg.Format(_T("Fractional location value for moment load is out of range. Value must range from 0.0 to 1.0. This load will be ignored."));
                  pgsMomentLoadStatusItem* pStatusItem = new pgsMomentLoadStatusItem(loadIdx,m_LoadStatusGroupID,m_scidMomentLoadWarning,strMsg,thisSpanKey);
                  pStatusCenter->Add(pStatusItem);
                  continue;
               }
            }
            else
            {
               if(0.0 <= pMomentLoad->m_Location && pMomentLoad->m_Location <= span_length)
               {
                  upl.m_Location = pMomentLoad->m_Location;
               }
               else
               {
                  CString strMsg;
                  strMsg.Format(_T("Location value for moment load is out of range. Value must range from 0.0 to span length. This load will be ignored."));
                  pgsMomentLoadStatusItem* pStatusItem = new pgsMomentLoadStatusItem(loadIdx,m_LoadStatusGroupID,m_scidMomentLoadWarning,strMsg,thisSpanKey);
                  pStatusCenter->Add(pStatusItem);
                  continue;
               }
            }

            // add a point of interest at this load location in every interval
            CSegmentKey segmentKey;
            Float64 Xs;
            ConvertSpanPointToSegmentCoordiante(thisSpanKey,pMomentLoad->m_Location,&segmentKey,&Xs);

            Float64 segLength = GetSegmentLength(segmentKey);
            Xs = ::ForceIntoRange(0.0,Xs,segLength);

            pgsPointOfInterest poi(segmentKey,Xs,POI_CONCLOAD);
            m_pPoiMgr->AddPointOfInterest( poi );

            // put load into our collection
            IntervalIndexType intervalIdx;
            if ( pMomentLoad->m_LoadCase == UserLoads::LL_IM )
            {
               intervalIdx = m_IntervalManager.GetLiveLoadInterval();
            }
            else
            {
               intervalIdx = m_IntervalManager.GetUserLoadInterval(thisSpanKey,pMomentLoad->m_LoadCase,pMomentLoad->m_EventIndex);
            }
            ATLASSERT(intervalIdx != INVALID_INDEX);

            CUserLoadKey key(thisSpanKey,intervalIdx);
            std::map<CUserLoadKey,std::vector<UserMomentLoad>>::iterator found(m_MomentLoads.find(key));
            if ( found == m_MomentLoads.end() )
            {
               std::vector<UserMomentLoad> momentLoads;
               momentLoads.push_back(upl);
               m_MomentLoads.insert(std::make_pair(key,momentLoads));
            }
            else
            {
               std::vector<UserMomentLoad>& momentLoads = found->second;
               momentLoads.push_back(upl);
            }
         }
      }
   }
}

void CBridgeAgentImp::ValidateSegmentOrientation(const CSegmentKey& segmentKey)
{
   ValidatePointsOfInterest(segmentKey); // calls VALIDATE(GIRDER);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
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
         vPoi = GetPointsOfInterest(segmentKey);
         poi = vPoi.front();
         break;

      case pgsTypes::MidspanNormal:
         vPoi = GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_5L);
         poi = vPoi.front();
         break;

      case pgsTypes::EndNormal:
         vPoi = GetPointsOfInterest(segmentKey);
         poi = vPoi.back();
         break;

      case pgsTypes::Plumb:
      default:
         ATLASSERT(false); // should never get here
      };

      GetStationAndOffset(poi,&station,&offset);

      MatingSurfaceIndexType nMatingSurfaces = GetNumberOfMatingSurfaces(segmentKey);
      ATLASSERT(0 < nMatingSurfaces); // going to subtract 1 below, and this is an unsigned value

      Float64 left_mating_surface_offset  = GetMatingSurfaceLocation(poi,0);
      Float64 right_mating_surface_offset = GetMatingSurfaceLocation(poi,nMatingSurfaces-1);

      Float64 left_mating_surface_width  = GetMatingSurfaceWidth(poi,0);
      Float64 right_mating_surface_width = GetMatingSurfaceWidth(poi,nMatingSurfaces-1);

      Float64 left_edge_offset  = left_mating_surface_offset  - left_mating_surface_width/2;
      Float64 right_edge_offset = right_mating_surface_offset + right_mating_surface_width/2;

      Float64 distance = right_edge_offset - left_edge_offset;

      if ( IsZero(distance) )
      {
         orientation = GetSlope(station,offset);
      }
      else
      {   
         Float64 ya_left  = GetElevation(station,offset+left_edge_offset);
         Float64 ya_right = GetElevation(station,offset+right_edge_offset);

         orientation = (ya_left - ya_right)/distance;
      }
   }

   CComPtr<ISuperstructureMember> ssmbr;
   GetSuperstructureMember(segmentKey,&ssmbr);

   CComPtr<ISuperstructureMemberSegment> segment;
   ssmbr->get_Segment(segmentKey.segmentIndex,&segment);

   segment->put_Orientation(orientation);
}

bool CBridgeAgentImp::BuildCogoModel()
{
   ATLASSERT(m_CogoModel == NULL);
   m_CogoModel.Release();
   m_CogoModel.CoCreateInstance(CLSID_CogoModel);

   GET_IFACE( IRoadwayData, pIAlignment );
   if ( pIAlignment == NULL )
   {
      return false;
   }

   AlignmentData2 alignment_data   = pIAlignment->GetAlignmentData2();
   ProfileData2   profile_data     = pIAlignment->GetProfileData2();
   RoadwaySectionData section_data = pIAlignment->GetRoadwaySectionData();

   CComPtr<IAlignmentCollection> alignments;
   m_CogoModel->get_Alignments(&alignments);

   CComPtr<IAlignment> alignment;
   alignments->Add(g_AlignmentID,&alignment); // create the main alignment object
   ATLASSERT(alignment);

   CComPtr<IProfile> profile;
   alignment->get_Profile(&profile);
   profile->Clear();

   CComPtr<IPointCollection> points;
   m_CogoModel->get_Points(&points);

   // Setup the alignment
   if ( alignment_data.HorzCurves.size() == 0 )
   {
      // straight alignment
      CogoObjectID id1 = 20000;
      CogoObjectID id2 = 20001;
      points->Add(id1,0,0,NULL); // start at point 0,0

      CComQIPtr<ILocate> locate(m_CogoModel);
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
      m_CogoModel->get_HorzCurves(&curves);

      CComPtr<IPointCollection> points;
      m_CogoModel->get_Points(&points);

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

      CogoObjectID curveID = 1;
      CollectionIndexType curveIdx = 0;

      std::vector<HorzCurveData>::iterator iter;
      for ( iter = alignment_data.HorzCurves.begin(); iter != alignment_data.HorzCurves.end(); iter++, curveID++, curveIdx++ )
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
               os << _T("The central angle of horizontal curve ") << curveID << _T(" is 0 or 180 degrees. Horizontal curve was modeled as a single point at the PI location.");
               std::_tstring strMsg = os.str();
               pgsAlignmentDescriptionStatusItem* p_status_item = new pgsAlignmentDescriptionStatusItem(m_StatusGroupID,m_scidAlignmentError,0,strMsg.c_str());
               GET_IFACE(IEAFStatusCenter,pStatusCenter);
               pStatusCenter->Add(p_status_item);
               
               alignment->AddEx(pi);

               back_tangent = fwd_tangent;
               prev_curve_ST_station = pi_station;

               if ( curveIdx == 0 )
               {
                  // this is the first curve so set the reference station at the PI
                  alignment->put_RefStation( CComVariant(pi_station) );
               }

               continue; // GO TO NEXT HORIZONTAL CURVE

               //strMsg += std::_tstring(_T("\nSee Status Center for Details"));
               //THROW_UNWIND(strMsg.c_str(),-1);
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

            // determine the station of the ST point because this point will serve
            // as the next point on back tangent
            Float64 L;
            hr = hc->get_TotalLength(&L);
            if ( FAILED(hr) )
            {
               std::_tostringstream os;
               if ( hr == COGO_E_SPIRALSOVERLAP )
               {
                  os << _T("The description of horizontal curve ") << curveID << _T(" is invalid. The entry and exit spirals overlap.");
               }
               else
               {
                  os << _T("An unknown error occured while modeling horizontal curve ") << curveID << _T(".");
               }

               os << std::endl;
               os << _T("The horizontal curve was ignored.");

               std::_tstring strMsg = os.str();
               pgsAlignmentDescriptionStatusItem* p_status_item = new pgsAlignmentDescriptionStatusItem(m_StatusGroupID,m_scidAlignmentError,0,strMsg.c_str());
               GET_IFACE(IEAFStatusCenter,pStatusCenter);
               pStatusCenter->Add(p_status_item);

               continue;
            }

            alignment->AddEx(hc);

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
      m_CogoModel->get_VertCurves(&vcurves);

      CComPtr<IProfilePointCollection> profilepoints;
      m_CogoModel->get_ProfilePoints(&profilepoints);

      CogoObjectID curveID = 1;
      CollectionIndexType curveIdx = 0;

      std::vector<VertCurveData>::iterator iter( profile_data.VertCurves.begin() );
      std::vector<VertCurveData>::iterator iterEnd( profile_data.VertCurves.end() );
      for ( ; iter != iterEnd; iter++ )
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
            // zero length vertical curve.... this is ok as it creates
            // a profile point. It isn't common so warn the user
            std::_tostringstream os;
            os << _T("Vertical curve ") << curveID << _T(" is a zero length curve.");
            std::_tstring strMsg = os.str();

            pgsAlignmentDescriptionStatusItem* p_status_item = new pgsAlignmentDescriptionStatusItem(m_StatusGroupID,m_scidAlignmentWarning,1,strMsg.c_str());
            GET_IFACE(IEAFStatusCenter,pStatusCenter);
            pStatusCenter->Add(p_status_item);
         }

         // add a vertical curve
         Float64 pfg_station = pvi_station + (iter == iterEnd-1 ? 100 : L2);
         Float64 pfg_elevation = pvi_elevation + curve_data.ExitGrade*(iter == iterEnd-1 ? 100 : L2);

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
            {
               os << _T("Vertical Curve ") << curveID << _T(" begins before the profile reference point.");
            }
            else
            {
               os << _T("Vertical curve ") << curveID << _T(" begins before curve ") << (curveID-1) << _T(" ends.");
            }

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

         curveID++;
         curveIdx++;
      }
   }

   // Create the roadway surface model. Make it very much larger than the bridge so
   // none of the points fall off the surface.
   CComPtr<ISurfaceCollection> surfaces;
   profile->get_Surfaces(&surfaces);

   CComPtr<ISurface> surface;
   surface.CoCreateInstance(CLSID_Surface);
   surfaces->Add(surface);

   surface->put_ID(COGO_FINISHED_SURFACE_ID);

   CComPtr<ISurfaceTemplateCollection> templates;
   surface->get_SurfaceTemplates(&templates);

#pragma Reminder("UPDATE: assumes crown point is on the same side of the alignment at all sections")
   // need to update the UI to make this TRUE

   // Ridge Point 2 is always the crown point
   if ( 0 < section_data.Superelevations.front().CrownPointOffset )
   {
      surface->put_AlignmentPoint(1);
      surface->put_ProfileGradePoint(1);
   }
   else if ( section_data.Superelevations.front().CrownPointOffset < 0 )
   {
      surface->put_AlignmentPoint(3);
      surface->put_ProfileGradePoint(3);
   }
   else
   {
      surface->put_AlignmentPoint(2);
      surface->put_ProfileGradePoint(2);
   }

   // NOTE: width of roadway surface is arbitrary
   // it could be determined from the bridge deck
   // input (and then doubled so the surface is for sure wide enough)
   Float64 width = 500; // 500 m is pretty wide... no real highway bridge is going to be this wide

   // Create a template section well before the start of the bridge
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   Float64 bridge_length = pBridgeDesc->GetLength();

   CrownData2& startCrown = section_data.Superelevations.front();
   const CPierData2* pPier = pBridgeDesc->GetPier(0);
   Float64 startStation = pPier->GetStation();
   startStation -= bridge_length;

   if ( startStation < startCrown.Station )
   {
      CComPtr<ISurfaceTemplate> startSurfaceTemplate;
      startSurfaceTemplate.CoCreateInstance(CLSID_SurfaceTemplate);
      startSurfaceTemplate->put_Station(CComVariant(startStation));
      startSurfaceTemplate->AddSegment(width,-startCrown.Left,tsHorizontal);
      startSurfaceTemplate->AddSegment(fabs(startCrown.CrownPointOffset),-startCrown.Left,tsHorizontal);
      startSurfaceTemplate->AddSegment(fabs(startCrown.CrownPointOffset), startCrown.Right,tsHorizontal);
      startSurfaceTemplate->AddSegment(width, startCrown.Right,tsHorizontal);

      templates->Add(startSurfaceTemplate);
   }

   // NOTE: Should be using the Cogo Model Superelevation objects, but
   // this is easier....
   std::vector<CrownData2>::iterator iter(section_data.Superelevations.begin());
   std::vector<CrownData2>::iterator iterEnd(section_data.Superelevations.end());
   for ( ; iter != iterEnd; iter++ )
   {
      CrownData2& super = *iter;

      CComPtr<ISurfaceTemplate> surfaceTemplate;
      surfaceTemplate.CoCreateInstance(CLSID_SurfaceTemplate);
      surfaceTemplate->put_Station(CComVariant(super.Station));
      surfaceTemplate->AddSegment(width,-super.Left,tsHorizontal);
      surfaceTemplate->AddSegment(fabs(super.CrownPointOffset),-super.Left,tsHorizontal);
      surfaceTemplate->AddSegment(fabs(super.CrownPointOffset), super.Right,tsHorizontal);
      surfaceTemplate->AddSegment(width, super.Right,tsHorizontal);

      templates->Add(surfaceTemplate);
   }

   // Create a template section well after the end of the bridge
   CrownData2& endCrown = section_data.Superelevations.back();
   pPier = pBridgeDesc->GetPier(pBridgeDesc->GetPierCount()-1);
   Float64 endStation = pPier->GetStation();
   endStation += bridge_length;
   if ( endCrown.Station < endStation )
   {
      CComPtr<ISurfaceTemplate> endSurfaceTemplate;
      endSurfaceTemplate.CoCreateInstance(CLSID_SurfaceTemplate);
      endSurfaceTemplate->put_Station(CComVariant(endStation));
      endSurfaceTemplate->AddSegment(width,-endCrown.Left,tsHorizontal);
      endSurfaceTemplate->AddSegment(fabs(endCrown.CrownPointOffset),-endCrown.Left,tsHorizontal);
      endSurfaceTemplate->AddSegment(fabs(endCrown.CrownPointOffset), endCrown.Right,tsHorizontal);
      endSurfaceTemplate->AddSegment(width, endCrown.Right,tsHorizontal);

      templates->Add(endSurfaceTemplate);
   }

   return true;
}

bool CBridgeAgentImp::BuildBridgeGeometryModel()
{
   WATCH(_T("Building Bridge Geometry Model"));

   CComPtr<IBridgeGeometry> bridgeGeometry;
   m_Bridge->get_BridgeGeometry(&bridgeGeometry);

   // Get the alignment that will be used for the bridge
   CComPtr<IAlignmentCollection> alignments;
   m_CogoModel->get_Alignments(&alignments);

   CComPtr<IAlignment> alignment;
   alignments->get_Item(g_AlignmentID,&alignment);

   // associated alignment with the bridge geometry
   bridgeGeometry->putref_Alignment(g_AlignmentID,alignment);
   bridgeGeometry->put_BridgeAlignmentID(g_AlignmentID);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CBridgeGeometryModelBuilder model_builder;
   model_builder.BuildBridgeGeometryModel(pBridgeDesc,m_CogoModel,alignment,bridgeGeometry);
   return true;
}

bool CBridgeAgentImp::BuildBridgeModel()
{
   //////////////////////////////////////////////////////////////////////////
   // NOTE
   // The analysis intervals that we use map directly to the stages
   // in the generic bridge model. The generic bridge model stages are
   // different than the stages in the timeline manager.
   //
   // The generic bridge model supports the analysis implementation, not the
   // raw input, for this reason we use analysis intervals as the generic
   // bridge "stages".
   //////////////////////////////////////////////////////////////////////////

#pragma Reminder("WORKING HERE - somewhere I need to validate the pier geometry... girders must be on the XBeam and columns must fit")
   // Need to create bridge so we can get to the cogo model
   ATLASSERT(m_Bridge == NULL);
   m_Bridge.Release(); // release just in case, but it should already be done
   HRESULT hr = m_Bridge.CoCreateInstance(CLSID_GenericBridge);
   ATLASSERT(SUCCEEDED(hr));

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // Create effective flange width tool (it is needed to validate some of the parameters)
   CComObject<CEffectiveFlangeWidthTool>* pTool;
   hr = CComObject<CEffectiveFlangeWidthTool>::CreateInstance(&pTool);
   pTool->Init(m_pBroker,m_StatusGroupID);
   m_EffFlangeWidthTool = pTool;
   if ( FAILED(hr) || m_EffFlangeWidthTool == NULL )
   {
      THROW_UNWIND(_T("Custom Effective Flange Width Tool not created"),-1);
   }

   m_SectCutTool->putref_EffectiveFlangeWidthTool(m_EffFlangeWidthTool);

   // Set up the interval manager
   GET_IFACE(ILossParameters,pLossParameters);
   pgsTypes::LossMethod loss_method = pLossParameters->GetLossMethod();
   bool bTimeStepMethod = (loss_method == pgsTypes::TIME_STEP ? true : false);
   m_IntervalManager.BuildIntervals(pBridgeDesc->GetTimelineManager(),bTimeStepMethod);

   BuildBridgeGeometryModel(); // creates the framing geometry for the bridge model

   // Now that the basic input has been generated, update the bridge model
   // This will get all the geometry correct and build the pier objects.
   // This basic geometry is needed to lay out girders, deck, etc
   m_Bridge->UpdateBridgeModel();

   WATCH(_T("Validating Bridge Model"));

   if ( !LayoutPiers(pBridgeDesc) )
   {
      return false;
   }

   if ( !LayoutGirders(pBridgeDesc) )
   {
      return false;
   }

   if ( !LayoutDeck(pBridgeDesc) )
   {
      return false;
   }

   if ( !LayoutTrafficBarriers(pBridgeDesc) )
   {
      return false;
   }

   // Layout the bridge again to complete the geometry
   // This updates the geometry for the items that were added (girder, deck, etc)
   m_Bridge->UpdateBridgeModel();

   // check bridge for errors - will throw an exception if there are errors
   CheckBridge();

   return true;
}

bool CBridgeAgentImp::LayoutPiers(const CBridgeDescription2* pBridgeDesc)
{
   // Build the physical generic pier model for the generic bridge object
   // Since we aren't using this information right now, this can be deferred until
   // a later date. The CPierData2 object contains enough information to create the
   // full physical pier model.
#pragma Reminder("WORKING HERE - build full pier model") // also add access methods to get pier model data
   // (may be useful for 3D BrIM type models

   //PierIndexType nPiers = pBridgeDesc->GetPierCount();
   //for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   //{
   //   const CPierData2* pPierData = pBridgeDesc->GetPier(pierIdx);

   //   if ( pPierData->GetPierModelType() == pgsTypes::pmtPhysical )
   //   {
   //      //
   //      // General Pier Information
   //      //
   //      CComPtr<IBridgePier> pier;
   //      GetGenericBridgePier(pierIdx,&pier);

   //      // Pier Type (derived from boundary condition)
   //      PierType pierType = GetPierType(pPierData);
   //      pier->put_Type(pierType);

   //      // skew angle should already be set

   //      // 
   //      // set the crown slope in the plane of the pier???
   //      // set the deck elevation at the alignment???
   //      // set the crown point offset???
   //      // set the curb line offsets???

   //      //
   //      // Cross Beam
   //      //
   //      CComPtr<ILinearCrossBeam> xbeam;
   //      xbeam.CoCreateInstance(CLSID_LinearCrossBeam);

   //      Float64 H1, H2, H3, H4;
   //      Float64 X1, X2, X3, X4;
   //      pPierData->GetXBeamDimensions(pgsTypes::pstLeft,&H1,&H2,&X1,&X2);
   //      pPierData->GetXBeamDimensions(pgsTypes::pstRight,&H3,&H4,&X3,&X4);

   //      Float64 W1 = pPierData->GetXBeamWidth();

   //      Float64 X5, X6;
   //      pPierData->GetXBeamOverhangs(&X5,&X6);

   //      Float64 H5 = pPierData->GetDiaphragmHeight(pgsTypes::Back) + pPierData->GetDiaphragmHeight(pgsTypes::Ahead);
   //      Float64 W2 = pPierData->GetDiaphragmWidth(pgsTypes::Back) + pPierData->GetDiaphragmWidth(pgsTypes::Ahead);

   //      xbeam->put_H1(H1);
   //      xbeam->put_H2(H2);
   //      xbeam->put_H3(H3);
   //      xbeam->put_H4(H4);
   //      xbeam->put_H5(H5);

   //      xbeam->put_X1(X1);
   //      xbeam->put_X2(X2);
   //      xbeam->put_X3(X3);
   //      xbeam->put_X4(X4);

   //      xbeam->put_W1(W1);
   //      xbeam->put_W2(W2);

   //      CComQIPtr<ICrossBeam> crossBeam(xbeam);
   //      pier->putref_CrossBeam(crossBeam);
   //      
   //      //
   //      // Bearing Layout
   //      //
   //      CComPtr<IBearingLayout> bearingLayout;
   //      bearingLayout.CoCreateInstance(CLSID_BearingLayout);
   //      pier->putref_BearingLayout(bearingLayout);

   //      //
   //      // Column Layout
   //      //
   //      CComPtr<IColumnLayout> columnLayout;
   //      columnLayout.CoCreateInstance(CLSID_ColumnLayout);
   //      pier->putref_ColumnLayout(columnLayout);
   //   }
   //}

   return true;
}

bool CBridgeAgentImp::LayoutGirders(const CBridgeDescription2* pBridgeDesc)
{
   CComPtr<IBridgeGeometry> geometry;
   m_Bridge->get_BridgeGeometry(&geometry);

   Float64 gross_slab_depth = pBridgeDesc->GetDeckDescription()->GrossDepth;
   pgsTypes::SupportedDeckType deckType = pBridgeDesc->GetDeckDescription()->DeckType;

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);

      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CGirderKey girderKey(grpIdx,gdrIdx);

         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);

         // get the girder library entry
         const GirderLibraryEntry* pGirderEntry = GetGirderLibraryEntry(girderKey);

         GirderLibraryEntry::Dimensions dimensions = pGirderEntry->GetDimensions();

         CComPtr<IBeamFactory> beamFactory;
         pGirderEntry->GetBeamFactory(&beamFactory);

         // create a superstructure member
         LocationType locationType = (gdrIdx == 0 ? ltLeftExteriorGirder : (gdrIdx == nGirders-1 ? ltRightExteriorGirder : ltInteriorGirder));

         CComPtr<ISuperstructureMember> ssmbr;
         IDType ssmbrID = ::GetSuperstructureMemberID(girderKey.groupIndex,girderKey.girderIndex);
         m_Bridge->CreateSuperstructureMember(ssmbrID,locationType,&ssmbr);

         SegmentIndexType nSegments = pGirder->GetSegmentCount();

         // first layout all segments in this girder (this connects the segments
         // together so the overall girder line geometry is correct).
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey segmentKey(girderKey,segIdx);

            // Let beam factory build segments as necessary to represent the beam
            // this includes setting the cross section
            beamFactory->CreateSegment(m_pBroker,m_StatusGroupID,segmentKey,ssmbr);

            // associate girder line with segment so segment has plan view geometry
            LineIDType girderLineID = ::GetGirderSegmentLineID(segmentKey);
            CComPtr<IGirderLine> girderLine;
            geometry->FindGirderLine(girderLineID,&girderLine);

            CComPtr<ISuperstructureMemberSegment> segment;
            ssmbr->get_Segment(segIdx,&segment);
            segment->putref_GirderLine(girderLine);
         }

         // WARNING: You will be tempted to combine this loop with the one above. Don't do it.
         // This loop requires all the segments to be connected together in the bridge model.
         // That is what the loop above does when the beam factory creates the girder line layout.
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CComPtr<ISuperstructureMemberSegment> segment;
            ssmbr->get_Segment(segIdx,&segment);

            CComPtr<IGirderLine> girderLine;
            segment->get_GirderLine(&girderLine);

            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
            CSegmentKey segmentKey(pSegment->GetSegmentKey());

            Float64 Lseg;
            segment->get_Length(&Lseg);


            Float64 startHaunch = 0;
            Float64 endHaunch   = 0;
            if ( deckType != pgsTypes::sdtNone )
            {
               GetHaunchDepth(pSegment,&startHaunch,&endHaunch);
            }

            segment->put_HaunchDepth(etStart,startHaunch);
            segment->put_HaunchDepth(etEnd,  endHaunch);

            // if these fire, something is really messed up
            ATLASSERT( 0 <= startHaunch && startHaunch < 1e9 );
            ATLASSERT( 0 <= endHaunch   && endHaunch   < 1e9 );

            // create the strand mover
            pgsTypes::AdjustableStrandType adjType = pSegment->Strands.GetAdjustableStrandType();
            CComPtr<IStrandMover> strand_mover;
            CreateStrandMover(pGirderEntry->GetName().c_str(),adjType,&strand_mover);
            ATLASSERT(strand_mover != NULL);
            
            // assign a precast girder model to the segment
            CComPtr<IPrecastGirder> girder;
            HRESULT hr = girder.CoCreateInstance(CLSID_PrecastGirder);
            if ( FAILED(hr) || girder == NULL )
            {
               THROW_UNWIND(_T("Precast girder object not created"),-1);
            }

            girder->Initialize(segment,strand_mover);
            CComQIPtr<IItemData> item_data(segment);
            item_data->AddItemData(CComBSTR(_T("Precast Girder")),girder);

            // Create strand material
            IntervalIndexType stressStrandIntervalIdx = m_IntervalManager.GetStressStrandInterval(segmentKey);

            const matPsStrand* pStrand = pSegment->Strands.GetStrandMaterial(pgsTypes::Straight);
            CComPtr<IPrestressingStrand> straightStrandMaterial;
            straightStrandMaterial.CoCreateInstance(CLSID_PrestressingStrand);
            straightStrandMaterial->put_Name( CComBSTR(pStrand->GetName().c_str()) );
            straightStrandMaterial->put_Grade((StrandGrade)pStrand->GetGrade());
            straightStrandMaterial->put_Type((StrandType)pStrand->GetType());
            straightStrandMaterial->put_Size((StrandSize)pStrand->GetSize());
            straightStrandMaterial->put_InstallationStage(stressStrandIntervalIdx);
            girder->putref_StraightStrandMaterial(straightStrandMaterial);

            pStrand = pSegment->Strands.GetStrandMaterial(pgsTypes::Harped);
            CComPtr<IPrestressingStrand> harpedStrandMaterial;
            harpedStrandMaterial.CoCreateInstance(CLSID_PrestressingStrand);
            harpedStrandMaterial->put_Name( CComBSTR(pStrand->GetName().c_str()) );
            harpedStrandMaterial->put_Grade((StrandGrade)pStrand->GetGrade());
            harpedStrandMaterial->put_Type((StrandType)pStrand->GetType());
            harpedStrandMaterial->put_Size((StrandSize)pStrand->GetSize());
            harpedStrandMaterial->put_InstallationStage(stressStrandIntervalIdx);
            girder->putref_HarpedStrandMaterial(harpedStrandMaterial);

            pStrand = pSegment->Strands.GetStrandMaterial(pgsTypes::Temporary);
            CComPtr<IPrestressingStrand> temporaryStrandMaterial;
            temporaryStrandMaterial.CoCreateInstance(CLSID_PrestressingStrand);
            temporaryStrandMaterial->put_Name( CComBSTR(pStrand->GetName().c_str()) );
            temporaryStrandMaterial->put_Grade((StrandGrade)pStrand->GetGrade());
            temporaryStrandMaterial->put_Type((StrandType)pStrand->GetType());
            temporaryStrandMaterial->put_Size((StrandSize)pStrand->GetSize());
            temporaryStrandMaterial->put_InstallationStage(stressStrandIntervalIdx);
            girder->putref_TemporaryStrandMaterial(temporaryStrandMaterial);

            // layout the harping points
            girder->put_AllowOddNumberOfHarpedStrands(pGirderEntry->OddNumberOfHarpedStrands() ? VARIANT_TRUE : VARIANT_FALSE);
            girder->put_UseDifferentHarpedGridsAtEnds(pGirderEntry->IsDifferentHarpedGridAtEndsUsed() ? VARIANT_TRUE : VARIANT_FALSE);

            GirderLibraryEntry::MeasurementLocation hpref = pGirderEntry->GetHarpingPointReference();
            GirderLibraryEntry::MeasurementType hpmeasure = pGirderEntry->GetHarpingPointMeasure();
            Float64 hpLoc = pGirderEntry->GetHarpingPointLocation();

            girder->put_UseMinHarpPointDistance( pGirderEntry->IsMinHarpingPointLocationUsed() ? VARIANT_TRUE : VARIANT_FALSE);
            girder->put_MinHarpPointDistance( pGirderEntry->GetMinHarpingPointLocation() );

            girder->put_HarpingPointReference( HarpPointReference(hpref) );
            girder->put_HarpingPointMeasure( HarpPointMeasure(hpmeasure) );
            girder->SetHarpingPoints(hpLoc,hpLoc);
            
            // For this strand definition type, the end harp points are always located at the end faces of the girder
            girder->put_EndHarpingPointReference( hprEndOfGirder );
            girder->put_EndHarpingPointMeasure( hpmFractionOfGirderLength );
            girder->SetEndHarpingPoints(0.0,0.0);

            // Get height of girder section... going to needed this for
            // filling up strand patterns...
            //
            // the calls to get_Section below need locations in Segment Coordinates (coordinates
            // measured from CL Pier/TS at start of segment)
            Float64 XsStart = 0;    // start face of segment
            Float64 XsEnd   = Lseg; // end face of segment

            Float64 XsHp1Loc,XsHp2Loc;
            girder->GetHarpingPointLocations(&XsHp1Loc,&XsHp2Loc);

            IntervalIndexType releaseIntervalIdx = m_IntervalManager.GetPrestressReleaseInterval(segmentKey);

            CComPtr<IShape> startShape, hp1Shape, hp2Shape, endShape;
            segment->get_PrimaryShape(XsStart,  &startShape);
            segment->get_PrimaryShape(XsHp1Loc, &hp1Shape);
            segment->get_PrimaryShape(XsHp2Loc, &hp2Shape);
            segment->get_PrimaryShape(XsEnd,    &endShape);

            // bounding boxes of the section (height of section is height of bounding box)
            CComPtr<IRect2d> bbStart, bbHP1, bbHP2, bbEnd;
            startShape->get_BoundingBox(&bbStart);
            hp1Shape->get_BoundingBox(&bbHP1);
            hp2Shape->get_BoundingBox(&bbHP2);
            endShape->get_BoundingBox(&bbEnd);

            Float64 HgStart, HgHP1, HgHP2, HgEnd;
            bbStart->get_Height(&HgStart);
            bbHP1->get_Height(&HgHP1);
            bbHP2->get_Height(&HgHP2);
            bbEnd->get_Height(&HgEnd);

            // Fill up strand patterns
            CComPtr<IStrandGrid> strGrd[2], harpGrdEnd[2], harpGrdHP[2], tempGrd[2];
            girder->get_StraightStrandGrid(etStart,&strGrd[etStart]);
            girder->get_HarpedStrandGridEnd(etStart,&harpGrdEnd[etStart]);
            girder->get_HarpedStrandGridHP(etStart,&harpGrdHP[etStart]);
            girder->get_TemporaryStrandGrid(etStart,&tempGrd[etStart]);

            girder->get_StraightStrandGrid(etEnd,&strGrd[etEnd]);
            girder->get_HarpedStrandGridEnd(etEnd,&harpGrdEnd[etEnd]);
            girder->get_HarpedStrandGridHP(etEnd,&harpGrdHP[etEnd]);
            girder->get_TemporaryStrandGrid(etEnd,&tempGrd[etEnd]);

            if ( pSegment->Strands.GetStrandDefinitionType() == CStrandData::sdtDirectInput )
            {
               // Not using the strand grid in the library... create the strand grid based on user input
               const CStrandRowCollection& strandRows = pSegment->Strands.GetStrandRows();
               CStrandRowCollection::const_iterator iter(strandRows.begin());
               CStrandRowCollection::const_iterator iterEnd(strandRows.end());
               for ( ; iter != iterEnd; iter++ )
               {
                  Float64 segmentLength = GetSegmentLength(segmentKey);

                  const CStrandRow& strandRow(*iter);
                  GridIndexType nGridPoints = strandRow.m_nStrands/2; // strand grid is only half the full grid (just the grid on the positive X side)
                  Float64 Xi = strandRow.m_InnerSpacing/2; // distance from CL Girder to first strand
                  ATLASSERT(IsOdd(strandRow.m_nStrands) ? IsZero(Xi) : !IsZero(Xi));

                  Float64 Z[4], Y[4];
                  for ( int i = 0; i < 4; i++ )
                  {
                     Z[i] = strandRow.m_X[i];
                     if ( Z[i] < 0 )
                     {
                        // fractional measure
                        Z[i] *= -1*segmentLength;
                     }

                     Y[i] = strandRow.m_Y[i];

                     if ( strandRow.m_Face[i] == pgsTypes::TopFace )
                     {
                        // measured down from top of girder... this is negative in Girder Section Coordinates
                        Y[i] *= -1;
                     }
                     else
                     {
                        // adjust to be measured from top of girder
                        CComPtr<IShape> shape;
                        segment->get_PrimaryShape(Z[i], &shape);

                        // bounding boxes of the section (height of section is height of bounding box)
                        CComPtr<IRect2d> box;
                        shape->get_BoundingBox(&box);

                        Float64 Hg;
                        box->get_Height(&Hg);

                        Y[i] -= Hg;
                     }
                  }

                  for ( GridIndexType gridPointIdx = 0; gridPointIdx < nGridPoints; gridPointIdx++ )
                  {
                     Float64 X = Xi + gridPointIdx*strandRow.m_Spacing;

                     CComPtr<IPoint2d> pntStart;
                     pntStart.CoCreateInstance(CLSID_Point2d);
                     pntStart->Move(X,Y[LOCATION_START]);

                     CComPtr<IPoint2d> pntLeftHP;
                     pntLeftHP.CoCreateInstance(CLSID_Point2d);
                     pntLeftHP->Move(X,Y[LOCATION_LEFT_HP]);

                     CComPtr<IPoint2d> pntRightHP;
                     pntRightHP.CoCreateInstance(CLSID_Point2d);
                     pntRightHP->Move(X,Y[LOCATION_RIGHT_HP]);

                     CComPtr<IPoint2d> pntEnd;
                     pntEnd.CoCreateInstance(CLSID_Point2d);
                     pntEnd->Move(X,Y[LOCATION_END]);

                     switch(strandRow.m_StrandType)
                     {
                     case pgsTypes::Straight:
                        strGrd[etStart]->AddGridPoint(pntStart);
                        strGrd[etEnd]->AddGridPoint(pntEnd);
                        break;

                     case pgsTypes::Harped:
                        girder->SetEndHarpingPoints(Z[LOCATION_START],segmentLength - Z[LOCATION_END]);
                        girder->SetHarpingPoints(Z[LOCATION_LEFT_HP],segmentLength - Z[LOCATION_RIGHT_HP]);
                        girder->put_HarpingPointMeasure(hpmAbsoluteDistance);
                        girder->put_HarpingPointReference(hprEndOfGirder);
                        girder->put_EndHarpingPointMeasure(hpmAbsoluteDistance);
                        girder->put_EndHarpingPointReference(hprEndOfGirder);

                        harpGrdEnd[etStart]->AddGridPoint(pntStart);
                        harpGrdEnd[etEnd]->AddGridPoint(pntEnd);

                        harpGrdHP[etStart]->AddGridPoint(pntLeftHP);
                        harpGrdHP[etEnd]->AddGridPoint(pntRightHP);

                        break;
                     
                     case pgsTypes::Temporary:
                        tempGrd[etStart]->AddGridPoint(pntStart);
                        tempGrd[etEnd]->AddGridPoint(pntEnd);
                        break;

                     default:
                        ATLASSERT(false); // should never get here
                     }
                  }
               }
            }
            else
            {
               // not user defined
               pGirderEntry->ConfigureStraightStrandGrid(HgStart,HgEnd,strGrd[etStart],strGrd[etEnd]);
               pGirderEntry->ConfigureHarpedStrandGrids(HgStart,HgHP1,HgHP2,HgEnd,harpGrdEnd[etStart], harpGrdHP[etStart], harpGrdHP[etEnd], harpGrdEnd[etEnd]);
               pGirderEntry->ConfigureTemporaryStrandGrid(HgStart,HgEnd,tempGrd[etStart],tempGrd[etEnd]);
            }
         } // segment loop

         // add a tendon collection to the superstructure member... spliced girders know how to use it
         CComQIPtr<IItemData> ssmbrItemData(ssmbr);
         CComPtr<ITendonCollection> tendons;
         CreateTendons(pBridgeDesc,girderKey,ssmbr,&tendons);
         ssmbrItemData->AddItemData(CComBSTR(_T("Tendons")),tendons);
      } // girder loop

      // now that all the girders are layed out, we can layout the rebar
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CGirderKey girderKey(grpIdx,gdrIdx);

         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);

         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey segmentKey(girderKey.groupIndex,girderKey.girderIndex,segIdx);

            LayoutSegmentRebar(segmentKey);

            if ( segIdx != nSegments-1 )
            {
               LayoutClosureJointRebar(segmentKey);
            }
         } // segment loop
      } // girder loop
   } // group loop
   return true;
}

void CBridgeAgentImp::GetHaunchDepth(const CPrecastSegmentData* pSegment,Float64* pStartHaunch,Float64* pEndHaunch)
{
   // The generic bridge model wants haunch depths at the end of each segment
   const CSegmentKey& segmentKey = pSegment->GetSegmentKey();
   const CDeckDescription2* pDeck = pSegment->GetGirder()->GetGirderGroup()->GetBridgeDescription()->GetDeckDescription();
   Float64 tDeck;
   if ( pDeck->DeckType == pgsTypes::sdtCompositeSIP )
   {
      tDeck = pDeck->GrossDepth + pDeck->PanelDepth;
   }
   else
   {
      tDeck = pDeck->GrossDepth;
   }

   const CPierData2* pPier[2];
   const CTemporarySupportData* pTS[2];
   pSegment->GetSupport(pgsTypes::metStart,&pPier[pgsTypes::metStart],&pTS[pgsTypes::metStart]);
   pSegment->GetSupport(pgsTypes::metEnd  ,&pPier[pgsTypes::metEnd],  &pTS[pgsTypes::metEnd]  );

   const CSpanData2* pSpan[2];
   pSpan[pgsTypes::metStart] = pSegment->GetSpan(pgsTypes::metStart);
   pSpan[pgsTypes::metEnd  ] = pSegment->GetSpan(pgsTypes::metEnd);

   Float64 haunch[2];

   // loop over the two spans where the segment starts and ends
   // i = 0, computing haunch at the start of the segment which occurs
   // in the start span
   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::MemberEndType end = pgsTypes::MemberEndType(i);

      // get the slab offset at the start of the span
      const CPierData2* pStartPier = pSpan[end]->GetPier(pgsTypes::metStart);
      const CGirderGroupData* pStartGroup = pStartPier->GetGirderGroup(pgsTypes::Ahead);
      Float64 startSlabOffset = pStartGroup->GetSlabOffset(pStartPier->GetIndex(),segmentKey.girderIndex);

      // get the slab offset at the end of the span
      const CPierData2* pEndPier = pSpan[end]->GetPier(pgsTypes::metEnd);
      const CGirderGroupData* pEndGroup = pEndPier->GetGirderGroup(pgsTypes::Back);
      Float64 endSlabOffset = pEndGroup->GetSlabOffset(pEndPier->GetIndex(),segmentKey.girderIndex);

      Float64 startStation = pStartPier->GetStation();
      Float64 endStation   = pEndPier->GetStation();
      Float64 spanLength = endStation - startStation;

      if ( pPier[end] )
      {
         // segment is supported by a pier
         haunch[end] = (end == pgsTypes::metStart ? startSlabOffset : endSlabOffset) - tDeck;
      }
      else
      {
         // segment is supported by a temporary support
         ATLASSERT(pTS[end] != NULL);
         Float64 elevAdjustment = pTS[end]->GetElevationAdjustment();
         Float64 tsStation = pTS[end]->GetStation();
         Float64 dist = tsStation - startStation;
         haunch[end] = ::LinInterp(dist,startSlabOffset,endSlabOffset,spanLength) - tDeck - elevAdjustment;
      }
   }

   *pStartHaunch = haunch[pgsTypes::metStart];
   *pEndHaunch   = haunch[pgsTypes::metEnd];
}

bool CBridgeAgentImp::LayoutDeck(const CBridgeDescription2* pBridgeDesc)
{
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();
   pgsTypes::SupportedDeckType deckType = pDeck->DeckType;

   CGirderKey girderKey(0,0);
   IntervalIndexType nIntervals = m_IntervalManager.GetIntervalCount();
   IntervalIndexType lastIntervalIdx = nIntervals-1;
   IntervalIndexType overlayIntervalIdx = m_IntervalManager.GetOverlayInterval();
   IntervalIndexType liveLoadIntervalIdx = m_IntervalManager.GetLiveLoadInterval();


   // put the wearing surface into the generic bridge model
   if ( pDeck->WearingSurface == pgsTypes::wstSacrificialDepth || 
        pDeck->WearingSurface == pgsTypes::wstFutureOverlay ||
        (pDeck->WearingSurface == pgsTypes::wstOverlay && liveLoadIntervalIdx < overlayIntervalIdx) // when live load comes before the overlay, the overlay is treated as a "future" overlay
      )
   {
      m_Bridge->put_SacrificialDepth(pDeck->SacrificialDepth);
      m_Bridge->put_SacrificialDepthStage(lastIntervalIdx);
   }
   else
   {
      m_Bridge->put_SacrificialDepth(0);
      m_Bridge->put_SacrificialDepthStage(INVALID_INDEX);
   }

   if ( pDeck->WearingSurface == pgsTypes::wstOverlay || pDeck->WearingSurface == pgsTypes::wstFutureOverlay )
   {
      if ( pDeck->bInputAsDepthAndDensity )
      {
         m_Bridge->put_WearingSurfaceDepth(pDeck->OverlayDepth);
         m_Bridge->put_WearingSurfaceDensity(pDeck->OverlayDensity);
      }
      else
      {
         // depth not explicitly input... estimate based on 140 pcf material
         Float64 density = ::ConvertToSysUnits(140.0,unitMeasure::LbfPerFeet3);
         m_Bridge->put_WearingSurfaceDensity(density);
         Float64 g = unitSysUnitsMgr::GetGravitationalAcceleration();
         Float64 depth = (pDeck->OverlayWeight/g) / density;
         m_Bridge->put_WearingSurfaceDepth(depth);
      }

      m_Bridge->put_WearingSurfaceStage(overlayIntervalIdx);
   }
   else
   {
      m_Bridge->put_WearingSurfaceDepth(0);
      m_Bridge->put_WearingSurfaceDensity(0);
      m_Bridge->put_WearingSurfaceStage(INVALID_INDEX);
   }

   CComPtr<IBridgeDeck> deck;

   if ( IsAdjustableWidthDeck(deckType) )
   {
      if ( pDeck->DeckEdgePoints.size() == 1 )
      {
         LayoutSimpleDeck(pBridgeDesc,&deck);
      }
      else
      {
         LayoutFullDeck(pBridgeDesc,&deck);
      }
   }
   else
   {
      LayoutNoDeck(pBridgeDesc,&deck);
   }

   if ( deck )
   {
      LayoutDeckRebar(pDeck,deck);

      // put the deck in the bridge model before creating the deck material
      // for time-step analysis we need age adjusted concrete it it depends on 
      // deck geometry parameters (V/S). The bridge model needs the deck to compute
      // these parameters.
      m_Bridge->putref_Deck(deck);

      CComPtr<IMaterial> deck_material;

      GET_IFACE(ILossParameters,pLossParams);
      bool bTimeStepAnalysis = (pLossParams->GetLossMethod() == pgsTypes::TIME_STEP ? true : false);

      if ( bTimeStepAnalysis )
      {
         GET_IFACE(IMaterials,pMaterials);
         CComPtr<IAgeAdjustedMaterial> age_adjusted_material;
         HRESULT hr = age_adjusted_material.CoCreateInstance(CLSID_AgeAdjustedMaterial);
         ATLASSERT(SUCCEEDED(hr));

         age_adjusted_material->InitDeck(girderKey,pMaterials);

         age_adjusted_material.QueryInterface(&deck_material);
      }
      else
      {
         IntervalIndexType compositeDeckIntervalIdx = m_IntervalManager.GetCompositeDeckInterval();
         deck_material.CoCreateInstance(CLSID_Material);
         IntervalIndexType nIntervals = m_IntervalManager.GetIntervalCount();
         for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
         {
            Float64 E(0), D(0);
            if ( compositeDeckIntervalIdx <= intervalIdx )
            {
               E = GetDeckEc(intervalIdx);
               D = GetDeckWeightDensity(intervalIdx);
            }

            deck_material->put_E(intervalIdx,E);
            deck_material->put_Density(intervalIdx,D);
         }
      }
 
      deck->putref_Material(deck_material);
   }


   return true;
}

void CBridgeAgentImp::NoDeckEdgePoint(GroupIndexType grpIdx,SegmentIndexType segIdx,pgsTypes::MemberEndType end,DirectionType side,IPoint2d** ppPoint)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);

   GirderIndexType gdrIdx;
   if ( side == qcbLeft )
   {
      gdrIdx = 0;
   }
   else
   {
      gdrIdx = pGroup->GetGirderCount()-1;
   }

   CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

   Float64 location;
   if ( end == pgsTypes::metStart )
   {
      location = 0.0;
   }
   else
   {
      location = GetSegmentLength(segmentKey);
   }

   CComPtr<IGirderSection> girder_section;
   GetGirderSection(pgsPointOfInterest(segmentKey,location),pgsTypes::scBridge,&girder_section);
   Float64 width;
   girder_section->get_TopWidth(&width);
   width /= 2;

   CComPtr<ISuperstructureMember> ssmbr;
   HRESULT hr = m_Bridge->get_SuperstructureMember(::GetSuperstructureMemberID(grpIdx,gdrIdx),&ssmbr);
   ATLASSERT(SUCCEEDED(hr));

   CComPtr<ISuperstructureMemberSegment> segment;
   hr = ssmbr->get_Segment(segmentKey.segmentIndex,&segment);
   ATLASSERT(SUCCEEDED(hr));

   CComPtr<IGirderLine> girderLine;
   hr = segment->get_GirderLine(&girderLine);
   ATLASSERT(SUCCEEDED(hr));

   CComPtr<IPoint2d> point_on_girder;
   hr = girderLine->get_PierPoint(end == pgsTypes::metStart ? etStart : etEnd,&point_on_girder);
   ATLASSERT(SUCCEEDED(hr));

   if ( side == qcbRight )
   {
      width *= -1;
   }

   CComPtr<IDirection> girder_direction;
   girderLine->get_Direction(&girder_direction); // bearing of girder

   CComPtr<IDirection> normal;
   girder_direction->Increment(CComVariant(PI_OVER_2),&normal);// rotate 90 to make a normal to the girder

   CComQIPtr<ILocate2> locate(m_CogoEngine);
   CComPtr<IPoint2d> point_on_edge;
   hr = locate->ByDistDir(point_on_girder,width,CComVariant(normal),0.00,&point_on_edge);
   ATLASSERT(SUCCEEDED(hr));

   (*ppPoint) = point_on_edge;
   (*ppPoint)->AddRef();
}

bool CBridgeAgentImp::LayoutNoDeck(const CBridgeDescription2* pBridgeDesc,IBridgeDeck** ppDeck)
{
   // There isn't an explicit deck in this case, so layout of the composite slab must be done
   // based on the girder geometry. Update the bridge model now so that the girder geometry is correct
   m_Bridge->UpdateBridgeModel();

   CComPtr<IBridgeGeometry> bridgeGeometry;
   m_Bridge->get_BridgeGeometry(&bridgeGeometry);

   CogoObjectID alignmentID;
   bridgeGeometry->get_BridgeAlignmentID(&alignmentID);

   HRESULT hr = S_OK;
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();
   pgsTypes::SupportedDeckType deckType = pDeck->DeckType;

   if ( deckType == pgsTypes::sdtNone )
   {
      (*ppDeck) = NULL;
      return true;
   }

   ATLASSERT( deckType == pgsTypes::sdtCompositeOverlay );

   // create path objects that represent the deck edges
   CComPtr<IPath> left_path, right_path;
   CreateCompositeOverlayEdgePaths(pBridgeDesc,&left_path,&right_path);

   // store the deck edge paths as layout lines in the bridge geometry model
   CComPtr<ISimpleLayoutLineFactory> layoutLineFactory;
   layoutLineFactory.CoCreateInstance(CLSID_SimpleLayoutLineFactory);
   layoutLineFactory->AddPath(LEFT_DECK_EDGE_LAYOUT_LINE_ID,left_path);
   layoutLineFactory->AddPath(RIGHT_DECK_EDGE_LAYOUT_LINE_ID,right_path);
   bridgeGeometry->CreateLayoutLines(layoutLineFactory);

   // create the deck boundary
   CComPtr<ISimpleDeckBoundaryFactory> deckBoundaryFactory;
   deckBoundaryFactory.CoCreateInstance(CLSID_SimpleDeckBoundaryFactory);
   deckBoundaryFactory->put_TransverseEdgeID( etStart, GetPierLineID(0) );
   deckBoundaryFactory->put_TransverseEdgeType(etStart, setPier);
   deckBoundaryFactory->put_TransverseEdgeID( etEnd, GetPierLineID(pBridgeDesc->GetPierCount()-1) ); 
   deckBoundaryFactory->put_TransverseEdgeType(etEnd, setPier);
   deckBoundaryFactory->put_EdgeID(stLeft, LEFT_DECK_EDGE_LAYOUT_LINE_ID); 
   deckBoundaryFactory->put_EdgeID(stRight, RIGHT_DECK_EDGE_LAYOUT_LINE_ID);

   bridgeGeometry->CreateDeckBoundary(deckBoundaryFactory); 

   // get the deck boundary
   CComPtr<IDeckBoundary> deckBoundary;
   bridgeGeometry->get_DeckBoundary(&deckBoundary);

   // create the specific deck type
   CComPtr<IOverlaySlab> slab;
   slab.CoCreateInstance(CLSID_OverlaySlab);

   slab->put_GrossDepth(pDeck->GrossDepth);

   slab.QueryInterface(ppDeck);

   (*ppDeck)->put_Composite( deckType == pgsTypes::sdtCompositeOverlay ? VARIANT_TRUE : VARIANT_FALSE );
   (*ppDeck)->putref_DeckBoundary(deckBoundary);


   return true;
}

bool CBridgeAgentImp::LayoutSimpleDeck(const CBridgeDescription2* pBridgeDesc,IBridgeDeck** ppDeck)
{
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();
   pgsTypes::SupportedDeckType deckType = pDeck->DeckType;

   ATLASSERT( deckType == pgsTypes::sdtCompositeSIP || deckType == pgsTypes::sdtCompositeCIP );
   ATLASSERT( pDeck->DeckEdgePoints.size() == 1 );

   Float64 alignment_offset = pBridgeDesc->GetAlignmentOffset();

   // this is a simple deck edge that parallels the alignment
   CDeckPoint deckPoint = *(pDeck->DeckEdgePoints.begin());

   Float64 left_offset;
   Float64 right_offset;

   if ( deckPoint.MeasurementType == pgsTypes::omtAlignment )
   {
      // deck edge is measured from the alignment
      left_offset  =  deckPoint.LeftEdge;
      right_offset = -deckPoint.RightEdge;
   }
   else
   {
      // deck edge is measured from the CL bridge. compute the offsets from the alignment
      left_offset  = -alignment_offset + deckPoint.LeftEdge;
      right_offset = -alignment_offset - deckPoint.RightEdge;
   }

   //
   // Create slab model in new BridgeGeometry sub-system
   //
   CComPtr<IBridgeGeometry> bridgeGeometry;
   m_Bridge->get_BridgeGeometry(&bridgeGeometry);

   PierIndexType nPiers = pBridgeDesc->GetPierCount();

   CogoObjectID alignmentID;
   bridgeGeometry->get_BridgeAlignmentID(&alignmentID);

   // Create slab edge paths
   CComPtr<IAlignmentOffsetLayoutLineFactory> factory;
   factory.CoCreateInstance(CLSID_AlignmentOffsetLayoutLineFactory);
   factory->put_AlignmentID(alignmentID);
   factory->put_LayoutLineID(LEFT_DECK_EDGE_LAYOUT_LINE_ID); // left edge
   factory->put_Offset(left_offset);
   bridgeGeometry->CreateLayoutLines(factory);

   factory->put_LayoutLineID(RIGHT_DECK_EDGE_LAYOUT_LINE_ID); // right edge
   factory->put_Offset(right_offset);
   bridgeGeometry->CreateLayoutLines(factory);

   CComPtr<ISimpleDeckBoundaryFactory> deckBoundaryFactory;
   deckBoundaryFactory.CoCreateInstance(CLSID_SimpleDeckBoundaryFactory);
   deckBoundaryFactory->put_TransverseEdgeID( etStart, GetPierLineID(0) );
   deckBoundaryFactory->put_TransverseEdgeType(etStart, setPier);
   deckBoundaryFactory->put_TransverseEdgeID( etEnd, GetPierLineID(nPiers-1) ); 
   deckBoundaryFactory->put_TransverseEdgeType(etEnd, setPier);
   deckBoundaryFactory->put_EdgeID(stLeft,LEFT_DECK_EDGE_LAYOUT_LINE_ID); 
   deckBoundaryFactory->put_EdgeID(stRight,RIGHT_DECK_EDGE_LAYOUT_LINE_ID);

//   // this is a fake slab break just for testing and an example on how to model it
//   // when we actually use it
//   factory->put_Offset(left_offset-0.5);
//   factory->put_LayoutLineID(-1001); // left edge
//   bridgeGeometry->CreateLayoutLines(factory);
//   factory->put_LayoutLineID(-2001); // right edge
//   factory->put_Offset(right_offset+0.5);
//   bridgeGeometry->CreateLayoutLines(factory);
//   deckBoundaryFactory->put_BreakEdge(etStart,stLeft,VARIANT_TRUE);
//   deckBoundaryFactory->put_BreakEdge(etEnd,  stLeft, VARIANT_TRUE);
//   deckBoundaryFactory->put_BreakEdge(etStart,stRight,VARIANT_TRUE);
//   deckBoundaryFactory->put_BreakEdge(etEnd,  stRight,VARIANT_TRUE);
//   deckBoundaryFactory->put_EdgeBreakID(stLeft,-1001);
//   deckBoundaryFactory->put_EdgeBreakID(stRight,-2001);

   bridgeGeometry->CreateDeckBoundary(deckBoundaryFactory); 

   CComPtr<IDeckBoundary> deckBoundary;
   bridgeGeometry->get_DeckBoundary(&deckBoundary);

   if ( deckType == pgsTypes::sdtCompositeCIP )
   {
      LayoutCompositeCIPDeck(pDeck,deckBoundary,ppDeck);
   }
   else if ( deckType == pgsTypes::sdtCompositeSIP )
   {
      LayoutCompositeSIPDeck(pDeck,deckBoundary,ppDeck);
   }
   else
   {
      ATLASSERT(false); // shouldn't get here
   }

   return true;
}

bool CBridgeAgentImp::LayoutFullDeck(const CBridgeDescription2* pBridgeDesc,IBridgeDeck** ppDeck)
{
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();
   pgsTypes::SupportedDeckType deckType = pDeck->DeckType;

   ATLASSERT( deckType == pgsTypes::sdtCompositeSIP || deckType == pgsTypes::sdtCompositeCIP );
   ATLASSERT( 1 < pDeck->DeckEdgePoints.size() );

   // the deck edge is described by a series of points. the transitions
   // between the points can be parallel to alignment, linear or spline. 
   // for spline transitions a cubic spline will be used. 
   // for parallel transitations, a parallel alignment sub-path will be used.
   CComPtr<IAlignment> alignment;
   m_Bridge->get_Alignment(&alignment);

   CComPtr<IBridgeGeometry> geometry;
   m_Bridge->get_BridgeGeometry(&geometry);

   // create the path objects for the edge of deck
   CComPtr<IPath> left_path, right_path;
   CDeckEdgeBuilder deck_edge_builder;
   deck_edge_builder.BuildDeckEdges(pBridgeDesc,m_CogoEngine,alignment,&left_path,&right_path);

   // store the deck edge paths as layout lines in the bridge geometry model
   CComPtr<ISimpleLayoutLineFactory> layoutLineFactory;
   layoutLineFactory.CoCreateInstance(CLSID_SimpleLayoutLineFactory);
   layoutLineFactory->AddPath(LEFT_DECK_EDGE_LAYOUT_LINE_ID,left_path);
   layoutLineFactory->AddPath(RIGHT_DECK_EDGE_LAYOUT_LINE_ID,right_path);
   geometry->CreateLayoutLines(layoutLineFactory);

   // create the bridge deck boundary
   CComPtr<ISimpleDeckBoundaryFactory> deckBoundaryFactory;
   deckBoundaryFactory.CoCreateInstance(CLSID_SimpleDeckBoundaryFactory);
   deckBoundaryFactory->put_TransverseEdgeID(etStart,::GetPierLineID(0));
   deckBoundaryFactory->put_TransverseEdgeType(etStart,setPier);
   deckBoundaryFactory->put_TransverseEdgeID(etEnd,::GetPierLineID(pBridgeDesc->GetPierCount()-1));
   deckBoundaryFactory->put_TransverseEdgeType(etEnd,setPier);
   deckBoundaryFactory->put_EdgeID(stLeft,LEFT_DECK_EDGE_LAYOUT_LINE_ID);
   deckBoundaryFactory->put_EdgeID(stRight,RIGHT_DECK_EDGE_LAYOUT_LINE_ID);
   geometry->CreateDeckBoundary(deckBoundaryFactory);

   // get the deck boundary
   CComPtr<IDeckBoundary> deckBoundary;
   geometry->get_DeckBoundary(&deckBoundary);

   if ( deckType == pgsTypes::sdtCompositeCIP )
   {
      LayoutCompositeCIPDeck(pDeck,deckBoundary,ppDeck);
   }
   else if ( deckType == pgsTypes::sdtCompositeSIP )
   {
      LayoutCompositeSIPDeck(pDeck,deckBoundary,ppDeck);
   }
   else
   {
      ATLASSERT(false); // shouldn't get here
   }

   return true;
}

bool CBridgeAgentImp::LayoutCompositeCIPDeck(const CDeckDescription2* pDeck,IDeckBoundary* pBoundary,IBridgeDeck** ppDeck)
{
   CComPtr<ICastSlab> slab;
   slab.CoCreateInstance(CLSID_CastSlab);

   slab->put_Fillet(pDeck->Fillet);
   slab->put_GrossDepth(pDeck->GrossDepth);
   slab->put_OverhangDepth(pDeck->OverhangEdgeDepth);
   slab->put_OverhangTaper((DeckOverhangTaper)pDeck->OverhangTaper);

   slab.QueryInterface(ppDeck);

   (*ppDeck)->put_Composite( VARIANT_TRUE );
   (*ppDeck)->putref_DeckBoundary(pBoundary);

   return true;
}

bool CBridgeAgentImp::LayoutCompositeSIPDeck(const CDeckDescription2* pDeck,IDeckBoundary* pBoundary,IBridgeDeck** ppDeck)
{
   CComPtr<IPrecastSlab> slab;
   slab.CoCreateInstance(CLSID_PrecastSlab);

   slab->put_Fillet(pDeck->Fillet);
   slab->put_PanelDepth(pDeck->PanelDepth);
   slab->put_CastDepth(pDeck->GrossDepth); // interpreted as cast depth
   slab->put_OverhangDepth(pDeck->OverhangEdgeDepth);
   slab->put_OverhangTaper((DeckOverhangTaper)pDeck->OverhangTaper);

   slab.QueryInterface(ppDeck);

   (*ppDeck)->put_Composite( VARIANT_TRUE );
   (*ppDeck)->putref_DeckBoundary(pBoundary);

   return true;
}

bool CBridgeAgentImp::LayoutTrafficBarriers(const CBridgeDescription2* pBridgeDesc)
{
   CComPtr<ISidewalkBarrier> lb;
   const CRailingSystem* pLeftRailingSystem  = pBridgeDesc->GetLeftRailingSystem();
   LayoutTrafficBarrier(pBridgeDesc,pLeftRailingSystem,pgsTypes::tboLeft,&lb);
   m_Bridge->putref_LeftBarrier(lb);

   CComPtr<ISidewalkBarrier> rb;
   const CRailingSystem* pRightRailingSystem = pBridgeDesc->GetRightRailingSystem();
   LayoutTrafficBarrier(pBridgeDesc,pRightRailingSystem,pgsTypes::tboRight,&rb);
   m_Bridge->putref_RightBarrier(rb);

   CComPtr<IPath> leftPath, rightPath;
   if ( pBridgeDesc->GetDeckDescription()->DeckType == pgsTypes::sdtNone )
   {
      CreateCompositeOverlayEdgePaths(pBridgeDesc,&leftPath,&rightPath);
   }
   else
   {
      CComPtr<IBridgeDeck> deck;
      m_Bridge->get_Deck(&deck);

      CComPtr<IDeckBoundary> deckBoundary;
      deck->get_DeckBoundary(&deckBoundary);

      deckBoundary->get_EdgePath(stLeft,VARIANT_TRUE,&leftPath);
      deckBoundary->get_EdgePath(stRight,VARIANT_TRUE,&rightPath);
   }
   lb->put_Path(leftPath);
   rb->put_Path(rightPath);

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

   CComPtr<IMaterial> material;
   (*pBarrier)->get_Material(&material);

   IntervalIndexType installRailingSystemIntervalIdx = m_IntervalManager.GetInstallRailingSystemInterval();
   IntervalIndexType nIntervals = m_IntervalManager.GetIntervalCount();
   for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
   {
      Float64 E(0), D(0);
      if ( installRailingSystemIntervalIdx < intervalIdx )
      {
         E = GetRailingSystemEc(orientation,intervalIdx);
         D = GetRailingSystemWeightDensity(orientation,intervalIdx);
      }

      material->put_E(intervalIdx,E);
      material->put_Density(intervalIdx,D);
   }
}

bool CBridgeAgentImp::LayoutTrafficBarrier(const CBridgeDescription2* pBridgeDesc,const CRailingSystem* pRailingSystem,pgsTypes::TrafficBarrierOrientation orientation,ISidewalkBarrier** ppBarrier)
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
   UpdatePrestressing(ALL_GROUPS,ALL_GIRDERS,ALL_SEGMENTS);
   return true;
}

void CBridgeAgentImp::ValidateGirder()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);

      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

            IntervalIndexType releaseIntervalIdx = GetPrestressReleaseInterval(segmentKey);

            Float64 length = GetSegmentLength(segmentKey);
            Float64 effStrands;
            Float64 end_ecc = GetEccentricity(releaseIntervalIdx, pgsPointOfInterest(segmentKey, 0.0),       pgsTypes::Harped, &effStrands);
            Float64 hp_ecc  = GetEccentricity(releaseIntervalIdx, pgsPointOfInterest(segmentKey,length/2.0), pgsTypes::Harped, &effStrands);

            if (hp_ecc+TOLERANCE < end_ecc)
            {
               GET_IFACE(IEAFStatusCenter,pStatusCenter);
               LPCTSTR msg = _T("Harped strand eccentricity at girder ends is larger than at harping points. Drape is upside down");
               pgsGirderDescriptionStatusItem* pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,0,m_StatusGroupID,m_scidGirderDescriptionWarning,msg);
               pStatusCenter->Add(pStatusItem);
            }

            ValidateElevationAdjustments(segmentKey);
         } // segment loop
      } // girder loop
   } // group loop
}

void CBridgeAgentImp::ValidateElevationAdjustments(const CSegmentKey& segmentKey)
{
   if (m_ElevationAdjustmentEquations.find(segmentKey) != m_ElevationAdjustmentEquations.end())
   {
      return; // validation for this segment is done
   }

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   if ( !pBridgeDesc->IsStable() )
   {
      // Bridge is not stable so the elevation adjustment can't be computed.
      // Analysis cannot happen if the bridge is not stable so this is just a dummy/placeholder
      // equation for this segment.
      // The bridge is a mechanism or a linkage and there will be rigid body motions of the segments.
      // Calculaton of the elevation adjusts ends up in an infinite recursive loop for unstable structures..
      // Just add a dummy element for this segment. The Analysis Agent will balk for unstable structures.
      // If we balk here, the structure can't be drawn in the UI.
      mathLinFunc2d fn(0,0);
      m_ElevationAdjustmentEquations.insert(std::make_pair(segmentKey,fn));
      return;
   }

   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   const CSplicedGirderData* pGirder   = pSegment->GetGirder();
   const CGirderGroupData* pGroup      = pGirder->GetGirderGroup();

   // elevation adjustments at permanent piers are measured relative to the "A" dimension
   // at the first pier. Get "A" at the first pier
   GirderIndexType gdrIdx = Min(pIBridgeDesc->GetGirderGroup(0)->GetGirderCount()-1,segmentKey.girderIndex);
   Float64 Ao = pIBridgeDesc->GetGirderGroup(0)->GetSlabOffset(0,gdrIdx);

   // We want to get an equation for the slope of the segment in terms of y=mx+b
   // The range of the equation is the segment coordinate system

   // Compute slope of segment
   bool bIsSlopeComputed = false;
   Float64 slope = 0;
   Float64 Yo = 0;
   Float64 Xo = 0;

   // If a segment, when erected is a statically indeterminant beam, it may be overconstrained.
   // There is an inherant assumption that all the supports are on a straight line. If this
   // is not true, additional forces are needed to bend the segment into conformance with the
   // geometry of the support conditions. We are not computing the forces required to manipulate
   // the segment into the support conditions because it is essentially bad input.
   //
   // Consider a segment that is supported by erection towers at each end and a permanent pier
   // at is center when it is erected. We assume that the elastic line of the girder is straight.
   // When the segment is erected, it is placed such that the resulting structure is a two span
   // continuous statically indetermant beam (classic, textbook beam). If the center pier is too high
   // or one or both of the erection towers are too high or too low, forces are required to get both
   // ends and the center of the segment to engage the supports. Let's say their is a support elevation
   // adjustment at the right hand erection tower of -2" (2" down). When the segment is erected, it will bear on
   // the left erection tower and the permanent pier. The right end of the segment will be cantilevered because
   // the right erection tower is 2" low. In order to conform to our assumption that the beam is a two-span
   // continuous statically indeterminate beam, forces have to be applied to the right end of the beam to 
   // make it engage the erection tower. The 2" elevation adjust is the root cause of the problem (bad input).
   // 
   // We deal with this by posting an item into the status center that tells the user the support adjustments
   // are not consistent with the segment geometry. When computing the segment slope below, only two
   // supports are used so we get a good slope.
   if ( pBridgeDesc->IsSegmentOverconstrained(segmentKey) )
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);

      CString strMsg;
      strMsg.Format(_T("Segment %d in Group %d Girder %s is overconstrained. One or more of the erection tower elevation adjustments or slab offsets at CL pier have been ignored. Adjust support elevation geometry as needed."),
         LABEL_SEGMENT(segmentKey.segmentIndex),LABEL_GROUP(segmentKey.groupIndex),LABEL_GIRDER(segmentKey.girderIndex));

      pgsBridgeDescriptionStatusItem* pStatusItem = 
         new pgsBridgeDescriptionStatusItem(m_StatusGroupID,m_scidBridgeDescriptionError,pgsBridgeDescriptionStatusItem::Framing,strMsg);

      pStatusCenter->Add(pStatusItem);
   }

   // Get key segment points. these will be used to compute the distance from
   // the start end of the segment to one of the supports used to establish
   // the segment slope
   CComPtr<IPoint2d> pntPier1, pntEnd1, pntBrg1, pntBrg2, pntEnd2, pntPier2;
   GetSegmentEndPoints(segmentKey,&pntPier1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntPier2);

   // segment slope is established by its supports
   std::vector<const CPierData2*> vPiers(pSegment->GetPiers());
   std::vector<const CTemporarySupportData*> vTS( pSegment->GetTemporarySupports());

   // First priority is given to permanent piers
   if ( 2 <= vPiers.size() )
   {
      // There are at least 2 permanent piers supporting this segment
      // Use the first and last pier to establish the slope.
      // If there are more then 2 piers, the slab offset at the interior piers
      // must be such that the entire segment has a constant slope. This is checked
      // when checking if the segment is overconstrained above.
      const CPierData2* pFirstPier = vPiers.front();
      const CPierData2* pLastPier  = vPiers.back();

      CComPtr<IPoint2d> pntFirst, pntLast;
      VERIFY(GetSegmentPierIntersection(segmentKey,pFirstPier->GetIndex(),&pntFirst));
      VERIFY(GetSegmentPierIntersection(segmentKey,pLastPier->GetIndex(), &pntLast));

      Float64 dist;
      pntLast->DistanceEx(pntFirst,&dist);

      Float64 Astart = pGroup->GetSlabOffset(pFirstPier->GetIndex(),segmentKey.girderIndex);
      Float64 Aend   = pGroup->GetSlabOffset(pLastPier->GetIndex(), segmentKey.girderIndex);

      // if Aend < Astart, the "A" dimension decreases over the length of the girder
      // so the slope is upwards to the right, which is positive
      slope = (Astart-Aend)/dist;

      // point on line
      Yo = Ao - Astart;
      pntPier1->DistanceEx(pntFirst,&Xo);

      bIsSlopeComputed = true;
   }
   else if ( 1 == vPiers.size() )
   {
      // second priority is given to a single permanent pier and a temporary support
      // Temporary support must be an erection tower

      ATLASSERT( 1 <= vTS.size() );

      // Find the first erection tower
      const CTemporarySupportData* pFirstTS = NULL;
      std::vector<const CTemporarySupportData*>::const_iterator tsIter(vTS.begin());
      std::vector<const CTemporarySupportData*>::const_iterator tsIterEnd(vTS.end());
      for ( ; tsIter != tsIterEnd; tsIter++ )
      {
         const CTemporarySupportData* pTS = *tsIter;
         if ( pTS->GetSupportType() == pgsTypes::ErectionTower )
         {
            pFirstTS = pTS;
            break;
         }
      }

      if ( pFirstTS != NULL )
      {
         const CPierData2* pPier = vPiers.front();
         ATLASSERT(!IsEqual(pFirstTS->GetStation(),pPier->GetStation()));

         // we have two unique supports, so the segment slope can be computed
         CComPtr<IPoint2d> pntFirst, pntLast;
         VERIFY(GetSegmentTempSupportIntersection(segmentKey,pFirstTS->GetIndex(),&pntFirst));
         VERIFY(GetSegmentPierIntersection(segmentKey,pPier->GetIndex(),  &pntLast));

         Float64 dist;
         pntLast->DistanceEx(pntFirst,&dist);

         Float64 Dstart = pFirstTS->GetElevationAdjustment();
         Float64 Aend   = pGroup->GetSlabOffset(pPier->GetIndex(), segmentKey.girderIndex);

         Float64 Dend = Ao-Aend;
         slope = (Dend-Dstart)/dist;

         // slope is computed assuming that the last support is after the start support...
         // if that isn't true, we need to change the sign on the slope
         Float64 Xfirst,Xlast;
         pntFirst->get_X(&Xfirst);
         pntLast->get_X(&Xlast);
         if ( Xlast < Xfirst )
         {
            slope *= -1;
         }

         // point on line
         Yo = Dstart;
         pntPier1->DistanceEx(pntFirst,&Xo);

         bIsSlopeComputed = true;
      }
   }
   else if ( 2 <= vTS.size() )
   {
      // there aren't any permanent piers supporting this segment so priority now goes
      // to temporary supports.
      //

      // Temporary supports must be erection towers. Find first erection tower
      const CTemporarySupportData* pFirstTS = NULL;
      std::vector<const CTemporarySupportData*>::const_iterator tsIter(vTS.begin());
      std::vector<const CTemporarySupportData*>::const_iterator tsIterEnd(vTS.end());
      for ( ; tsIter != tsIterEnd; tsIter++ )
      {
         const CTemporarySupportData* pTS = *tsIter;
         if ( pTS->GetSupportType() == pgsTypes::ErectionTower )
         {
            pFirstTS = pTS;
            break;
         }
      }

      // find last erection tower (iterate in reverse order)
      const CTemporarySupportData* pLastTS = NULL;
      std::vector<const CTemporarySupportData*>::const_reverse_iterator rtsIter(vTS.rbegin());
      std::vector<const CTemporarySupportData*>::const_reverse_iterator rtsIterEnd(vTS.rend());
      for ( ; rtsIter != rtsIterEnd; rtsIter++ )
      {
         const CTemporarySupportData* pTS = *rtsIter;
         if ( pTS->GetSupportType() == pgsTypes::ErectionTower )
         {
            pLastTS = pTS;
            break;
         }
      }

      if ( pFirstTS != NULL && pLastTS != NULL && pFirstTS != pLastTS )
      {
         // we have two unique erection towers, the segment slope can be computed
         CComPtr<IPoint2d> pntFirst, pntLast;
         VERIFY(GetSegmentTempSupportIntersection(segmentKey,pFirstTS->GetIndex(),&pntFirst));
         VERIFY(GetSegmentTempSupportIntersection(segmentKey,pLastTS->GetIndex(),  &pntLast));

         Float64 dist;
         pntLast->DistanceEx(pntFirst,&dist);

         Float64 Dstart = pFirstTS->GetElevationAdjustment();
         Float64 Dend   = pLastTS->GetElevationAdjustment();

         slope = (Dend-Dstart)/dist;

         // point on line
         Yo = Dstart;
         pntPier1->DistanceEx(pntFirst,&Xo);

         bIsSlopeComputed = true;
      }
   }

   if ( !bIsSlopeComputed )
   {
      // there aren't enough supports to establish the slope of this segment, therefore
      // the segment slope is defined by the adjustments at the end/start of the previous/next segment.
      // this case typically occurs with drop-in segments that are supported only with strongbacks at each end.


      Float64 startAdj     = 0;
      Float64 startDistAdj = 0;
      Float64 endAdj       = 0;
      Float64 endDistAdj   = 0;

      if ( segmentKey.segmentIndex == 0 )
      {
         // startAdj is measured at CL bearing so the segment layout length needs to be adjusted by
         // this amount to get the slope right
         startAdj = 0;
         startDistAdj = GetSegmentStartBearingOffset(segmentKey) - GetSegmentStartEndDistance(segmentKey);
      }

      if ( segmentKey.segmentIndex != 0 )
      {
         CSegmentKey prevSegmentKey(segmentKey);
         prevSegmentKey.segmentIndex--;

         Float64 prevSegmentLayoutLength = GetSegmentLayoutLength(prevSegmentKey);

         ValidateElevationAdjustments(prevSegmentKey); // if the bridge isn't stable, this call is infinitely recursive
         std::map<CSegmentKey,mathLinFunc2d>::iterator found(m_ElevationAdjustmentEquations.find(prevSegmentKey));
         ATLASSERT(found != m_ElevationAdjustmentEquations.end());
         mathLinFunc2d& fn = found->second;
         startAdj = fn.Evaluate(prevSegmentLayoutLength);

         // startAdj is measured at CL bearing so the segment layout length needs to be adjusted by
         // this amount to get the slope right
         startDistAdj = GetSegmentStartBearingOffset(segmentKey) - GetSegmentStartEndDistance(segmentKey);
      }

      if ( segmentKey.segmentIndex == pGirder->GetSegmentCount()-1 )
      {
         Float64 Aend = pGroup->GetSlabOffset(pGroup->GetPier(pgsTypes::metEnd)->GetIndex(),segmentKey.girderIndex);
         endAdj = Ao - Aend;

         // endAdj is measured at CL bearing so the segment layout length needs to be adjusted by
         // this amount to get the slope right
         endDistAdj = GetSegmentEndBearingOffset(segmentKey) - GetSegmentEndEndDistance(segmentKey);
      }
      else
      {
         CSegmentKey nextSegmentKey(segmentKey);
         nextSegmentKey.segmentIndex++;
         
         ValidateElevationAdjustments(nextSegmentKey); // if the bridge isn't stable, this call is infinitely recursive
         std::map<CSegmentKey,mathLinFunc2d>::iterator found(m_ElevationAdjustmentEquations.find(nextSegmentKey));
         ATLASSERT(found != m_ElevationAdjustmentEquations.end());
         mathLinFunc2d& fn = found->second;
         endAdj = fn.Evaluate(0.0);
      }

      Float64 dist = GetSegmentLayoutLength(segmentKey) - startDistAdj - endDistAdj;

      slope = (endAdj - startAdj)/dist;

      Yo = startAdj;
      Xo = startDistAdj;
   }

   // Now that we have the slope, need to compute the Y-intercept (b in y=mx+b)
   Float64 b = Yo - slope*Xo;

   // Compute the elevation adjustment at the poi
   mathLinFunc2d fn(slope,b);

   m_ElevationAdjustmentEquations.insert(std::make_pair(segmentKey,fn));
}

void CBridgeAgentImp::UpdatePrestressing(GroupIndexType groupIdx,GirderIndexType girderIdx,SegmentIndexType segmentIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GroupIndexType firstGroupIdx,lastGroupIdx;
   if ( groupIdx == ALL_GROUPS )
   {
      firstGroupIdx = 0;
      lastGroupIdx = pBridgeDesc->GetGirderGroupCount()-1;
   }
   else
   {
      firstGroupIdx = groupIdx;
      lastGroupIdx = firstGroupIdx;
   }

   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);

      GirderIndexType firstGirderIdx, lastGirderIdx;
      if ( girderIdx == ALL_GIRDERS )
      {
         firstGirderIdx = 0;
         lastGirderIdx = pGroup->GetGirderCount()-1;
      }
      else
      {
         firstGirderIdx = girderIdx;
         lastGirderIdx = firstGirderIdx;
      }

      for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++ )
      {
         Float64 segmentOffset = 0;

         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType firstSegmentIdx, lastSegmentIdx;
         if ( segmentIdx == ALL_SEGMENTS )
         {
            firstSegmentIdx = 0;
            lastSegmentIdx = pGirder->GetSegmentCount()-1;
         }
         else
         {
            firstSegmentIdx = segmentIdx;
            lastSegmentIdx = firstSegmentIdx;
         }

         for ( SegmentIndexType segIdx = firstSegmentIdx; segIdx <= lastSegmentIdx; segIdx++ )
         {
            // should be passing segment key into this method
            CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);
            const GirderLibraryEntry* pGirderEntry = GetGirderLibraryEntry(segmentKey);

            // Inititalize a strand filler for each girder
            InitializeStrandFiller(pGirderEntry, segmentKey);

            CComPtr<IPrecastGirder> girder;
            GetGirder(segmentKey,&girder);

            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

            girder->ClearStraightStrandDebonding();

            // Fill strands
            CStrandData::StrandDefinitionType strandDefinitionType = pSegment->Strands.GetStrandDefinitionType();
            if (strandDefinitionType == CStrandData::sdtTotal || 
                strandDefinitionType == CStrandData::sdtStraightHarped ||
                strandDefinitionType == CStrandData::sdtDirectInput )
            {
               // Continuous fill
               CContinuousStrandFiller* pfiller = GetContinuousStrandFiller(segmentKey);

               HRESULT hr;
               hr = pfiller->SetStraightContinuousFill(girder, pSegment->Strands.GetStrandCount(pgsTypes::Straight));
               ATLASSERT( SUCCEEDED(hr));
               hr = pfiller->SetHarpedContinuousFill(girder, pSegment->Strands.GetStrandCount(pgsTypes::Harped));
               ATLASSERT( SUCCEEDED(hr));
               hr = pfiller->SetTemporaryContinuousFill(girder, pSegment->Strands.GetStrandCount(pgsTypes::Temporary));
               ATLASSERT( SUCCEEDED(hr));
            }
            else if (strandDefinitionType == CStrandData::sdtDirectSelection)
            {
               // Direct fill
               CDirectStrandFiller* pfiller = GetDirectStrandFiller(segmentKey);

               HRESULT hr;
               hr = pfiller->SetStraightDirectStrandFill(girder, pSegment->Strands.GetDirectStrandFillStraight());
               ATLASSERT( SUCCEEDED(hr));
               hr = pfiller->SetHarpedDirectStrandFill(girder, pSegment->Strands.GetDirectStrandFillHarped());
               ATLASSERT( SUCCEEDED(hr));
               hr = pfiller->SetTemporaryDirectStrandFill(girder, pSegment->Strands.GetDirectStrandFillTemporary());
               ATLASSERT( SUCCEEDED(hr));
            }
            else
            {
               ATLASSERT(false); // is there a new fill type?
            }

            if ( strandDefinitionType != CStrandData::sdtDirectInput )
            {
               // Apply harped strand pattern offsets.
               // Get fill array for harped and convert to ConfigStrandFillVector
               CComPtr<IIndexArray> hFillArray;
               girder->get_HarpedStrandFill(&hFillArray);
               ConfigStrandFillVector hFillVec;
               IndexArray2ConfigStrandFillVec(hFillArray, hFillVec);

               Float64 adjustment(0.0);
               if (pSegment->Strands.GetAdjustableStrandType() == pgsTypes::asHarped)
               {
                  if ( pGirderEntry->IsVerticalAdjustmentAllowedEnd() )
                  {
                     adjustment = this->ComputeAbsoluteHarpedOffsetEnd(segmentKey, hFillVec, pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd(),pSegment->Strands.GetHarpStrandOffsetAtEnd());
                     girder->put_HarpedStrandAdjustmentEnd(adjustment);
                  }
      
                  if ( pGirderEntry->IsVerticalAdjustmentAllowedHP() && pSegment->Strands.GetAdjustableStrandType()==pgsTypes::asHarped)
                  {
                     adjustment = this->ComputeAbsoluteHarpedOffsetHp(segmentKey, hFillVec, 
                                                                      pSegment->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint(), pSegment->Strands.GetHarpStrandOffsetAtHarpPoint());
                     girder->put_HarpedStrandAdjustmentHP(adjustment);
                  }
               }
               else
               {
                  // Adjustable strands are straight
                  if ( pGirderEntry->IsVerticalAdjustmentAllowedStraight() )
                  {
                     adjustment = this->ComputeAbsoluteHarpedOffsetEnd(segmentKey, hFillVec, pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd(), pSegment->Strands.GetHarpStrandOffsetAtEnd());
                     girder->put_HarpedStrandAdjustmentEnd(adjustment);
                     // Use same adjustment at harping points if harped strands are forced to straight
                     girder->put_HarpedStrandAdjustmentHP(adjustment);
                  }
               }
            }

            // Apply debonding
            const std::vector<CDebondData>& vDebond = pSegment->Strands.GetDebonding(pgsTypes::Straight);
            std::vector<CDebondData>::const_iterator iter(vDebond.begin());
            std::vector<CDebondData>::const_iterator end(vDebond.end());
            for ( ; iter != end; iter++ )
            {
               const CDebondData& debond_data = *iter;

               // debond data index is in same order as grid fill
               girder->DebondStraightStrandByGridIndex(debond_data.strandTypeGridIdx,debond_data.Length[pgsTypes::metStart],debond_data.Length[pgsTypes::metEnd]);
            }
         
            // lay out POIs based on this prestressing
            LayoutPrestressTransferAndDebondPoi(segmentKey,segmentOffset); 

            segmentOffset += GetSegmentLayoutLength(segmentKey);

         } // segment loop
      } // girder loop
   } // group loop
}

bool CBridgeAgentImp::AreGirderTopFlangesRoughened(const CSegmentKey& segmentKey)
{
   GET_IFACE(IShear,pShear);
	const CShearData2* pShearData = pShear->GetSegmentShearData(segmentKey);
   return pShearData->bIsRoughenedSurface;
}

void CBridgeAgentImp::GetClosureJointProfile(const CClosureKey& closureKey,IShape** ppShape)
{
   GroupIndexType      grpIdx     = closureKey.groupIndex;
   GirderIndexType     gdrIdx     = closureKey.girderIndex;
   CollectionIndexType closureIdx = closureKey.segmentIndex;

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
   const CClosureJointData* pClosureJoint = pGirder->GetClosureJoint(closureIdx);
   
   const CPrecastSegmentData* pLeftSegment = pClosureJoint->GetLeftSegment();
   SegmentIndexType leftSegIdx = pLeftSegment->GetIndex();
   
   const CPrecastSegmentData* pRightSegment = pClosureJoint->GetRightSegment();
   SegmentIndexType rightSegIdx = pRightSegment->GetIndex();

   CSegmentKey leftSegmentKey(grpIdx,gdrIdx,leftSegIdx);
   CSegmentKey rightSegmentKey(grpIdx,gdrIdx,rightSegIdx);

   Float64 leftSegStartOffset, leftSegEndOffset;
   GetSegmentEndDistance(leftSegmentKey,&leftSegStartOffset,&leftSegEndOffset);

   Float64 rightSegStartOffset, rightSegEndOffset;
   GetSegmentEndDistance(rightSegmentKey,pGirder,&rightSegStartOffset,&rightSegEndOffset);

   Float64 leftSegStartBrgOffset, leftSegEndBrgOffset;
   GetSegmentBearingOffset(leftSegmentKey,&leftSegStartBrgOffset,&leftSegEndBrgOffset);

   Float64 rightSegStartBrgOffset, rightSegEndBrgOffset;
   GetSegmentBearingOffset(rightSegmentKey,&rightSegStartBrgOffset,&rightSegEndBrgOffset);

   Float64 xLeftStart,xLeftEnd;
   GetSegmentRange(leftSegmentKey,&xLeftStart,&xLeftEnd);

   Float64 xRightStart,xRightEnd;
   GetSegmentRange(rightSegmentKey,&xRightStart,&xRightEnd);

   Float64 xStart = xLeftEnd;
   Float64 xEnd = xRightStart;

   xStart -= (leftSegEndBrgOffset - leftSegEndOffset);
   xEnd   += (rightSegStartBrgOffset - rightSegStartOffset);

   boost::shared_ptr<mathFunction2d> f = CreateGirderProfile(pGirder);
   CComPtr<IPolyShape> polyShape;
   polyShape.CoCreateInstance(CLSID_PolyShape);

   std::vector<Float64> xValues;

   // fill up with other points
   for ( int i = 0; i < 5; i++ )
   {
      Float64 x = i*(xEnd - xStart)/4 + xStart;
      xValues.push_back(x);
   }

   std::sort(xValues.begin(),xValues.end());

   std::vector<Float64>::iterator iter(xValues.begin());
   std::vector<Float64>::iterator end(xValues.end());
   for ( ; iter != end; iter++ )
   {
      Float64 x = *iter;
      Float64 y = f->Evaluate(x);
      polyShape->AddPoint(x,-y);
   }

   // points across the top of the segment
   polyShape->AddPoint(xEnd,0);
   polyShape->AddPoint(xStart,0);

   polyShape->get_Shape(ppShape);
}

void CBridgeAgentImp::GetClosureJointSize(const CClosureKey& closureKey,Float64* pLeft,Float64* pRight)
{
   GroupIndexType      grpIdx     = closureKey.groupIndex;
   GirderIndexType     gdrIdx     = closureKey.girderIndex;
   CollectionIndexType closureIdx = closureKey.segmentIndex;

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
   
   if ( pGirder->GetClosureJointCount() <= closureIdx )
   {
      ATLASSERT(false); // shouldn't get here
      *pLeft = 0;
      *pRight = 0;
      return;
   }

   const CClosureJointData* pClosureJoint = pGirder->GetClosureJoint(closureIdx);
   
   const CPrecastSegmentData* pLeftSegment = pClosureJoint->GetLeftSegment();
   SegmentIndexType leftSegIdx = pLeftSegment->GetIndex();
   
   const CPrecastSegmentData* pRightSegment = pClosureJoint->GetRightSegment();
   SegmentIndexType rightSegIdx = pRightSegment->GetIndex();

   CSegmentKey leftSegmentKey(grpIdx,gdrIdx,leftSegIdx);
   CSegmentKey rightSegmentKey(grpIdx,gdrIdx,rightSegIdx);

   Float64 leftSegStartEndDist, leftSegEndEndDist;
   GetSegmentEndDistance(leftSegmentKey,&leftSegStartEndDist,&leftSegEndEndDist);

   Float64 rightSegStartEndDist, rightSegEndEndDist;
   GetSegmentEndDistance(rightSegmentKey,&rightSegStartEndDist,&rightSegEndEndDist);

   Float64 leftSegStartBrgOffset, leftSegEndBrgOffset;
   GetSegmentBearingOffset(leftSegmentKey,&leftSegStartBrgOffset,&leftSegEndBrgOffset);

   Float64 rightSegStartBrgOffset, rightSegEndBrgOffset;
   GetSegmentBearingOffset(rightSegmentKey,&rightSegStartBrgOffset,&rightSegEndBrgOffset);

   *pLeft  = leftSegEndBrgOffset    - leftSegEndEndDist;
   *pRight = rightSegStartBrgOffset - rightSegStartEndDist;
}

Float64 CBridgeAgentImp::GetClosureJointLength(const CClosureKey& closureKey)
{
   Float64 left, right;
   GetClosureJointSize(closureKey,&left,&right);
   return left+right;
}

void CBridgeAgentImp::GetAngleBetweenSegments(const CClosureKey& closureKey,IAngle** ppAngle)
{
   CSegmentKey backSegmentKey(closureKey);
   CSegmentKey aheadSegmentKey(closureKey);
   aheadSegmentKey.segmentIndex++;

   CComPtr<IDirection> backBearing, aheadBearing;
   GetSegmentDirection(backSegmentKey,&backBearing);
   GetSegmentDirection(aheadSegmentKey,&aheadBearing);

   backBearing->AngleBetween(aheadBearing,ppAngle);
}

void CBridgeAgentImp::LayoutPointsOfInterest(const CGirderKey& girderKey)
{
   ASSERT_GIRDER_KEY(girderKey);

   SpanIndexType startSpanIdx, endSpanIdx;
   GetGirderGroupSpans(girderKey.groupIndex,&startSpanIdx,&endSpanIdx);
   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      CSpanKey spanKey(spanIdx,girderKey.girderIndex);
      LayoutSpanPoi(spanKey,10);
      LayoutPoiForIntermediateDiaphragmLoads(spanKey);
   }

   SegmentIndexType nSegments = GetSegmentCount(girderKey);
   Float64 segment_offset = 0; // offset from start of the girder to the end of the previous segment

   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);

      LayoutRegularPoi(segmentKey,10,segment_offset); // 10th points in each segment.
      LayoutSpecialPoi(segmentKey,segment_offset);

      Float64 segment_layout_length = GetSegmentLayoutLength(segmentKey);
      segment_offset += segment_layout_length;
   }

   LayoutPoiForSlabBarCutoffs(girderKey);

   LayoutPoiForTendons(girderKey);
}

void CBridgeAgentImp::LayoutSpanPoi(const CSpanKey& spanKey,Uint16 nPnts)
{
   Float64 span_length = GetSpanLength(spanKey);
   const Float64 toler = +1.0e-6;

   for ( Uint16 i = 0; i <= nPnts; i++ )
   {
      Float64 Xspan = span_length * ((Float64)i / (Float64)nPnts); // distance from CL Brg

      pgsPointOfInterest poi = ConvertSpanPointToPoi(spanKey,Xspan);
      if ( poi.GetID() != INVALID_ID && poi.GetReferencedAttributes(POI_SPAN) != 0)
      {
         // if the poi has an ID and it is a span attribute then it is
         // a common POI in an adjacent span. don't merge this poi
         // when adding to the poi manager
         // We want (Span i-1, 1.0L) and (Span i, 0.0L) as seperate attributes
         // because a POI can't be at two of the same type of tenth point locations at once
         poi.SetID(INVALID_ID);
         poi.SetNonReferencedAttributes(0);
         poi.SetReferencedAttributes(0);
         poi.CanMerge(false);
      }

      Uint16 tenthPoint = 0;

      // Add a special attribute flag if poi is at a tenth point
      Float64 val = Float64(i)/Float64(nPnts)+toler;
      Float64 modv = fmod(val, 0.1);
      if (IsZero(modv,2*toler) || modv==0.1)
      {
         tenthPoint = Uint16(10.*Float64(i)/Float64(nPnts) + 1);
      }

      poi.MakeTenthPoint(POI_SPAN,tenthPoint);
      m_pPoiMgr->AddPointOfInterest( poi );
   }
}

void CBridgeAgentImp::LayoutRegularPoi(const CSegmentKey& segmentKey,Uint16 nPnts,Float64 segmentOffset)
{
   // This method creates POIs at n-th points for segment spans lengths at release and for the erected segment
   ASSERT_SEGMENT_KEY(segmentKey);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);

   const Float64 toler = +1.0e-6;

   Float64 segment_length  = GetSegmentLength(segmentKey); // end to end length
   Float64 span_length     = GetSegmentSpanLength(segmentKey); // cl brg to cl brg length
   Float64 left_brg_offset = GetSegmentStartBearingOffset(segmentKey);
   Float64 left_end_size   = GetSegmentStartEndDistance(segmentKey);

   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   Float64 left_storage_point  = pSegment->HandlingData.LeftStoragePoint;
   Float64 right_storage_point = pSegment->HandlingData.RightStoragePoint;
   Float64 storage_span_length = segment_length - left_storage_point - right_storage_point;

   GroupIndexType nGroups     = GetGirderGroupCount();
   SegmentIndexType nSegments = GetSegmentCount(segmentKey);

   // distance from CL Support to start end of girder
   Float64 start_offset = left_brg_offset - left_end_size;

   CSegmentKey firstSegmentKey(segmentKey);
   firstSegmentKey.segmentIndex = 0;
   Float64 first_segment_start_offset = GetSegmentStartBearingOffset(firstSegmentKey) - GetSegmentStartEndDistance(firstSegmentKey);

   for ( Uint16 i = 0; i <= nPnts; i++ )
   {
#if defined _REDUCE_POI
      // cut down on the number of POI.
      // Will only have POI at 0.0, 0.3, 0.5, 0.7 and 1.0L
      // This makes the time-step analysis go faster
      if ( i == 1 || i == 2 || i == 4 || i == 6 || i == 8 || i == 9 )
      {
         continue;
      }
#endif
      Float64 casting_yard_dist = segment_length * ((Float64)i / (Float64)nPnts); // distance from CL Brg
      Float64 bridge_site_dist  = span_length * ((Float64)i / (Float64)nPnts); // distance from CL Brg
      Float64 storage_dist      = storage_span_length * ((Float64)i / (Float64)nPnts); // distance from left storage point

      PoiAttributeType attribute = 0;

      Uint16 tenthPoint = 0;

      // Add a special attribute flag if poi is at a tenth point
      Float64 val = Float64(i)/Float64(nPnts)+toler;
      Float64 modv = fmod(val, 0.1);
      if (IsZero(modv,2*toler) || modv==0.1)
      {
         tenthPoint = Uint16(10.*Float64(i)/Float64(nPnts) + 1);
      }

      // create casting yard POI
      Float64 Xs  = casting_yard_dist; // distance from left face of segment
      Float64 Xsp = start_offset + Xs; // distance from CLSupport - CLSegment intersection point
      Float64 Xgp = segmentOffset + Xsp; // distance from CLSupport - CLGirder intersection point
      Float64 Xg  = Xgp - first_segment_start_offset;  // distance from left face of first segment
      pgsPointOfInterest cyPoi(segmentKey,Xs,Xsp,Xg,Xgp,attribute | POI_RELEASED_SEGMENT);
      cyPoi.MakeTenthPoint(POI_RELEASED_SEGMENT,tenthPoint);
      m_pPoiMgr->AddPointOfInterest( cyPoi );

      Xs  = left_end_size + bridge_site_dist;
      Xsp = start_offset + Xs;
      Xgp = segmentOffset + Xsp;
      Xg  = Xgp - first_segment_start_offset;
      pgsPointOfInterest bsPoi(segmentKey,Xs,Xsp,Xg,Xgp,attribute | POI_ERECTED_SEGMENT);
      bsPoi.MakeTenthPoint(POI_ERECTED_SEGMENT,tenthPoint);

      // If this is the very first or very last erected segment 10th point in the bridge, these
      // are key CL Bearing points. Mark them with the POI_ABUTMENT attribute so they can be
      // easily found when searching for piers
      if ( (segmentKey.groupIndex == 0         && segmentKey.segmentIndex == 0           && bsPoi.IsTenthPoint(POI_ERECTED_SEGMENT) == 1) ||
           (segmentKey.groupIndex == nGroups-1 && segmentKey.segmentIndex == nSegments-1 && bsPoi.IsTenthPoint(POI_ERECTED_SEGMENT) == 11) )
      {
         bsPoi.SetNonReferencedAttributes(bsPoi.GetNonReferencedAttributes() | POI_ABUTMENT);
      }

      m_pPoiMgr->AddPointOfInterest( bsPoi );

      Xs  = left_storage_point + storage_dist;
      Xsp = start_offset + Xs;
      Xgp = segmentOffset + Xsp;
      Xg  = Xgp - first_segment_start_offset;
      pgsPointOfInterest stPoi(segmentKey,Xs,Xsp,Xg,Xgp,attribute | POI_STORAGE_SEGMENT);
      stPoi.MakeTenthPoint(POI_STORAGE_SEGMENT,tenthPoint);

      m_pPoiMgr->AddPointOfInterest( stPoi );

      // if this is the last poi on the segment, and there is a closure joint on the right end
      // of the segment, then add a closure POI
      if ( i == nPnts )
      {
         const CClosureJointData*   pClosure = pSegment->GetEndClosure();
         if ( pClosure != NULL )
         {
            Float64 closure_left, closure_right;
            GetClosureJointSize(pClosure->GetClosureKey(),&closure_left,&closure_right);

            Float64 Xs  = segment_length + closure_left;
            Float64 Xsp = start_offset + Xs;
            Float64 Xgp = segmentOffset + Xsp;
            Float64 Xg  = Xgp - first_segment_start_offset;
            pgsPointOfInterest cpPOI(segmentKey,Xs,Xsp,Xg,Xgp,POI_CLOSURE);

            // if the segment ends on a permanent pier, add the POI_BOUNDARY_PIER attribute
            const CPierData2* pPier;
            const CTemporarySupportData* pTS;
            pSegment->GetSupport(pgsTypes::metEnd,&pPier,&pTS);
            if ( pPier )
            {
               cpPOI.SetNonReferencedAttributes(cpPOI.GetNonReferencedAttributes() | POI_BOUNDARY_PIER);
            }
            m_pPoiMgr->AddPointOfInterest( cpPOI );
         }
      }
   }
}

void CBridgeAgentImp::LayoutSpecialPoi(const CSegmentKey& segmentKey,Float64 segmentOffset)
{
   ASSERT_SEGMENT_KEY(segmentKey);

   LayoutEndSizePoi(segmentKey,segmentOffset);
   LayoutHarpingPointPoi(segmentKey,segmentOffset);
   //LayoutPrestressTransferAndDebondPoi(segmentKey,segmentOffset); // this is done when the prestressing is updated
   LayoutPoiForPrecastDiaphragmLoads(segmentKey,segmentOffset);
   LayoutPoiForShear(segmentKey,segmentOffset);
   LayoutPoiForSegmentBarCutoffs(segmentKey,segmentOffset);

   LayoutPoiForHandling(segmentKey);     // lifting and hauling
   LayoutPoiForSectionChanges(segmentKey);
   LayoutPoiForPiers(segmentKey);
   LayoutPoiForTemporarySupports(segmentKey);
}

void CBridgeAgentImp::ModelCantilevers(const CSegmentKey& segmentKey,bool* pbStartCantilever,bool* pbEndCantilever)
{
   // This method determines if the overhangs at the ends of a segment are modeled as cantilevers
   ASSERT_SEGMENT_KEY(segmentKey);

   *pbStartCantilever = false;
   *pbEndCantilever   = false;

   if ( segmentKey.groupIndex == 0 && segmentKey.segmentIndex == 0 )
   {
      // the first segment in the first group will be supported by the first pier in the bridge
      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      const CPierData2* pPier = pIBridgeDesc->GetPier(0);
      if ( pPier->HasCantilever() )
      {
         *pbStartCantilever = true;
      }
   }

   GroupIndexType nGroups = GetGirderGroupCount();
   SegmentIndexType nSegments = GetSegmentCount(nGroups-1,0);
   if ( segmentKey.groupIndex == nGroups-1 && segmentKey.segmentIndex == nSegments-1 )
   {
      // the last segment in the last group will be supported by the last pier in the bridge
      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      PierIndexType nPiers = pIBridgeDesc->GetPierCount();
      const CPierData2* pPier = pIBridgeDesc->GetPier(nPiers-1);
      if ( pPier->HasCantilever() )
      {
         *pbEndCantilever = true;
      }
   }

   // Overhangs are modeled as cantilevers if they are longer than the height of the segment at the CL Bearing
   Float64 segment_length       = GetSegmentLength(segmentKey);
   Float64 start_offset         = GetSegmentStartEndDistance(segmentKey);
   Float64 end_offset           = GetSegmentEndEndDistance(segmentKey);
   Float64 segment_height_start = GetHeight(pgsPointOfInterest(segmentKey,start_offset));
   Float64 segment_height_end   = GetHeight(pgsPointOfInterest(segmentKey,segment_length-end_offset));

   // the cantilevers at the ends of the segment are modeled as flexural members if
   // if the cantilever length exceeds the height of the girder
   *pbStartCantilever = (*pbStartCantilever || ::IsLT(segment_height_start,start_offset) ? true : false);
   *pbEndCantilever   = (*pbEndCantilever   || ::IsLT(segment_height_end,  end_offset)   ? true : false);
}

void CBridgeAgentImp::LayoutEndSizePoi(const CSegmentKey& segmentKey,Float64 segmentOffset)
{
   Float64 left_end_dist   = GetSegmentStartEndDistance(segmentKey);
   Float64 left_brg_offset = GetSegmentStartBearingOffset(segmentKey);
   Float64 right_end_dist  = GetSegmentEndEndDistance(segmentKey);
   Float64 segment_length  = GetSegmentLength(segmentKey);
   Float64 span_length     = GetSegmentSpanLength(segmentKey); // cl brg to cl brg length

   Float64 start_offset = left_brg_offset - left_end_dist;

   CSegmentKey firstSegmentKey(segmentKey);
   firstSegmentKey.segmentIndex = 0;
   Float64 first_segment_start_offset = GetSegmentStartBearingOffset(firstSegmentKey) - GetSegmentStartEndDistance(firstSegmentKey);

   bool bStartCantilever, bEndCantilever;
   ModelCantilevers(segmentKey,&bStartCantilever,&bEndCantilever);
   if ( bStartCantilever )
   {
      // model 5 poi per cantilever, except the poi spacing can't exceed 
      // the 10th point spacing
      Float64 max_poi_spacing = span_length/10.0;
      IndexType nPoi = 5; // puts poi at quarter points on cantilever, including a poi at start and end
      Float64 poi_spacing = left_end_dist/(nPoi-1);
      if ( max_poi_spacing < poi_spacing )
      {
         nPoi = IndexType(left_end_dist/max_poi_spacing);
         poi_spacing = left_end_dist/(nPoi-1);
      }

      Float64 Xs = 0;
      for ( IndexType poiIdx = 0; poiIdx < nPoi; poiIdx++ )
      {
         Float64 Xsp = Xs + start_offset;
         Float64 Xgp = segmentOffset + Xsp;
         Float64 Xg = Xgp - first_segment_start_offset;
         pgsPointOfInterest poi(segmentKey, Xs, Xsp, Xg, Xgp,POI_SPAN | POI_CANTILEVER);
         poi.SetReferencedAttributes(POI_ERECTED_SEGMENT | POI_CANTILEVER);
         m_pPoiMgr->AddPointOfInterest(poi);
         Xs += poi_spacing;
      }
   }

   if ( bEndCantilever )
   {
      // model 5 poi per cantilever, except the poi spacing can't exceed 
      // the 10th point spacing
      Float64 max_poi_spacing = span_length/10.0;
      IndexType nPoi = 5; // puts poi at quarter points on cantilever, including a poi at start and end
      Float64 poi_spacing = right_end_dist/(nPoi-1);
      if ( max_poi_spacing < poi_spacing )
      {
         nPoi = IndexType(right_end_dist/max_poi_spacing);
         poi_spacing = right_end_dist/(nPoi-1);
      }

      Float64 Xs = segment_length - right_end_dist;
      for ( IndexType poiIdx = 0; poiIdx < nPoi; poiIdx++ )
      {
         Float64 Xsp = Xs + start_offset;
         Float64 Xgp = segmentOffset + Xsp;
         Float64 Xg = Xgp - first_segment_start_offset;
         pgsPointOfInterest poi(segmentKey, Xs, Xsp, Xg, Xgp,POI_SPAN | POI_CANTILEVER);
         poi.SetReferencedAttributes(POI_ERECTED_SEGMENT | POI_CANTILEVER);
         m_pPoiMgr->AddPointOfInterest( poi );
         Xs += poi_spacing;
      }
   }

   // Add a POI at the start and end of every segment, except at the start of the first segment in the first group
   // and the end of the last segment in the last group.
   //
   // POIs are needed at this location because of the way the Anaysis Agent builds the LBAM models. At intermediate piers
   // and closure joints, the "span length" is extended to the end of the segment rather than the CL Bearing.
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   SegmentIndexType nSegments = pGirder->GetSegmentCount();
   if ( !((segmentKey.segmentIndex == 0) && (segmentKey.groupIndex == 0)) )
   {
      Float64 Xs  = 0;
      Float64 Xsp = Xs + start_offset;
      Float64 Xgp = segmentOffset + Xsp;
      Float64 Xg  = Xgp - first_segment_start_offset;
      pgsPointOfInterest poi(segmentKey,Xs,Xsp,Xg,Xgp);
      m_pPoiMgr->AddPointOfInterest(poi);
   }

   if ( !((segmentKey.segmentIndex == nSegments-1) && (segmentKey.groupIndex == nGroups-1)) )
   {
      Float64 Xs  = segment_length;
      Float64 Xsp = Xs + start_offset;
      Float64 Xgp = segmentOffset + Xsp;
      Float64 Xg  = Xgp - first_segment_start_offset;
      pgsPointOfInterest poi(segmentKey,Xs,Xsp,Xg,Xgp);
      m_pPoiMgr->AddPointOfInterest(poi);
   }
}

void CBridgeAgentImp::LayoutHarpingPointPoi(const CSegmentKey& segmentKey,Float64 segmentOffset)
{
   IndexType maxHarped = GetNumHarpPoints(segmentKey);

   // if there can't be any harped strands, then there is no need to use the harping point attribute
   if ( maxHarped == 0 )
   {
      return; 
   }

   Float64 left_end_dist = GetSegmentStartEndDistance(segmentKey);
   Float64 left_brg_offset = GetSegmentStartBearingOffset(segmentKey);
   Float64 segment_length = GetSegmentLength(segmentKey);
   Float64 start_offset = left_brg_offset - left_end_dist;

   CSegmentKey firstSegmentKey(segmentKey);
   firstSegmentKey.segmentIndex = 0;
   Float64 first_segment_start_offset = GetSegmentStartBearingOffset(firstSegmentKey) - GetSegmentStartEndDistance(firstSegmentKey);

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   Float64 hp1, hp2;
   girder->GetHarpingPointLocations( &hp1, &hp2 );

   Float64 Xs  = hp1;
   Float64 Xsp = Xs + start_offset;
   Float64 Xgp = segmentOffset + Xsp;
   Float64 Xg  = Xgp - first_segment_start_offset;
   pgsPointOfInterest poiHP1(segmentKey,Xs,Xsp,Xg,Xgp,POI_HARPINGPOINT);

   Xs  = hp2;
   Xsp = Xs + start_offset;
   Xgp = segmentOffset + Xsp;
   Xg = Xgp - first_segment_start_offset;
   pgsPointOfInterest poiHP2(segmentKey,Xs,Xsp,Xg,Xgp,POI_HARPINGPOINT);


   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   Float64 left_end, right_end, length;
   girder->get_LeftEndDistance(&left_end);
   girder->get_RightEndDistance(&right_end);
   girder->get_GirderLength(&length);
   if ( hp1 <= left_end || (length-right_end) <= hp2 )
   {
      // harp points are outside of the support location
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      std::_tostringstream os;
      os << _T("The harping points for ") << SEGMENT_LABEL(segmentKey)
         << _T(" are located outside of the bearings. You can fix this by increasing the segment length, or")
         << _T(" by changing the harping point location in the girder library entry.")<<std::endl;

      pgsBridgeDescriptionStatusItem* pStatusItem = 
         new pgsBridgeDescriptionStatusItem(m_StatusGroupID,m_scidBridgeDescriptionError,pgsBridgeDescriptionStatusItem::General,os.str().c_str());

      pStatusCenter->Add(pStatusItem);

      os << _T("See Status Center for Details");
      THROW_UNWIND(os.str().c_str(),XREASON_NEGATIVE_GIRDER_LENGTH);
   }

   m_pPoiMgr->AddPointOfInterest( poiHP1 );
   m_pPoiMgr->AddPointOfInterest( poiHP2 );

   // add POI on either side of the harping point. Because the vertical component of prestress, Vp, is zero
   // on one side of the harp point and Vp on the other side there is a jump in shear capacity. Add these
   // poi to pick up the jump.
   poiHP1.Offset( 0.0015);
   poiHP2.Offset(-0.0015);

   poiHP1.SetNonReferencedAttributes(0);
   poiHP2.SetNonReferencedAttributes(0);
   
   m_pPoiMgr->AddPointOfInterest( poiHP1 );
   m_pPoiMgr->AddPointOfInterest( poiHP2 );
}

void CBridgeAgentImp::LayoutPrestressTransferAndDebondPoi(const CSegmentKey& segmentKey,Float64 segmentOffset)
{
   PoiAttributeType attrib_debond = POI_DEBOND;
   PoiAttributeType attrib_xfer   = POI_PSXFER;

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   SegmentIDType segID = pIBridgeDesc->GetSegmentID(segmentKey);

   // remove any current ps-xfer and debond pois
   std::vector<pgsPointOfInterest> poi;
   m_pPoiMgr->GetPointsOfInterest(segmentKey,POI_DEBOND | POI_PSXFER,POIMGR_OR,&poi);
   std::vector<pgsPointOfInterest>::iterator iter(poi.begin());
   std::vector<pgsPointOfInterest>::iterator iterEnd(poi.end());
   for ( ; iter != iterEnd; iter++ )
   {
      m_pPoiMgr->RemovePointOfInterest(*iter);
   }

#if defined _DEBUG
   m_pPoiMgr->GetPointsOfInterest(segmentKey,POI_DEBOND | POI_PSXFER,POIMGR_OR,&poi);
   ATLASSERT(poi.size() == 0);
#endif

   // Add POIs at the prestress transfer length from the ends of the girder
   GET_IFACE(IPretensionForce,pPrestress);
   Float64 xfer_length = pPrestress->GetXferLength(segmentKey,pgsTypes::Permanent);
   
   Float64 left_end_dist = GetSegmentStartEndDistance(segmentKey);
   Float64 left_brg_offset = GetSegmentStartBearingOffset(segmentKey);
   Float64 segment_length = GetSegmentLength(segmentKey);
   Float64 start_offset = left_brg_offset - left_end_dist;

   CSegmentKey firstSegmentKey(segmentKey);
   firstSegmentKey.segmentIndex = 0;
   Float64 first_segment_start_offset = GetSegmentStartBearingOffset(firstSegmentKey) - GetSegmentStartEndDistance(firstSegmentKey);

   Float64 d1 = xfer_length;
   Float64 d2 = segment_length - xfer_length;

   Float64 Xs  = d1;
   Float64 Xsp = Xs + start_offset;
   Float64 Xgp = segmentOffset + Xsp;
   Float64 Xg  = Xgp - first_segment_start_offset;
   pgsPointOfInterest poiXfer1(segmentKey,Xs,Xsp,Xg,Xgp,attrib_xfer);
   m_pPoiMgr->AddPointOfInterest( poiXfer1 );

   Xs  = d2;
   Xsp = Xs + start_offset;
   Xgp = segmentOffset + Xsp;
   Xg  = Xgp - first_segment_start_offset;
   pgsPointOfInterest poiXfer2(segmentKey,Xs,Xsp,Xg,Xgp,attrib_xfer);
   m_pPoiMgr->AddPointOfInterest( poiXfer2 );

   ////////////////////////////////////////////////////////////////
   // debonded strands
   ////////////////////////////////////////////////////////////////

   GET_IFACE(ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   for ( Uint16 i = 0; i < 3; i++ )
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      const std::vector<CDebondData>& vDebond(pStrands->GetDebonding(strandType));
      std::vector<CDebondData>::const_iterator iter(vDebond.begin());
      std::vector<CDebondData>::const_iterator end(vDebond.end());
      for ( ; iter != end; iter++ )
      {
         const CDebondData& debond_info = *iter;

         d1 = debond_info.Length[pgsTypes::metStart];
         d2 = d1 + xfer_length;

         // only add POI if debond and transfer point are on the girder
         if ( d1 < segment_length && d2 < segment_length )
         {
            Xs  = d1;
            Xsp = Xs + start_offset;
            Xgp = segmentOffset + Xsp;
            Xg  = Xgp - first_segment_start_offset;
            pgsPointOfInterest poiDBD(segmentKey,Xs,Xsp,Xg,Xgp,attrib_debond);
            m_pPoiMgr->AddPointOfInterest( poiDBD );

            Xs  = d2;
            Xsp = Xs + start_offset;
            Xgp = segmentOffset + Xsp;
            Xg  = Xgp - first_segment_start_offset;
            pgsPointOfInterest poiXFR(segmentKey,Xs,Xsp,Xg,Xgp,attrib_xfer);
            m_pPoiMgr->AddPointOfInterest( poiXFR );
         }

         d1 = segment_length - debond_info.Length[pgsTypes::metEnd];
         d2 = d1 - xfer_length;
         // only add POI if debond and transfer point are on the girder
         if ( 0 < d1 && 0 < d2 )
         {
            Xs  = d1;
            Xsp = Xs + start_offset;
            Xgp = segmentOffset + Xsp;
            Xg  = Xgp - first_segment_start_offset;
            pgsPointOfInterest poiDBD(segmentKey,Xs,Xsp,Xg,Xgp,attrib_debond);
            m_pPoiMgr->AddPointOfInterest( poiDBD );

            Xs  = d2;
            Xsp = Xs + start_offset;
            Xgp = segmentOffset + Xsp;
            Xg  = Xgp - first_segment_start_offset;
            pgsPointOfInterest poiXFR(segmentKey,Xs,Xsp,Xg,Xgp,attrib_xfer);
            m_pPoiMgr->AddPointOfInterest( poiXFR );
         }
      }
   }
}

void CBridgeAgentImp::LayoutPoiForPrecastDiaphragmLoads(const CSegmentKey& segmentKey,Float64 segmentOffset)
{
   // we want to capture "jumps" due to diaphragm loads in the graphical displays
   Float64 left_end_dist = GetSegmentStartEndDistance(segmentKey);
   Float64 left_brg_offset = GetSegmentStartBearingOffset(segmentKey);
   Float64 start_offset = left_brg_offset - left_end_dist;

   CSegmentKey firstSegmentKey(segmentKey);
   firstSegmentKey.segmentIndex = 0;
   Float64 first_segment_start_offset = GetSegmentStartBearingOffset(firstSegmentKey) - GetSegmentStartEndDistance(firstSegmentKey);

   // layout for diaphragms that are built in the casting yard
   std::vector<IntermedateDiaphragm> pcDiaphragms( GetPrecastDiaphragms(segmentKey) );
   std::vector<IntermedateDiaphragm>::iterator iter(pcDiaphragms.begin());
   std::vector<IntermedateDiaphragm>::iterator end(pcDiaphragms.end());
   for ( ; iter != end; iter++ )
   {
      IntermedateDiaphragm& diaphragm = *iter;

      Float64 Xs  = diaphragm.Location; // location in POI coordinates
      Float64 Xsp = start_offset  + Xs; // location in segment path coordinates
      Float64 Xgp = segmentOffset + Xsp;  // location in girder path coordinates
      Float64 Xg  = Xgp - first_segment_start_offset;   // location in girder coordinates
      pgsPointOfInterest poi( segmentKey, Xs, Xsp, Xg, Xgp, POI_DIAPHRAGM);
      m_pPoiMgr->AddPointOfInterest( poi );
   }
}

void CBridgeAgentImp::LayoutPoiForIntermediateDiaphragmLoads(const CSpanKey& spanKey)
{
   // we want to capture "jumps" due to diaphragm loads in the graphical displays
   // get loads for bridge site 
   std::vector<IntermedateDiaphragm> cipDiaphragms( GetCastInPlaceDiaphragms(spanKey) );
   std::vector<IntermedateDiaphragm>::iterator iter(cipDiaphragms.begin());
   std::vector<IntermedateDiaphragm>::iterator end(cipDiaphragms.end());
   for ( ; iter != end; iter++ )
   {
      IntermedateDiaphragm& diaphragm = *iter;

      Float64 Xspan = diaphragm.Location;

      pgsPointOfInterest poi = ConvertSpanPointToPoi(spanKey,Xspan);
      poi.SetNonReferencedAttributes(POI_DIAPHRAGM);

      m_pPoiMgr->AddPointOfInterest( poi );
   }
}

void CBridgeAgentImp::LayoutPoiForShear(const CSegmentKey& segmentKey,Float64 segmentOffset)
{
   // Layout POI at H, 1.5H, 2.5H, and FaceOfSupport from CL-Brg for piers that occur at the
   // ends of a segment (See LayoutPoiForPier for shear POIs that occur near
   // intermediate supports for segments that span over a pier)

   Float64 start_end_dist   = GetSegmentStartEndDistance(segmentKey);
   Float64 end_end_dist     = GetSegmentEndEndDistance(segmentKey);
   Float64 start_brg_offset = GetSegmentStartBearingOffset(segmentKey);
   Float64 end_brg_offset   = GetSegmentEndBearingOffset(segmentKey);
   Float64 start_offset     = start_brg_offset - start_end_dist;
   Float64 segment_length   = GetSegmentLength(segmentKey);

   CSegmentKey firstSegmentKey(segmentKey);
   firstSegmentKey.segmentIndex = 0;
   Float64 first_segment_start_offset = GetSegmentStartBearingOffset(firstSegmentKey) - GetSegmentStartEndDistance(firstSegmentKey);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   const CPierData2* pPier;
   const CTemporarySupportData* pTS;
   pSegment->GetSupport(pgsTypes::metStart,&pPier,&pTS);

   if ( pPier )
   {
      // this is a pier at the start of this segment
      Float64 XsCLPier;
      GetPierLocation(pPier->GetIndex(),segmentKey,&XsCLPier); // location of CL pier in segment coordinates
      Float64 XsCLBrg = XsCLPier + start_brg_offset; // location of CL brg in segment coordinates
      Float64 Hg = GetHeight(pgsPointOfInterest(segmentKey,XsCLBrg));

      // NOTE: Assuming that the support is symmetric about the CL Bearing
      // Distance from CL Brg to face of support is taken to be support_width/2
      Float64 support_width = GetSegmentStartSupportWidth(segmentKey);

      // If "H" from the end of the girder is at the point of bearing
      // make sure there isn't any "noise" in the data
      if ( IsEqual(Hg,start_end_dist) )
      {
         Hg = start_end_dist;
      }

      Float64 Xs, Xsp, Xg, Xgp;

      // POI between FOS and 1.5H for purposes of computing critical section
      Xs  = start_end_dist + /*support_width/2 +*/ 0.75*Hg; // support width was not used in Version 2.x
      Xsp = Xs + start_offset;
      Xgp = segmentOffset + Xsp;
      Xg  = Xgp - first_segment_start_offset;
      pgsPointOfInterest poi_075h( segmentKey, Xs, Xsp, Xg, Xgp);
      m_pPoiMgr->AddPointOfInterest(poi_075h);

      Xs  = start_end_dist + support_width/2 + Hg;
      Xsp = Xs + start_offset;
      Xgp = segmentOffset + Xsp;
      Xg  = Xgp - first_segment_start_offset;
      pgsPointOfInterest poi_h( segmentKey, Xs, Xsp, Xg, Xgp, POI_H);
      m_pPoiMgr->AddPointOfInterest(poi_h);

      Xs  = start_end_dist + support_width/2 + 1.5*Hg;
      Xsp = Xs + start_offset;
      Xgp = segmentOffset + Xsp;
      Xg  = Xgp - first_segment_start_offset;
      pgsPointOfInterest poi_15h( segmentKey, Xs, Xsp, Xg, Xgp, POI_15H);
      m_pPoiMgr->AddPointOfInterest(poi_15h);

      Xs  = start_end_dist + support_width/2;
      Xsp = Xs + start_offset;
      Xgp = segmentOffset + Xsp;
      Xg  = Xgp - first_segment_start_offset;
      pgsPointOfInterest poi_fos( segmentKey, Xs, Xsp, Xg, Xgp, POI_FACEOFSUPPORT);
      m_pPoiMgr->AddPointOfInterest(poi_fos);
   }

   pSegment->GetSupport(pgsTypes::metEnd,&pPier,&pTS);
   if ( pPier )
   {
      // this is a pier at the end of this segment
      Float64 XsCLPier;
      GetPierLocation(pPier->GetIndex(),segmentKey,&XsCLPier); // CL pier in segment coordinates
      Float64 XsCLBrg = XsCLPier - end_brg_offset; // CL brg in segment coordinates
      ATLASSERT( ::IsLE(XsCLBrg,segment_length) );

      Float64 Hg = GetHeight(pgsPointOfInterest(segmentKey,XsCLBrg));

      Float64 support_width = GetSegmentEndSupportWidth(segmentKey);

      // If "H" from the end of the girder is at the point of bearing
      // make sure there isn't any "noise" in the data
      if ( IsEqual(Hg,end_end_dist) )
      {
         Hg = end_end_dist;
      }

      Float64 Xs, Xsp, Xg, Xgp;
      // add a POI at 0.75H for purposes of computing critical section
      Xs  = segment_length - (end_end_dist + /*support_width/2 +*/ 0.75*Hg); // support width was not used in Version 2.x
      Xsp = Xs + start_offset;
      Xgp = Xsp + segmentOffset;
      Xg  = Xgp - first_segment_start_offset;
      pgsPointOfInterest poi_075h( segmentKey, Xs, Xsp, Xg, Xgp);
      m_pPoiMgr->AddPointOfInterest(poi_075h);

      Xs  = segment_length - (end_end_dist + support_width/2 + Hg);
      Xsp = Xs + start_offset;
      Xgp = Xsp + segmentOffset;
      Xg  = Xgp - first_segment_start_offset;
      pgsPointOfInterest poi_h( segmentKey, Xs, Xsp, Xg, Xgp, POI_H);
      m_pPoiMgr->AddPointOfInterest(poi_h);

      Xs  = segment_length - (end_end_dist + support_width/2 + 1.5*Hg);
      Xsp = Xs + start_offset;
      Xgp = Xsp + segmentOffset;
      Xg  = Xgp - first_segment_start_offset;
      pgsPointOfInterest poi_15h( segmentKey, Xs, Xsp, Xg, Xgp, POI_15H);
      m_pPoiMgr->AddPointOfInterest(poi_15h);

      Xs  = segment_length - (end_end_dist + support_width/2);
      Xsp = Xs + start_offset;
      Xgp = Xsp + segmentOffset;
      Xg  = Xgp - first_segment_start_offset;
      pgsPointOfInterest poi_fos( segmentKey, Xs, Xsp, Xg, Xgp, POI_FACEOFSUPPORT);
      m_pPoiMgr->AddPointOfInterest(poi_fos);
   }


   // POI's at stirrup zone boundaries
   Float64 end_support_loc = segment_length - end_end_dist;
   Float64 midLen = segment_length/2.0;

   ZoneIndexType nZones = GetPrimaryZoneCount(segmentKey);
   for (ZoneIndexType zoneIdx = 1; zoneIdx < nZones; zoneIdx++) // note that count starts at one
   {
      Float64 zStart, zEnd;
      GetPrimaryZoneBounds(segmentKey, zoneIdx, &zStart, &zEnd);

      // Nudge poi toward mid-span as this is where smaller Av/s will typically lie
      zStart += (zStart < midLen ? 0.001 : -0.001);

      if (start_end_dist < zStart && zStart < end_support_loc)
      {
         Float64 Xs, Xsp, Xg, Xgp;
         Xs  = zStart;
         Xsp = Xs + start_offset;
         Xgp = Xsp + segmentOffset;
         Xg  = Xgp - first_segment_start_offset;

         pgsPointOfInterest poi(segmentKey, Xs, Xsp, Xg, Xgp, POI_STIRRUP_ZONE);
         m_pPoiMgr->AddPointOfInterest(poi);
      }
   }
}

void CBridgeAgentImp::LayoutPoiForSlabBarCutoffs(const CGirderKey& girderKey)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckRebarData& rebarData = pBridgeDesc->GetDeckDescription()->DeckRebarData;

   CComPtr<IGeomUtil2d> geomUtil;
   geomUtil.CoCreateInstance(CLSID_GeomUtil);

   PierIndexType startPierIdx, endPierIdx;
   GetGirderGroupPiers(girderKey.groupIndex,&startPierIdx,&endPierIdx);
   for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
   {
      std::vector<CDeckRebarData::NegMomentRebarData> vSupplementalRebarData(rebarData.GetSupplementalReinforcement(pierIdx));
      BOOST_FOREACH(CDeckRebarData::NegMomentRebarData& nmRebarData,vSupplementalRebarData)
      {
         CComPtr<IPierLine> pierLine;
         GetPierLine(pierIdx,&pierLine);

         CComPtr<ILine2d> centerlinePier;
         pierLine->get_Centerline(&centerlinePier);

         CComPtr<IAngle> objSkewAngle;
         pierLine->get_Skew(&objSkewAngle);
         Float64 skewAngle;
         objSkewAngle->get_Value(&skewAngle);

         if ( pierIdx != endPierIdx )
         {
            // on ahead side of pier... do this for all pier except that last one in this group
            // the ahead side doesn't exist if this is the last group or it will be handled
            // when the next group is processed

            Float64 offset = nmRebarData.RightCutoff*cos(skewAngle);

            CComPtr<ILine2d> pAheadLine;
            geomUtil->CreateParallelLine(centerlinePier,offset,&pAheadLine);

            SpanIndexType spanIdx = (SpanIndexType)pierIdx;
            CSpanKey spanKey(spanIdx,girderKey.girderIndex);
            Float64 Xspan = nmRebarData.RightCutoff;

            // this is the poi measuring the right cutoff along the CL girder... this
            // we don't actually want to do this. we want to measure bar cutoff
            // in the direction of the alignment at the pier.
            // This poi is near the one we want so it makes finding the actual location quicker
            pgsPointOfInterest poi = ConvertSpanPointToPoi(spanKey,Xspan);

            Float64 Xpoi;
            SegmentIndexType segIdx;
            VERIFY(GirderLineIntersect(girderKey,pAheadLine,poi.GetSegmentKey().segmentIndex,&segIdx,&Xpoi));
            poi.SetSegmentKey(CSegmentKey(girderKey,segIdx));
            poi.SetDistFromStart(Xpoi);
            poi.SetID(INVALID_ID);
            poi.ClearAttributes();
            poi.SetNonReferencedAttributes(POI_DECKBARCUTOFF);
            m_pPoiMgr->AddPointOfInterest( poi );

            // put a POI just after the bar cutoff so we capture jumps in capacity
            poi.ClearAttributes();
            poi.SetDistFromStart(poi.GetDistFromStart()+DECK_REBAR_OFFSET);
            m_pPoiMgr->AddPointOfInterest( poi );
         }

         if ( pierIdx != startPierIdx )
         {
            // on back side of pier... do this for all pier except for first one in this group
            // the back side doesn't exist if this is the first group or it was handled
            // when the previous group was processed

            Float64 offset = nmRebarData.LeftCutoff*cos(skewAngle);

            CComPtr<ILine2d> pBackLine;
            geomUtil->CreateParallelLine(centerlinePier,-offset,&pBackLine);

            SpanIndexType spanIdx = SpanIndexType(pierIdx-1);
            CSpanKey spanKey(spanIdx,girderKey.girderIndex);
            Float64 spanLength = GetSpanLength(spanKey);
            Float64 Xspan = spanLength - nmRebarData.LeftCutoff;

            pgsPointOfInterest poi = ConvertSpanPointToPoi(spanKey,Xspan);
            Float64 Xpoi;
            SegmentIndexType segIdx;
            VERIFY(GirderLineIntersect(girderKey,pBackLine,poi.GetSegmentKey().segmentIndex,&segIdx,&Xpoi));
            poi.SetSegmentKey(CSegmentKey(girderKey,segIdx));
            poi.SetDistFromStart(Xpoi);

            poi.SetID(INVALID_ID);
            poi.ClearAttributes();
            poi.SetNonReferencedAttributes(POI_DECKBARCUTOFF);
            m_pPoiMgr->AddPointOfInterest( poi );

            // put a POI just before the bar cutoff so we capture jumps in capacity
            poi.ClearAttributes();
            poi.SetDistFromStart(poi.GetDistFromStart()-DECK_REBAR_OFFSET);
            m_pPoiMgr->AddPointOfInterest( poi );
         }
      }
   }
}

void CBridgeAgentImp::LayoutPoiForSegmentBarCutoffs(const CSegmentKey& segmentKey,Float64 segmentOffset)
{
   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   Float64 segment_length = GetSegmentLength(segmentKey);
   Float64 left_brg_loc  = GetSegmentStartEndDistance(segmentKey);
   Float64 right_brg_loc = segment_length - GetSegmentEndEndDistance(segmentKey);

   // TRICKY: Move bearing locations slightly so bar cutoffs directly over supports get picked up and tagged correctly
   left_brg_loc  -= 1.0e-06;
   right_brg_loc += 1.0e-06;

   Float64 fc = GetSegmentFc28(segmentKey);
   pgsTypes::ConcreteType concType = GetSegmentConcreteType(segmentKey);
   bool hasFct = DoesSegmentConcreteHaveAggSplittingStrength(segmentKey);
   Float64 Fct = hasFct ? GetSegmentConcreteAggSplittingStrength(segmentKey) : 0.0;

   PoiAttributeType cut_attribute = POI_BARCUTOFF;
   PoiAttributeType dev_attribute = POI_BARDEVELOP;

   CComPtr<IRebarLayout> rebar_layout;
   girder->get_RebarLayout(&rebar_layout);

   CollectionIndexType nRebarLayoutItems;
   rebar_layout->get_Count(&nRebarLayoutItems);
   for (CollectionIndexType rebarLayoutItemIdx = 0; rebarLayoutItemIdx < nRebarLayoutItems; rebarLayoutItemIdx++)
   {
      CComPtr<IRebarLayoutItem> rebarLayoutItem;
      rebar_layout->get_Item(rebarLayoutItemIdx, &rebarLayoutItem);

      Float64 startLoc, barLength;
      rebarLayoutItem->get_Start(&startLoc);
      rebarLayoutItem->get_Length(&barLength);
      Float64 endLoc = startLoc + barLength;

      if ( segment_length <= startLoc )
      {
         // NOTE: if we decide to put dev length POI in closure joint, then
         // fc used to compute closure joint below needs to be re-examined... it is currently fc for the girder

         // bar starts after the end of the segment... bar is in the closure joint..
         // NO BAR CUTOFF OR DEVELOPMENT POI IN THE CLOSURE JOINT
         // ASSUMING CLOSURE JOINT BARS ARE FULLY DEVELOPED
         continue;
      }

      if (segment_length < endLoc)
      {
         ATLASSERT(false); // probably should never happen
         endLoc = segment_length;
      }

      // Add pois at cutoffs if they are within bearing locations
      if (left_brg_loc < startLoc && startLoc < right_brg_loc)
      {
         m_pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey, startLoc, cut_attribute) );
      }

      if (left_brg_loc < endLoc && endLoc < right_brg_loc)
      {
         m_pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey, endLoc, cut_attribute) );
      }

      // we only create one pattern per layout
      CollectionIndexType nRebarPatterns;
      rebarLayoutItem->get_Count(&nRebarPatterns);
      ATLASSERT(nRebarPatterns==1);
      if (0 < nRebarPatterns)
      {
         CComPtr<IRebarPattern> rebarPattern;
         rebarLayoutItem->get_Item(0, &rebarPattern);

         CComPtr<IRebar> rebar;
         rebarPattern->get_Rebar(&rebar);

         if (rebar)
         {
            // Get development length and add poi only if dev length is shorter than 1/2 rebar length
            REBARDEVLENGTHDETAILS devDetails = GetRebarDevelopmentLengthDetails(rebar, concType, fc, hasFct, Fct);
            Float64 ldb = devDetails.ldb;
            if (ldb < barLength/2.0)
            {
               startLoc += ldb;
               endLoc -= ldb;

               if (left_brg_loc < startLoc && startLoc < right_brg_loc)
               {
                  m_pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey, startLoc, dev_attribute) );
               }

               if (left_brg_loc < endLoc && endLoc < right_brg_loc)
               {
                  m_pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey, endLoc,   dev_attribute) );
               }
            }
         }
         else
         {
            ATLASSERT(false);
         }
      }
   }
}

void CBridgeAgentImp::LayoutPoiForHandling(const CSegmentKey& segmentKey)
{
   GET_IFACE(ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
   if (pSegmentLiftingSpecCriteria->IsLiftingAnalysisEnabled())
   {
      LayoutLiftingPoi(segmentKey,10); // puts poi at 10th points
   }

   GET_IFACE(ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria);
   if ( pSegmentHaulingSpecCriteria->IsHaulingAnalysisEnabled() )
   {
      LayoutHaulingPoi(segmentKey,10);
   }
}

void CBridgeAgentImp::LayoutPoiForSectionChanges(const CSegmentKey& segmentKey)
{
   const GirderLibraryEntry* pGirderEntry = GetGirderLibraryEntry(segmentKey);
   CComPtr<IBeamFactory> beamFactory;
   pGirderEntry->GetBeamFactory(&beamFactory);

   beamFactory->LayoutSectionChangePointsOfInterest(m_pBroker,segmentKey,m_pPoiMgr.get());
}

void CBridgeAgentImp::LayoutPoiForPiers(const CSegmentKey& segmentKey)
{
   // puts a POI at piers that are located between the ends of a segment
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData*    pGroup      = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData*  pGirder     = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment    = pGirder->GetSegment(segmentKey.segmentIndex);

   const CSpanData2* pStartSpan = pSegment->GetSpan(pgsTypes::metStart);
   const CSpanData2* pEndSpan   = pSegment->GetSpan(pgsTypes::metEnd);

   // collect all the piers that are between the ends of this segment
   std::vector<const CPierData2*> piers;
   const CPierData2* pPier = pStartSpan->GetNextPier();
   while ( pPier != pEndSpan->GetNextPier() )
   {
      piers.push_back(pPier);

      pPier = pPier->GetNextSpan()->GetNextPier();
   }

   if ( 0 < piers.size() )
   {
      std::vector<const CPierData2*>::iterator iter(piers.begin());
      std::vector<const CPierData2*>::iterator iterEnd(piers.end());
      for ( ; iter != iterEnd; iter++ )
      {
         const CPierData2* pPier = *iter;

         Float64 Xpoi;
         VERIFY(GetPierLocation(pPier->GetIndex(),segmentKey,&Xpoi));
         pgsPointOfInterest poi(segmentKey,Xpoi,POI_INTERMEDIATE_PIER);
         m_pPoiMgr->AddPointOfInterest(poi);

         // POI at h and 1.5h from cl-brg (also at 2.5h for purposes of computing critical section)
         Float64 Hg = GetHeight(poi);

         // left side of pier
         Float64 location;
         CSegmentKey thisSegmentKey(segmentKey);
         location = Xpoi - Hg;
         if ( location < 0 )
         {
            // poi falls in previous segment
            ATLASSERT(thisSegmentKey.segmentIndex != 0);
            thisSegmentKey.segmentIndex--;
            Float64 Ls  = GetSegmentLength(thisSegmentKey);
            Float64 BOl = GetSegmentEndBearingOffset(thisSegmentKey);
            Float64 EDl = GetSegmentEndEndDistance(thisSegmentKey);
            Float64 BOr = GetSegmentStartBearingOffset(segmentKey);
            Float64 EDr = GetSegmentStartEndDistance(segmentKey);
            location = Ls + (BOl - EDl) + (BOr - EDr) + location;
         }
         pgsPointOfInterest left_H(thisSegmentKey,location,POI_H);
         m_pPoiMgr->AddPointOfInterest(left_H);

         thisSegmentKey = segmentKey;
         location = Xpoi - 1.5*Hg;
         if ( location < 0 )
         {
            // poi falls in previous segment
            ATLASSERT(thisSegmentKey.segmentIndex != 0);
            thisSegmentKey.segmentIndex--;
            Float64 Ls  = GetSegmentLength(thisSegmentKey);
            Float64 BOl = GetSegmentEndBearingOffset(thisSegmentKey);
            Float64 EDl = GetSegmentEndEndDistance(thisSegmentKey);
            Float64 BOr = GetSegmentStartBearingOffset(segmentKey);
            Float64 EDr = GetSegmentStartEndDistance(segmentKey);
            location = Ls + (BOl - EDl) + (BOr - EDr) + location;
         }
         pgsPointOfInterest left_15H(thisSegmentKey,location,POI_15H);
         m_pPoiMgr->AddPointOfInterest(left_15H);

         thisSegmentKey = segmentKey;
         location = Xpoi - 2.5*Hg;
         if ( location < 0 )
         {
            // poi falls in previous segment
            ATLASSERT(thisSegmentKey.segmentIndex != 0);
            thisSegmentKey.segmentIndex--;
            Float64 Ls  = GetSegmentLength(thisSegmentKey);
            Float64 BOl = GetSegmentEndBearingOffset(thisSegmentKey);
            Float64 EDl = GetSegmentEndEndDistance(thisSegmentKey);
            Float64 BOr = GetSegmentStartBearingOffset(segmentKey);
            Float64 EDr = GetSegmentStartEndDistance(segmentKey);
            location = Ls + (BOl - EDl) + (BOr - EDr) + location;
         }
         pgsPointOfInterest left_25H(thisSegmentKey,location);
         m_pPoiMgr->AddPointOfInterest(left_25H);

         // right side of pier
         Float64 segment_length = GetSegmentLength(segmentKey);
         thisSegmentKey = segmentKey;
         location = Xpoi + Hg;
         if ( segment_length < location )
         {
            // poi falls in next segment
            thisSegmentKey.segmentIndex++;
            Float64 BOl = GetSegmentEndBearingOffset(segmentKey);
            Float64 EDl = GetSegmentEndEndDistance(segmentKey);
            Float64 BOr = GetSegmentStartBearingOffset(thisSegmentKey);
            Float64 EDr = GetSegmentStartEndDistance(thisSegmentKey);
            Float64 dist_between_segments = (BOl - EDl) + (BOr - EDr);
            location = location - segment_length - dist_between_segments;
         }
         pgsPointOfInterest right_H(thisSegmentKey,location,POI_H);
         m_pPoiMgr->AddPointOfInterest(right_H);

         thisSegmentKey = segmentKey;
         location = Xpoi + 1.5*Hg;
         if ( segment_length < location )
         {
            // poi falls in next segment
            thisSegmentKey.segmentIndex++;
            Float64 BOl = GetSegmentEndBearingOffset(segmentKey);
            Float64 EDl = GetSegmentEndEndDistance(segmentKey);
            Float64 BOr = GetSegmentStartBearingOffset(thisSegmentKey);
            Float64 EDr = GetSegmentStartEndDistance(thisSegmentKey);
            Float64 dist_between_segments = (BOl - EDl) + (BOr - EDr);
            location = location - segment_length - dist_between_segments;
         }
         pgsPointOfInterest right_15H(thisSegmentKey,location,POI_15H);
         m_pPoiMgr->AddPointOfInterest(right_15H);

         thisSegmentKey = segmentKey;
         location = Xpoi + 2.5*Hg;
         if ( segment_length < location )
         {
            // poi falls in next segment
            thisSegmentKey.segmentIndex++;
            Float64 BOl = GetSegmentEndBearingOffset(segmentKey);
            Float64 EDl = GetSegmentEndEndDistance(segmentKey);
            Float64 BOr = GetSegmentStartBearingOffset(thisSegmentKey);
            Float64 EDr = GetSegmentStartEndDistance(thisSegmentKey);
            Float64 dist_between_segments = (BOl - EDl) + (BOr - EDr);
            location = location - segment_length - dist_between_segments;
         }
         pgsPointOfInterest right_25H(thisSegmentKey,location);
         m_pPoiMgr->AddPointOfInterest(right_25H);

         // Add POIs for face of support
         Float64 left_support_width  = pPier->GetSupportWidth(pgsTypes::Back);
         Float64 right_support_width = pPier->GetSupportWidth(pgsTypes::Ahead);

         pgsPointOfInterest leftFOS(segmentKey,Xpoi - left_support_width,POI_FACEOFSUPPORT);
         m_pPoiMgr->AddPointOfInterest(leftFOS);

         if ( !IsZero(left_support_width) && !IsZero(right_support_width) )
         {
            pgsPointOfInterest rightFOS(segmentKey,Xpoi + right_support_width,POI_FACEOFSUPPORT);
            m_pPoiMgr->AddPointOfInterest(rightFOS);
         }
      } // next pier
   }

   // add POI at centerline of pier between groups
   if ( pSegment->GetEndClosure() == NULL )
   {
      const CPierData2* pPier;
      const CTemporarySupportData* pTS;
      pSegment->GetSupport(pgsTypes::metEnd,&pPier,&pTS);
      ATLASSERT(pPier != NULL);
      ATLASSERT(pTS == NULL);

      if ( pPier->GetNextSpan() )
      {
         Float64 Xs; // pier is in segment coordinates
         GetPierLocation(pPier->GetIndex(),segmentKey,&Xs);
         pgsPointOfInterest poi(segmentKey,Xs,POI_BOUNDARY_PIER);
         m_pPoiMgr->AddPointOfInterest(poi);
      }
   }
}

void CBridgeAgentImp::LayoutPoiForTemporarySupports(const CSegmentKey& segmentKey)
{
   // puts a POI at temporary supports that are located between the ends of a segment
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData*    pGroup      = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData*  pGirder     = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment    = pGirder->GetSegment(segmentKey.segmentIndex);

   const CSpanData2* pStartSpan = pSegment->GetSpan(pgsTypes::metStart);
   const CSpanData2* pEndSpan   = pSegment->GetSpan(pgsTypes::metEnd);

   std::vector<const CTemporarySupportData*> tempSupports(pStartSpan->GetTemporarySupports());
   std::vector<const CTemporarySupportData*> endTempSupports(pEndSpan->GetTemporarySupports());
   tempSupports.insert(tempSupports.begin(),endTempSupports.begin(),endTempSupports.end());

   if ( tempSupports.size() == 0 )
   {
      return; // no temporary supports
   }

   Float64 segment_start_station, segment_end_station;
   pSegment->GetStations(&segment_start_station,&segment_end_station);
   std::vector<const CTemporarySupportData*>::iterator iter(tempSupports.begin());
   std::vector<const CTemporarySupportData*>::iterator iterEnd(tempSupports.end());
   for ( ; iter != iterEnd; iter++ )
   {
      const CTemporarySupportData* pTS = *iter;
      Float64 ts_station = pTS->GetStation();
      if ( ::IsEqual(segment_start_station,ts_station) || ::IsEqual(segment_end_station,ts_station) )
      {
         continue; // temporary support is at end of segment... we are creating POIs for intermediate temporary supports
      }

      if ( ::InRange(segment_start_station,ts_station,segment_end_station) )
      {
         Float64 Xpoi;
         VERIFY(GetTemporarySupportLocation(pTS->GetIndex(),segmentKey,&Xpoi));
         pgsPointOfInterest poi(segmentKey,Xpoi,POI_INTERMEDIATE_TEMPSUPPORT);

         m_pPoiMgr->AddPointOfInterest(poi);
      }
   } // next temporary support
}

void CBridgeAgentImp::LayoutPoiForTendons(const CGirderKey& girderKey)
{
   GET_IFACE(IBridgeDescription,pBridgeDesc);
   const CPTData* pPTData = pBridgeDesc->GetPostTensioning(girderKey);
   DuctIndexType nDucts = pPTData->GetDuctCount();
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      const CDuctData* pDuct = pPTData->GetDuct(ductIdx);
      switch(pDuct->DuctGeometryType)
      {
      case CDuctGeometry::Linear:
         LayoutPoiForTendon(girderKey,pDuct->LinearDuctGeometry);
         break;
      case CDuctGeometry::Parabolic:
         LayoutPoiForTendon(girderKey,pDuct->ParabolicDuctGeometry);
         break;
      case CDuctGeometry::Offset:
         LayoutPoiForTendon(girderKey,pDuct->OffsetDuctGeometry);
         break;
      default:
         ATLASSERT(false); // is there a new one?
         break;
      }
   }
}

void CBridgeAgentImp::LayoutPoiForTendon(const CGirderKey& girderKey,const CLinearDuctGeometry& ductGeometry)
{
   Float64 Lg = GetGirderLength(girderKey);
   CLinearDuctGeometry::MeasurementType measurementType = ductGeometry.GetMeasurementType();
   Float64 Xg = 0;
   IndexType nPoints = ductGeometry.GetPointCount();
   for  ( IndexType ptIdx = 0; ptIdx < nPoints; ptIdx++ )
   {
      Float64 location,offset;
      CDuctGeometry::OffsetType offsetType;
      ductGeometry.GetPoint(ptIdx,&location,&offset,&offsetType);
      if ( measurementType == CLinearDuctGeometry::AlongGirder )
      {
         if ( location < 0 )
         {
            Xg = -location*Lg;
         }
         else
         {
            Xg = location;
         }
      }
      else
      {
         Xg += location;
      }

      pgsPointOfInterest poi = ConvertGirderCoordinateToPoi(girderKey,Xg);
      poi.SetReferencedAttributes(0);
      poi.SetNonReferencedAttributes(0);
      m_pPoiMgr->AddPointOfInterest(poi);
   }
}

void CBridgeAgentImp::LayoutPoiForTendon(const CGirderKey& girderKey,const CParabolicDuctGeometry& ductGeometry)
{
   // Do we need POI at key points along a parabolic tendon?
}

void CBridgeAgentImp::LayoutPoiForTendon(const CGirderKey& girderKey,const COffsetDuctGeometry& ductGeometry)
{
   // offset ducts not supported yet
   ATLASSERT(false); // not supported yet
}

//-----------------------------------------------------------------------------
void CBridgeAgentImp::LayoutLiftingPoi(const CSegmentKey& segmentKey,Uint16 nPnts)
{
   // lifting
   LayoutHandlingPoi(GetLiftSegmentInterval(segmentKey),segmentKey,
                     nPnts, 
                     0,
                     m_pPoiMgr.get()); 
                     
}

//-----------------------------------------------------------------------------
void CBridgeAgentImp::LayoutHaulingPoi(const CSegmentKey& segmentKey,Uint16 nPnts)
{
   // hauling
   LayoutHandlingPoi(GetHaulSegmentInterval(segmentKey),segmentKey,
                     nPnts, 
                     0,
                     m_pPoiMgr.get());
}

//-----------------------------------------------------------------------------
void CBridgeAgentImp::LayoutHandlingPoi(IntervalIndexType intervalIdx,const CSegmentKey& segmentKey,
                                        Uint16 nPnts, 
                                        PoiAttributeType attrib,
                                        pgsPoiMgr* pPoiMgr)
{
   ATLASSERT(intervalIdx == GetLiftSegmentInterval(segmentKey) || intervalIdx == GetHaulSegmentInterval(segmentKey));
   PoiAttributeType supportAttribute, poiReference;
   Float64 leftOverhang, rightOverhang;
   if (intervalIdx == GetLiftSegmentInterval(segmentKey))
   {
      GET_IFACE(ISegmentLifting, pSegmentLifting);
      leftOverhang  = pSegmentLifting->GetLeftLiftingLoopLocation(segmentKey);
      rightOverhang = pSegmentLifting->GetRightLiftingLoopLocation(segmentKey);

      supportAttribute = POI_PICKPOINT;
      poiReference = POI_LIFT_SEGMENT;
   }
   else if (intervalIdx == GetHaulSegmentInterval(segmentKey))
   {
      GET_IFACE(ISegmentHauling, pSegmentHauling);
      leftOverhang  = pSegmentHauling->GetTrailingOverhang(segmentKey);
      rightOverhang = pSegmentHauling->GetLeadingOverhang(segmentKey);

      supportAttribute = POI_BUNKPOINT;
      poiReference = POI_HAUL_SEGMENT;
   }
   else
   {
      ATLASSERT(false); // you sent in the wrong interval index
   }

   LayoutHandlingPoi(segmentKey,nPnts,leftOverhang,rightOverhang,attrib,supportAttribute,poiReference,pPoiMgr);


#if defined _DEBUG
   // make sure there are two support POI
   std::vector<pgsPointOfInterest> vPoi;
   pPoiMgr->GetPointsOfInterest(segmentKey,poiReference | supportAttribute,POIMGR_AND,&vPoi);
   ATLASSERT(vPoi.size() == 2);
#endif
}

//-----------------------------------------------------------------------------
void CBridgeAgentImp::LayoutHandlingPoi(const CSegmentKey& segmentKey,
                                        Uint16 nPnts, 
                                        Float64 leftOverhang, 
                                        Float64 rightOverhang, 
                                        PoiAttributeType attrib, 
                                        PoiAttributeType supportAttribute,
                                        PoiAttributeType poiReference,
                                        pgsPoiMgr* pPoiMgr)
{
   Float64 segment_length = GetSegmentLength(segmentKey);
   Float64 span_length = segment_length - leftOverhang - rightOverhang;
   ATLASSERT(0.0 < span_length); // should be checked in input

   // add poi at support locations
   pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,leftOverhang,attrib | poiReference | supportAttribute) );
   pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,segment_length - rightOverhang,attrib | poiReference | supportAttribute) );

   // add poi at ends of girder
   pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,0,attrib) );
   pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,segment_length,attrib) );

   // nth point POI between overhang support points
   const Float64 toler = +1.0e-6;
   for ( Uint16 i = 0; i < nPnts+1; i++ )
   {
      Float64 dist = leftOverhang + span_length * ((Float64)i / (Float64)nPnts);

      PoiAttributeType attribute = attrib;

      // Add a special attribute flag if poi is at a tenth point
      Uint16 tenthPoint = 0;
      Float64 val = Float64(i)/Float64(nPnts)+toler;
      Float64 modv = fmod(val, 0.1);
      if (IsZero(modv,2*toler) || modv==0.1)
      {
         tenthPoint = Uint16(10.*Float64(i)/Float64(nPnts) + 1);
      }
      
      pgsPointOfInterest poi(segmentKey,dist,attribute);
      poi.MakeTenthPoint(poiReference,tenthPoint);
      pPoiMgr->AddPointOfInterest( poi );
   }

}

/////////////////////////////////////////////////////////////////////////
// IAgent
//
STDMETHODIMP CBridgeAgentImp::SetBroker(IBroker* pBroker)
{
   EAF_AGENT_SET_BROKER(pBroker);
   return S_OK;
}

STDMETHODIMP CBridgeAgentImp::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   pBrokerInit->RegInterface( IID_IRoadway,                       this );
   pBrokerInit->RegInterface( IID_IGeometry,                      this );
   pBrokerInit->RegInterface( IID_IBridge,                        this );
   pBrokerInit->RegInterface( IID_IMaterials,                     this );
   pBrokerInit->RegInterface( IID_IStrandGeometry,                this );
   pBrokerInit->RegInterface( IID_ILongRebarGeometry,             this );
   pBrokerInit->RegInterface( IID_IStirrupGeometry,               this );
   pBrokerInit->RegInterface( IID_IPointOfInterest,               this );
   pBrokerInit->RegInterface( IID_ISectionProperties,             this );
   pBrokerInit->RegInterface( IID_IShapes,                        this );
   pBrokerInit->RegInterface( IID_IBarriers,                      this );
   pBrokerInit->RegInterface( IID_ISegmentLiftingPointsOfInterest, this );
   pBrokerInit->RegInterface( IID_ISegmentHaulingPointsOfInterest, this );
   pBrokerInit->RegInterface( IID_IUserDefinedLoads,              this );
   pBrokerInit->RegInterface( IID_ITempSupport,                   this );
   pBrokerInit->RegInterface( IID_IGirder,                        this );
   pBrokerInit->RegInterface( IID_ITendonGeometry,                this );
   pBrokerInit->RegInterface( IID_IIntervals,                     this );

   return S_OK;
}

STDMETHODIMP CBridgeAgentImp::Init()
{
   CREATE_LOGFILE("BridgeAgent");
   EAF_AGENT_INIT; // this macro defines pStatusCenter
   m_LoadStatusGroupID = pStatusCenter->CreateStatusGroupID();

   // Register status callbacks that we want to use
   m_scidInformationalError       = pStatusCenter->RegisterCallback(new pgsInformationalStatusCallback(eafTypes::statusError)); 
   m_scidInformationalWarning     = pStatusCenter->RegisterCallback(new pgsInformationalStatusCallback(eafTypes::statusWarning)); 
   m_scidBridgeDescriptionError   = pStatusCenter->RegisterCallback(new pgsBridgeDescriptionStatusCallback(m_pBroker,eafTypes::statusError));
   m_scidBridgeDescriptionWarning = pStatusCenter->RegisterCallback(new pgsBridgeDescriptionStatusCallback(m_pBroker,eafTypes::statusWarning));
   m_scidAlignmentWarning         = pStatusCenter->RegisterCallback(new pgsAlignmentDescriptionStatusCallback(m_pBroker,eafTypes::statusWarning));
   m_scidAlignmentError           = pStatusCenter->RegisterCallback(new pgsAlignmentDescriptionStatusCallback(m_pBroker,eafTypes::statusError));
   m_scidGirderDescriptionWarning = pStatusCenter->RegisterCallback(new pgsGirderDescriptionStatusCallback(m_pBroker,eafTypes::statusWarning));
   m_scidPointLoadWarning         = pStatusCenter->RegisterCallback(new pgsPointLoadStatusCallback(m_pBroker,eafTypes::statusWarning));
   m_scidDistributedLoadWarning   = pStatusCenter->RegisterCallback(new pgsDistributedLoadStatusCallback(m_pBroker,eafTypes::statusWarning));
   m_scidMomentLoadWarning        = pStatusCenter->RegisterCallback(new pgsMomentLoadStatusCallback(m_pBroker,eafTypes::statusWarning));

   return AGENT_S_SECONDPASSINIT;
}

STDMETHODIMP CBridgeAgentImp::Init2()
{
   // Attach to connection points
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   CComPtr<IConnectionPoint> pCP;
   HRESULT hr = S_OK;

   // Connection point for the bridge description
   hr = pBrokerInit->FindConnectionPoint( IID_IBridgeDescriptionEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwBridgeDescCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   // Connection point for the specifications
   hr = pBrokerInit->FindConnectionPoint( IID_ISpecificationEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwSpecificationCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   // Connection point for the specifications
   hr = pBrokerInit->FindConnectionPoint( IID_ILossParametersEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwLossParametersCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   hr = m_SectCutTool.CoCreateInstance(CLSID_SectionCutTool);
   if ( FAILED(hr) || m_SectCutTool == NULL )
   {
      THROW_UNWIND(_T("GenericBridgeTools::SectionPropertyTool not created"),-1);
   }


   m_ConcreteManager.Init(m_pBroker,m_StatusGroupID);

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
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwBridgeDescCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_ISpecificationEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwSpecificationCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_ILossParametersEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwLossParametersCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   EAF_AGENT_CLEAR_INTERFACE_CACHE;
   CLOSE_LOGFILE;

   return S_OK;
}

/////////////////////////////////////////////////////////////////////////
// IRoadway
//
void CBridgeAgentImp::GetStartPoint(Float64 n,Float64* pStartStation,Float64* pStartElevation,Float64* pGrade,IPoint2d** ppPoint)
{
   Float64 pierStation = GetPierStation(0);
   Float64 spanLength = GetSpanLength(0);
   *pStartStation = pierStation - spanLength/n;

   CComPtr<IAlignment> alignment;
   GetAlignment(&alignment);

   IndexType nHCurves = GetCurveCount();
   if ( 0 < nHCurves )
   {
      CComPtr<IHorzCurve> hc;
      GetCurve(0,&hc);
      CComPtr<IPoint2d> pntTS;
      hc->get_TS(&pntTS);

      Float64 tsStation, offset;
      GetStationAndOffset(pntTS,&tsStation,&offset);
      *pStartStation = Min(*pStartStation,tsStation);
   }

   CComPtr<IProfile> profile;
   alignment->get_Profile(&profile);
   IndexType nElements;
   profile->get_Count(&nElements);
   if ( 0 < nElements )
   {
      CComPtr<IProfileElement> profileElement;
      profile->get_Item(0,&profileElement);
      ProfileElementType type;
      profileElement->get_Type(&type);
      CComPtr<IUnknown> punk;
      profileElement->get_Value(&punk);
      if ( type == pePoint )
      {
         CComQIPtr<IProfilePoint> pp(punk);
         CComPtr<IStation> objStation;
         pp->get_Station(&objStation);
         Float64 station;
         objStation->get_NormalizedValue(alignment,&station);
         *pStartStation = Min(*pStartStation,station);
      }
      else 
      {
         ATLASSERT( type == peVertCurve );
         CComQIPtr<IVertCurve> vc(punk);
         CComPtr<IProfilePoint> pp;
         vc->get_BVC(&pp);
         CComPtr<IStation> objStation;
         pp->get_Station(&objStation);
         Float64 station;
         objStation->get_NormalizedValue(alignment,&station);
         *pStartStation = Min(*pStartStation,station);
      }
   }

   CComPtr<ISurfaceCollection> surfaces;
   profile->get_Surfaces(&surfaces);
   IndexType nSurfaces;
   surfaces->get_Count(&nSurfaces);
   if ( 0 < nSurfaces )
   {
      CComPtr<ISurface> surface;
      surfaces->get_Item(0,&surface);

      CComPtr<IStation> objStartStation, objEndStation;
      surface->GetStationRange(&objStartStation,&objEndStation);
      Float64 station;
      objStartStation->get_NormalizedValue(alignment,&station);
      if ( nSurfaces == 1 && station < (pierStation - spanLength/n) && IsZero(station) )
      {
         // often, the roadway surface is defined by a single point at station 0+00
         // if there is only one surface defintion and it starts before the bridge
         // and it is at 0+00, skip it so we don't end up with a really long alignment
         // and a really short bridge... the graphics come out bad
      }
      else
      {
         *pStartStation = Min(*pStartStation,station);
      }
   }

   CComPtr<IStation> objRefStation;
   alignment->get_RefStation(&objRefStation);
   Float64 refStation;
   objRefStation->get_NormalizedValue(alignment,&refStation);
   CComPtr<IPoint2d> refPoint;
   alignment->LocatePoint(CComVariant(objRefStation),omtNormal,0.0,CComVariant(0),&refPoint);
   Float64 x,y;
   refPoint->Location(&x,&y);
   if ( !IsZero(refStation) || !IsZero(x) || !IsZero(y) )
   {
      // we'll ignore the ref point at 0+00 (N 0, E 0)
      // this is the default and it probably means the user didn't input the values
      *pStartStation = Min(*pStartStation,refStation);
   }

   GetPoint(*pStartStation,0,NULL,ppPoint);
   *pStartElevation = GetElevation(*pStartStation,0);
   *pGrade = GetProfileGrade(*pStartStation);
}

void CBridgeAgentImp::GetEndPoint(Float64 n,Float64* pEndStation,Float64* pEndElevation,Float64* pGrade,IPoint2d** ppPoint)
{
   PierIndexType nPiers = GetPierCount();
   SpanIndexType nSpans = GetSpanCount();
   Float64 pierStation = GetPierStation(nPiers-1);
   Float64 spanLength = GetSpanLength(nSpans-1);
   *pEndStation = pierStation + spanLength/n;

   CComPtr<IAlignment> alignment;
   GetAlignment(&alignment);

   IndexType nHCurves = GetCurveCount();
   if ( 0 < nHCurves )
   {
      CComPtr<IHorzCurve> hc;
      GetCurve(nHCurves-1,&hc);
      CComPtr<IPoint2d> pntST;
      hc->get_ST(&pntST);

      Float64 stStation, offset;
      GetStationAndOffset(pntST,&stStation,&offset);
      *pEndStation = Max(*pEndStation,stStation);
   }

   CComPtr<IProfile> profile;
   alignment->get_Profile(&profile);
   IndexType nElements;
   profile->get_Count(&nElements);
   if ( 0 < nElements )
   {
      CComPtr<IProfileElement> profileElement;
      profile->get_Item(nElements-1,&profileElement);
      ProfileElementType type;
      profileElement->get_Type(&type);
      CComPtr<IUnknown> punk;
      profileElement->get_Value(&punk);
      if ( type == pePoint )
      {
         CComQIPtr<IProfilePoint> pp(punk);
         CComPtr<IStation> objStation;
         pp->get_Station(&objStation);
         Float64 station;
         objStation->get_NormalizedValue(alignment,&station);
         *pEndStation = Max(*pEndStation,station);
      }
      else 
      {
         ATLASSERT( type == peVertCurve );
         CComQIPtr<IVertCurve> vc(punk);
         CComPtr<IProfilePoint> pp;
         vc->get_EVC(&pp);
         CComPtr<IStation> objStation;
         pp->get_Station(&objStation);
         Float64 station;
         objStation->get_NormalizedValue(alignment,&station);
         *pEndStation = Max(*pEndStation,station);
      }
   }

   CComPtr<ISurfaceCollection> surfaces;
   profile->get_Surfaces(&surfaces);
   IndexType nSurfaces;
   surfaces->get_Count(&nSurfaces);
   if ( 0 < nSurfaces )
   {
      CComPtr<ISurface> surface;
      surfaces->get_Item(nSurfaces-1,&surface);

      CComPtr<IStation> objStartStation, objEndStation;
      surface->GetStationRange(&objStartStation,&objEndStation);
      Float64 station;
      objEndStation->get_NormalizedValue(alignment,&station);
      *pEndStation = Max(*pEndStation,station);
   }

   GetPoint(*pEndStation,0,NULL,ppPoint);
   *pEndElevation = GetElevation(*pEndStation,0);
   *pGrade = GetProfileGrade(*pEndStation);
}

Float64 CBridgeAgentImp::GetSlope(Float64 station,Float64 offset)
{
   VALIDATE( COGO_MODEL );

   CComPtr<IProfile> profile;
   GetProfile(&profile);

   Float64 slope;
   profile->Slope(CComVariant(station),offset,&slope);

   return slope;
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

   CComPtr<IHorzCurveCollection> curves;
   m_CogoModel->get_HorzCurves(&curves);

   CollectionIndexType nCurves;
   curves->get_Count(&nCurves);
   return nCurves;
}

void CBridgeAgentImp::GetCurve(CollectionIndexType idx,IHorzCurve** ppCurve)
{
   VALIDATE( COGO_MODEL );

   CComPtr<IHorzCurveCollection> curves;
   m_CogoModel->get_HorzCurves(&curves);

   CogoObjectID key = m_HorzCurveKeys[idx];

   HRESULT hr = curves->get_Item(key,ppCurve);
   ATLASSERT(SUCCEEDED(hr));
}

CollectionIndexType CBridgeAgentImp::GetVertCurveCount()
{
   VALIDATE( COGO_MODEL );

   CComPtr<IVertCurveCollection> curves;
   m_CogoModel->get_VertCurves(&curves);

   CollectionIndexType nCurves;
   curves->get_Count(&nCurves);
   return nCurves;
}

void CBridgeAgentImp::GetVertCurve(CollectionIndexType idx,IVertCurve** ppCurve)
{
   VALIDATE( COGO_MODEL );

   CComPtr<IVertCurveCollection> curves;
   m_CogoModel->get_VertCurves(&curves);

   CogoObjectID key = m_VertCurveKeys[idx];

   HRESULT hr = curves->get_Item(key,ppCurve);
   ATLASSERT(SUCCEEDED(hr));
}

void CBridgeAgentImp::GetRoadwaySurface(Float64 station,IDirection* pDirection,IPoint2dCollection** ppPoints)
{
   CComPtr<IAlignment> alignment;
   GetAlignment(&alignment);

   CComPtr<IStation> objStation;
   objStation.CoCreateInstance(CLSID_Station);
   objStation->SetStation(INVALID_INDEX,station);
   CComVariant varStation(objStation);

   CComPtr<IProfile> profile;
   alignment->get_Profile(&profile);

   CComPtr<ISurface> surface;
   profile->GetSurface(COGO_FINISHED_SURFACE_ID,varStation,&surface);

   CComPtr<IPoint2dCollection> points;
   points.CoCreateInstance(CLSID_Point2dCollection);
   if ( pDirection == NULL )
   {
      // direction == NULL means the cut is normal to the alignment
      // using surface templates is faster but only works for normal cuts
      Float64 Xb = ConvertRouteToBridgeLineCoordinate(station);
      Float64 leftEdgeOffset  = GetLeftSlabEdgeOffset(Xb);
      Float64 rightEdgeOffset = GetRightSlabEdgeOffset(Xb);

      IndexType alignmentRidgePointIdx;
      surface->get_AlignmentPoint(&alignmentRidgePointIdx);

      CComPtr<ISurfaceTemplate> surfaceTemplate;
      surface->CreateSurfaceTemplate(varStation,VARIANT_TRUE,&surfaceTemplate);

      SegmentIndexType nSegments;
      surfaceTemplate->get_Count(&nSegments);
      IndexType nRidgePoints = nSegments+1;
      for ( IndexType ridgePointIdx = 0; ridgePointIdx < nRidgePoints; ridgePointIdx++ )
      {
         Float64 x,y;
         profile->RidgePointElevation(COGO_FINISHED_SURFACE_ID,varStation,ridgePointIdx,alignmentRidgePointIdx,&x,&y);

         if ( x < leftEdgeOffset )
         {
            x = leftEdgeOffset;
            profile->Elevation(varStation,x,&y);
         }
         else if ( rightEdgeOffset < x )
         {
            x = rightEdgeOffset;
            profile->Elevation(varStation,x,&y);
         }

         CComPtr<IPoint2d> point;
         point.CoCreateInstance(CLSID_Point2d);
         point->Move(x,y);
         points->Add(point);
      }
   }
   else
   {
      CComPtr<ISurfaceProfile> surfaceProfile;
      surface->CreateSurfaceProfile(varStation,CComVariant(pDirection),VARIANT_TRUE,&surfaceProfile);

      IndexType nRidgePoints;
      surfaceProfile->get_Count(&nRidgePoints);
      for ( IndexType ridgePointIdx = 0; ridgePointIdx < nRidgePoints; ridgePointIdx++ )
      {
         CComPtr<ISurfacePoint> surfacePoint;
         surfaceProfile->get_Item(ridgePointIdx,&surfacePoint);
         Float64 x,y;
         surfacePoint->get_CutLineOffset(&x);
         surfacePoint->get_Elevation(&y);

         CComPtr<IPoint2d> point;
         point.CoCreateInstance(CLSID_Point2d);
         point->Move(x,y);
         points->Add(point);
      }
   }

   points.CopyTo(ppPoints);
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

Float64 CBridgeAgentImp::GetCrownPointOffset(Float64 station)
{
   CComPtr<IAlignment> alignment;
   GetAlignment(&alignment);

   CComPtr<IStation> objStation;
   objStation.CoCreateInstance(CLSID_Station);
   objStation->SetStation(INVALID_INDEX,station);
   CComVariant varStation(objStation);

   CComPtr<IProfile> profile;
   alignment->get_Profile(&profile);

   CComPtr<ISurface> surface;
   profile->GetSurface(COGO_FINISHED_SURFACE_ID,varStation,&surface);

   IndexType alignmentRidgePointIdx;
   surface->get_AlignmentPoint(&alignmentRidgePointIdx);

   CComPtr<ISurfaceTemplate> surfaceTemplate;
   surface->CreateSurfaceTemplate(varStation,VARIANT_TRUE,&surfaceTemplate);

   Float64 offset;
   surfaceTemplate->GetRidgePointOffset(2,alignmentRidgePointIdx,&offset);

   return offset;
}

/////////////////////////////////////////////////////////
// IGeometry
HRESULT CBridgeAgentImp::Angle(IPoint2d* from,IPoint2d* vertex,IPoint2d* to,IAngle** angle)
{
   CComPtr<IMeasure2> measure;
   m_CogoEngine->get_Measure(&measure);
   return measure->Angle(from,vertex,to,angle);
}

HRESULT CBridgeAgentImp::Area(IPoint2dCollection* points,Float64* area)
{
   CComPtr<IMeasure2> measure;
   m_CogoEngine->get_Measure(&measure);
   return measure->Area(points,area);
}

HRESULT CBridgeAgentImp::Distance(IPoint2d* from,IPoint2d* to,Float64* dist)
{
   CComPtr<IMeasure2> measure;
   m_CogoEngine->get_Measure(&measure);
   return measure->Distance(from,to,dist);
}

HRESULT CBridgeAgentImp::Direction(IPoint2d* from,IPoint2d* to,IDirection** dir)
{
   CComPtr<IMeasure2> measure;
   m_CogoEngine->get_Measure(&measure);
   return measure->Direction(from,to,dir);
}

HRESULT CBridgeAgentImp::Inverse(IPoint2d* from,IPoint2d* to,Float64* dist,IDirection** dir)
{
   CComPtr<IMeasure2> measure;
   m_CogoEngine->get_Measure(&measure);
   return measure->Inverse(from,to,dist,dir);
}

HRESULT CBridgeAgentImp::ByDistAngle(IPoint2d* from,IPoint2d* to,Float64 dist,VARIANT varAngle,Float64 offset,IPoint2d** point)
{
   CComPtr<ILocate2> locate;
   m_CogoEngine->get_Locate(&locate);
   return locate->ByDistAngle(from,to,dist,varAngle,offset,point);
}

HRESULT CBridgeAgentImp::ByDistDefAngle(IPoint2d* from,IPoint2d* to,Float64 dist,VARIANT varDefAngle,Float64 offset,IPoint2d** point)
{
   CComPtr<ILocate2> locate;
   m_CogoEngine->get_Locate(&locate);
   return locate->ByDistDefAngle(from,to,dist,varDefAngle,offset,point);
}

HRESULT CBridgeAgentImp::ByDistDir(IPoint2d* from,Float64 dist,VARIANT varDir,Float64 offset,IPoint2d** point)
{
   CComPtr<ILocate2> locate;
   m_CogoEngine->get_Locate(&locate);
   return locate->ByDistDir(from,dist,varDir,offset,point);
}

HRESULT CBridgeAgentImp::PointOnLine(IPoint2d* from,IPoint2d* to,Float64 dist,Float64 offset,IPoint2d** point)
{
   CComPtr<ILocate2> locate;
   m_CogoEngine->get_Locate(&locate);
   return locate->PointOnLine(from,to,dist,offset,point);
}

HRESULT CBridgeAgentImp::ParallelLineByPoints(IPoint2d* from,IPoint2d* to,Float64 offset,IPoint2d** p1,IPoint2d** p2)
{
   CComPtr<ILocate2> locate;
   m_CogoEngine->get_Locate(&locate);
   return locate->ParallelLineByPoints(from,to,offset,p1,p2);
}

HRESULT CBridgeAgentImp::ParallelLineSegment(ILineSegment2d* ls,Float64 offset,ILineSegment2d** linesegment)
{
   CComPtr<ILocate2> locate;
   m_CogoEngine->get_Locate(&locate);
   return locate->ParallelLineSegment(ls,offset,linesegment);
}

HRESULT CBridgeAgentImp::Bearings(IPoint2d* p1,VARIANT varDir1,Float64 offset1,IPoint2d* p2,VARIANT varDir2,Float64 offset2,IPoint2d** point)
{
   CComPtr<IIntersect2> intersect;
   m_CogoEngine->get_Intersect(&intersect);
   return intersect->Bearings(p1,varDir1,offset1,p2,varDir2,offset2,point);
}

HRESULT CBridgeAgentImp::BearingCircle(IPoint2d* p1,VARIANT varDir,Float64 offset,IPoint2d* center,Float64 radius,IPoint2d* nearest,IPoint2d** point)
{
   CComPtr<IIntersect2> intersect;
   m_CogoEngine->get_Intersect(&intersect);
   return intersect->BearingCircle(p1,varDir,offset,center,radius,nearest,point);
}

HRESULT CBridgeAgentImp::Circles(IPoint2d* p1,Float64 r1,IPoint2d* p2,Float64 r2,IPoint2d* nearest,IPoint2d** point)
{
   CComPtr<IIntersect2> intersect;
   m_CogoEngine->get_Intersect(&intersect);
   return intersect->Circles(p1,r1,p2,r2,nearest,point);
}

HRESULT CBridgeAgentImp::LineByPointsCircle(IPoint2d* p1,IPoint2d* p2,Float64 offset,IPoint2d* center,Float64 radius,IPoint2d* nearest,IPoint2d** point)
{
   CComPtr<IIntersect2> intersect;
   m_CogoEngine->get_Intersect(&intersect);
   return intersect->LineByPointsCircle(p1,p2,offset,center,radius,nearest,point);
}

HRESULT CBridgeAgentImp::LinesByPoints(IPoint2d* p11,IPoint2d* p12,Float64 offset1,IPoint2d* p21,IPoint2d* p22,Float64 offset2,IPoint2d** point)
{
   CComPtr<IIntersect2> intersect;
   m_CogoEngine->get_Intersect(&intersect);
   return intersect->LinesByPoints(p11,p12,offset1,p21,p22,offset2,point);
}

HRESULT CBridgeAgentImp::Lines(ILineSegment2d* l1,Float64 offset1,ILineSegment2d* l2,Float64 offset2,IPoint2d** point)
{
   CComPtr<IIntersect2> intersect;
   m_CogoEngine->get_Intersect(&intersect);
   return intersect->Lines(l1,offset1,l2,offset2,point);
}

HRESULT CBridgeAgentImp::LineSegmentCircle(ILineSegment2d* pSeg,Float64 offset,IPoint2d* center,Float64 radius,IPoint2d* nearest, IPoint2d** point)
{
   CComPtr<IIntersect2> intersect;
   m_CogoEngine->get_Intersect(&intersect);
   return intersect->LineSegmentCircle(pSeg,offset,center,radius,nearest,point);
}

HRESULT CBridgeAgentImp::PointOnLineByPoints(IPoint2d* pnt,IPoint2d* start,IPoint2d* end,Float64 offset,IPoint2d** point)
{
   CComPtr<IProject2> project;
   m_CogoEngine->get_Project(&project);
   return project->PointOnLineByPoints(pnt,start,end,offset,point);
}

HRESULT CBridgeAgentImp::PointOnLineSegment(IPoint2d* from,ILineSegment2d* seg,Float64 offset,IPoint2d** point)
{
   CComPtr<IProject2> project;
   m_CogoEngine->get_Project(&project);
   return project->PointOnLineSegment(from,seg,offset,point);
}

HRESULT CBridgeAgentImp::PointOnCurve(IPoint2d* pnt,IHorzCurve* curve,IPoint2d** point)
{
   CComPtr<IProject2> project;
   m_CogoEngine->get_Project(&project);
   return project->PointOnCurve(pnt,curve,point);
}

HRESULT CBridgeAgentImp::Arc(IPoint2d* from, IPoint2d* vertex, IPoint2d* to,CollectionIndexType nParts,IPoint2dCollection** points)
{
   CComPtr<IDivide2> divide;
   m_CogoEngine->get_Divide(&divide);
   return divide->Arc(from,vertex,to,nParts,points);
}

HRESULT CBridgeAgentImp::BetweenPoints(IPoint2d* from, IPoint2d* to,CollectionIndexType nParts,IPoint2dCollection** points)
{
   CComPtr<IDivide2> divide;
   m_CogoEngine->get_Divide(&divide);
   return divide->BetweenPoints(from,to,nParts,points);
}

HRESULT CBridgeAgentImp::LineSegment(ILineSegment2d* seg,CollectionIndexType nParts,IPoint2dCollection** points)
{
   CComPtr<IDivide2> divide;
   m_CogoEngine->get_Divide(&divide);
   return divide->LineSegment(seg,nParts,points);
}

HRESULT CBridgeAgentImp::HorzCurve(IHorzCurve* curve, CollectionIndexType nParts, IPoint2dCollection** points)
{
   CComPtr<IDivide2> divide;
   m_CogoEngine->get_Divide(&divide);
   return divide->HorzCurve(curve,nParts,points);
}

HRESULT CBridgeAgentImp::Path(IPath* pPath,CollectionIndexType nParts,Float64 start,Float64 end,IPoint2dCollection** points)
{
   CComPtr<IDivide2> divide;
   m_CogoEngine->get_Divide(&divide);
   return divide->Path(pPath,nParts,start,end,points);
}

HRESULT CBridgeAgentImp::External(IPoint2d* center1, Float64 radius1,IPoint2d* center2,Float64 radius2,TangentSignType sign, IPoint2d** t1,IPoint2d** t2)
{
   CComPtr<ITangent2> tangent;
   m_CogoEngine->get_Tangent(&tangent);
   return tangent->External(center1,radius1,center2,radius2,sign,t1,t2);
}

HRESULT CBridgeAgentImp::Cross(IPoint2d* center1, Float64 radius1,IPoint2d* center2, Float64 radius2, TangentSignType sign, IPoint2d** t1,IPoint2d** t2)
{
   CComPtr<ITangent2> tangent;
   m_CogoEngine->get_Tangent(&tangent);
   return tangent->Cross(center1,radius1,center2,radius2,sign,t1,t2);
}

HRESULT CBridgeAgentImp::Point(IPoint2d* center, Float64 radius,IPoint2d* point, TangentSignType sign, IPoint2d** tangentPoint)
{
   CComPtr<ITangent2> tangent;
   m_CogoEngine->get_Tangent(&tangent);
   return tangent->Point(center,radius,point,sign,tangentPoint);
}

/////////////////////////////////////////////////////////////////////////
// IBridge
//
Float64 CBridgeAgentImp::GetLength()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   return pBridgeDesc->GetLength();
}

Float64 CBridgeAgentImp::GetSpanLength(SpanIndexType spanIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   PierIndexType startPierIdx = spanIdx;
   PierIndexType endPierIdx   = startPierIdx+1;

   const CPierData2* pStartPier = pBridgeDesc->GetPier(startPierIdx);
   const CPierData2* pEndPier   = pBridgeDesc->GetPier(endPierIdx);

   Float64 length = pEndPier->GetStation() - pStartPier->GetStation();

   return length;
}

Float64 CBridgeAgentImp::GetGirderLayoutLength(const CGirderKey& girderKey)
{
   ASSERT_GIRDER_KEY(girderKey);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);
   SegmentIndexType nSegments = pGirder->GetSegmentCount();

   Float64 L = 0;
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      L += GetSegmentLayoutLength(CSegmentKey(girderKey,segIdx));
   }
   return L;
}

Float64 CBridgeAgentImp::GetGirderSpanLength(const CGirderKey& girderKey)
{
   Float64 layout_length = GetGirderLayoutLength(girderKey);

   // layout length goes from between the back of pavement seats at the end piers
   // need to deduct for connection geometry at end piers

   Float64 startBrgOffset, endBrgOffset;
   CSegmentKey startSegmentKey(girderKey.groupIndex,girderKey.girderIndex,0);
   GetSegmentBearingOffset(startSegmentKey,&startBrgOffset,&endBrgOffset);
   layout_length -= startBrgOffset;

   SegmentIndexType nSegments = GetSegmentCount(girderKey);
   CSegmentKey endSegmentKey(girderKey.groupIndex,girderKey.girderIndex,nSegments-1);
   GetSegmentBearingOffset(endSegmentKey,&startBrgOffset,&endBrgOffset);
   layout_length -= endBrgOffset;

   return layout_length;
}

Float64 CBridgeAgentImp::GetGirderLength(const CGirderKey& girderKey)
{
   Float64 layout_length = GetGirderLayoutLength(girderKey);

   // layout length goes from between the back of pavement seats at the end piers
   // need to deduct for connection geometry at end piers

   CSegmentKey startSegmentKey(girderKey.groupIndex,girderKey.girderIndex,0);
   Float64 startBrgOffset, endBrgOffset;
   GetSegmentBearingOffset(startSegmentKey,&startBrgOffset,&endBrgOffset);
   layout_length -= startBrgOffset;

   Float64 startEndDist, endEndDist;
   GetSegmentEndDistance(startSegmentKey,&startEndDist,&endEndDist);
   layout_length += startEndDist;

   SegmentIndexType nSegments = GetSegmentCount(girderKey);
   CSegmentKey endSegmentKey(girderKey.groupIndex,girderKey.girderIndex,nSegments-1);

   GetSegmentBearingOffset(endSegmentKey,&startBrgOffset,&endBrgOffset);
   layout_length -= endBrgOffset;

   GetSegmentEndDistance(endSegmentKey,&startEndDist,&endEndDist);
   layout_length += endEndDist;

   return layout_length;
}

Float64 CBridgeAgentImp::GetAlignmentOffset()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   return pBridgeDesc->GetAlignmentOffset();
}

SpanIndexType CBridgeAgentImp::GetSpanCount()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   return pBridgeDesc->GetSpanCount();
}

PierIndexType CBridgeAgentImp::GetPierCount()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   return pBridgeDesc->GetPierCount();
}

SupportIndexType CBridgeAgentImp::GetTemporarySupportCount()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   return pBridgeDesc->GetTemporarySupportCount();
}

GroupIndexType CBridgeAgentImp::GetGirderGroupCount()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   return pBridgeDesc->GetGirderGroupCount();
}

GirderIndexType CBridgeAgentImp::GetGirderCount(GroupIndexType grpIdx)
{
   ATLASSERT( grpIdx != ALL_GROUPS );
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
   return pGroup->GetGirderCount();
}

GirderIndexType CBridgeAgentImp::GetGirderlineCount()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GirderIndexType nGirderLines = 0;
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      nGirderLines = Max(nGirderLines,pGroup->GetGirderCount());
   }
   return nGirderLines;
}

GirderIndexType CBridgeAgentImp::GetGirderCountBySpan(SpanIndexType spanIdx)
{
   ATLASSERT( spanIdx != ALL_SPANS );

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   return pGroup->GetGirderCount();
}

SegmentIndexType CBridgeAgentImp::GetSegmentCount(const CGirderKey& girderKey)
{
   ASSERT_GIRDER_KEY(girderKey);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);
   return pGirder->GetSegmentCount();
}

SegmentIndexType CBridgeAgentImp::GetSegmentCount(GroupIndexType grpIdx,GirderIndexType gdrIdx)
{
   return GetSegmentCount(CGirderKey(grpIdx,gdrIdx));
}

PierIndexType CBridgeAgentImp::GetGirderGroupStartPier(GroupIndexType grpIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx == ALL_GROUPS ? 0 : grpIdx);
   return pGroup->GetPierIndex(pgsTypes::metStart);
}

PierIndexType CBridgeAgentImp::GetGirderGroupEndPier(GroupIndexType grpIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx == ALL_GROUPS ? pBridgeDesc->GetGirderGroupCount()-1 : grpIdx);
   return pGroup->GetPierIndex(pgsTypes::metEnd);
}

void CBridgeAgentImp::GetGirderGroupPiers(GroupIndexType grpIdx,PierIndexType* pStartPierIdx,PierIndexType* pEndPierIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   if ( grpIdx == ALL_GROUPS )
   {
      *pStartPierIdx = 0;
      *pEndPierIdx = pBridgeDesc->GetPierCount()-1;
   }
   else
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      *pStartPierIdx = pGroup->GetPierIndex(pgsTypes::metStart);
      *pEndPierIdx = pGroup->GetPierIndex(pgsTypes::metEnd);
   }
}

SpanIndexType CBridgeAgentImp::GetGirderGroupStartSpan(GroupIndexType grpIdx)
{
   return (SpanIndexType)GetGirderGroupStartPier(grpIdx);
}

SpanIndexType CBridgeAgentImp::GetGirderGroupEndSpan(GroupIndexType grpIdx)
{
   return (SpanIndexType)(GetGirderGroupEndPier(grpIdx)-1);
}

void CBridgeAgentImp::GetGirderGroupSpans(GroupIndexType grpIdx,SpanIndexType* pStartSpanIdx,SpanIndexType* pEndSpanIdx)
{
   PierIndexType startPierIdx, endPierIdx;
   GetGirderGroupPiers(grpIdx,&startPierIdx,&endPierIdx);

   *pStartSpanIdx = (SpanIndexType)startPierIdx;
   *pEndSpanIdx   = (SpanIndexType)(endPierIdx-1);
}

GroupIndexType CBridgeAgentImp::GetGirderGroupIndex(SpanIndexType spanIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   return pGroup->GetIndex();
}

void CBridgeAgentImp::GetGirderGroupIndex(PierIndexType pierIdx,GroupIndexType* pBackGroupIdx,GroupIndexType* pAheadGroupIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);
   const CGirderGroupData* pPrevGroup = pPier->GetPrevGirderGroup();
   const CGirderGroupData* pNextGroup = pPier->GetNextGirderGroup();

   *pBackGroupIdx  = INVALID_INDEX;
   if ( pPrevGroup )
   {
      *pBackGroupIdx = pPrevGroup->GetIndex();
   }

   *pAheadGroupIdx = INVALID_INDEX;
   if ( pNextGroup )
   {
      *pAheadGroupIdx = pNextGroup->GetIndex();
   }
}

void CBridgeAgentImp::GetDistanceBetweenGirders(const pgsPointOfInterest& poi,Float64 *pLeft,Float64* pRight)
{
   VALIDATE( GIRDER );

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);

   const CGirderSpacing2* pStartSpacing;
   const CGirderSpacing2* pEndSpacing;
   pSegment->GetSpacing(&pStartSpacing,&pEndSpacing);

   // girder spacing is on the right hand side of the girder

   // Get spacing on the left side of the girder at start and end
   Float64 left_start = 0;
   Float64 left_end   = 0;

   if ( 0 != segmentKey.girderIndex )
   {
      SpacingIndexType spaceIdx = segmentKey.girderIndex-1;
      left_start = pStartSpacing->GetGirderSpacing(spaceIdx);
      left_end   = pEndSpacing->GetGirderSpacing(spaceIdx);
   }

   // Get spacing on the right side of the girder at start and end
   GirderIndexType nGirders = pGroup->GetGirderCount();
   Float64 right_start = 0;
   Float64 right_end   = 0;
   if ( segmentKey.girderIndex < nGirders-1 )
   {
      SpacingIndexType spaceIdx = segmentKey.girderIndex;
      right_start = pStartSpacing->GetGirderSpacing(spaceIdx);
      right_end   = pEndSpacing->GetGirderSpacing(spaceIdx);
   }

   Float64 gdrLength = GetSegmentLength(segmentKey);

   Float64 Xpoi = poi.GetDistFromStart();
   Float64 left  = ::LinInterp( Xpoi, left_start,  left_end,  gdrLength );
   Float64 right = ::LinInterp( Xpoi, right_start, right_end, gdrLength );

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
   if ( 0 != segmentKey.girderIndex )
   {
      pgsPointOfInterest leftPoi(poi);
      leftPoi.SetSegmentKey( CSegmentKey(segmentKey.groupIndex,segmentKey.girderIndex-1,segmentKey.segmentIndex) );
      left_width = Max(GetTopWidth(leftPoi),GetBottomWidth(leftPoi));
   }

   width = Max(GetTopWidth(poi),GetBottomWidth(poi));

   if ( segmentKey.girderIndex < nGirders-1 )
   {
      pgsPointOfInterest rightPoi(poi);
      rightPoi.SetSegmentKey( segmentKey );
      right_width = Max(GetTopWidth(rightPoi),GetBottomWidth(rightPoi));
   }

   // clear spacing is C-C spacing minus half the width of the adjacent girder minus the width of this girder
   *pLeft = left - left_width/2 - width/2;
   *pRight = right - right_width/2 - width/2;

   if ( *pLeft < 0 )
   {
      *pLeft = 0;
   }

   if ( *pRight < 0 )
   {
      *pRight = 0;
   }
}
void CBridgeAgentImp::GetBottomFlangeClearance(const pgsPointOfInterest& poi,Float64* pLeft,Float64* pRight)
{
   VALIDATE( BRIDGE );

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);

   const CGirderSpacing2* pStartSpacing;
   const CGirderSpacing2* pEndSpacing;
   pSegment->GetSpacing(&pStartSpacing,&pEndSpacing);

   // girder spacing is on the right hand side of the girder

   // Get spacing on the left side of the girder at start and end
   Float64 left_start = 0;
   Float64 left_end   = 0;

   if ( 0 != segmentKey.girderIndex )
   {
      SpacingIndexType spaceIdx = segmentKey.girderIndex-1;
      left_start = pStartSpacing->GetGirderSpacing(spaceIdx);
      left_end   = pEndSpacing->GetGirderSpacing(spaceIdx);
   }

   // Get spacing on the right side of the girder at start and end
   GirderIndexType nGirders = pGroup->GetGirderCount();
   Float64 right_start = 0;
   Float64 right_end   = 0;
   if ( segmentKey.girderIndex < nGirders-1 )
   {
      SpacingIndexType spaceIdx = segmentKey.girderIndex;
      right_start = pStartSpacing->GetGirderSpacing(spaceIdx);
      right_end   = pEndSpacing->GetGirderSpacing(spaceIdx);
   }

   Float64 gdrLength = GetSegmentLength(segmentKey);

   Float64 Xpoi = poi.GetDistFromStart();
   Float64 left  = ::LinInterp( Xpoi, left_start,  left_end,  gdrLength );
   Float64 right = ::LinInterp( Xpoi, right_start, right_end, gdrLength );

   // if the spacing is a joint spacing, we have what we are after
   if ( IsJointSpacing(pBridgeDesc->GetGirderSpacingType()) )
   {
      *pLeft  = left;
      *pRight = right;
      return;
   }

   // not a joint spacing so the spacing is between CL girders.... deduct the width
   // of the adjacent girders and this girder
   Float64 left_width = 0;
   Float64 width = 0;
   Float64 right_width = 0;
   if ( 0 != segmentKey.girderIndex )
   {
      pgsPointOfInterest leftPoi(poi);
      leftPoi.SetSegmentKey( CSegmentKey(segmentKey.groupIndex,segmentKey.girderIndex-1,segmentKey.segmentIndex) );
      left_width = GetBottomWidth(leftPoi);
   }

   width = GetBottomWidth(poi);

   if ( segmentKey.girderIndex < nGirders-1 )
   {
      pgsPointOfInterest rightPoi(poi);
      rightPoi.SetSegmentKey( segmentKey );
      right_width = GetBottomWidth(rightPoi);
   }

   // clear spacing is C-C spacing minus half the width of the adjacent girder minus the width of this girder
   *pLeft  = left  - left_width/2  - width/2;
   *pRight = right - right_width/2 - width/2;
}

std::vector<Float64> CBridgeAgentImp::GetGirderSpacing(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementLocation measureLocation,pgsTypes::MeasurementType measureType)
{
   VALIDATE( GIRDER );

   // Determine which girder group the face of pier belongs to
   GroupIndexType backGroupIdx, aheadGroupIdx;
   GetGirderGroupIndex(pierIdx,&backGroupIdx,&aheadGroupIdx);
   GroupIndexType grpIdx = (pierFace == pgsTypes::Back ? backGroupIdx : aheadGroupIdx);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(grpIdx);
   GirderIndexType nGirders = pGroup->GetGirderCount();

   // we are going to need the bridge geomtry model
   CComPtr<IBridgeGeometry> bridgeGeometry;
   m_Bridge->get_BridgeGeometry(&bridgeGeometry);

   CComPtr<IAlignment> alignment;
   m_Bridge->get_Alignment(&alignment);

   CComPtr<IPoint2d> pntLeft,pntAlignment,pntBridge,pntRight;
   GetPierPoints(pierIdx,&pntLeft,&pntAlignment,&pntBridge,&pntRight);

   CComPtr<ILine2d> measureLine; // only used if we are measuring normal to item (CLPier or CLBrg)
   if ( measureType == pgsTypes::NormalToItem )
   {
      // need the line that we are measuring along... then we will intersect the segment lines
      // with this line to compute the spacing
      if ( measureLocation == pgsTypes::AtPierLine )
      {
         // measuring along a line that is normal to the CL Pier line
         Float64 station = pIBridgeDesc->GetPier(pierIdx)->GetStation();
         CComPtr<IDirection> normal;
         alignment->Normal(CComVariant(station),&normal);
         Float64 dir;
         normal->get_Value(&dir);

         measureLine.CoCreateInstance(CLSID_Line2d);
         CComPtr<IVector2d> v;
         CComPtr<IPoint2d> pnt;
         measureLine->GetExplicit(&pnt,&v);

         v->put_Direction(dir);
         measureLine->SetExplicit(pntAlignment,v);
      }
      else
      {
         // measuring along a line that is normal the CL Bearing line
         // NOTE: in the general case, there can be multiple CL Bearing lines.
         // to make things managable, we'll create a CL Bearing line by connecting the CL Bearing point
         // for the first and last girder in the span
         CSegmentKey leftSegmentKey = GetSegmentAtPier(pierIdx,CGirderKey(grpIdx,0));
         CSegmentKey rightSegmentKey = GetSegmentAtPier(pierIdx,CGirderKey(grpIdx,nGirders-1));

         CComPtr<IPoint2d> pntPier1,pntEnd1,pntBrg1,pntBrg2,pntEnd2,pntPier2;
         GetSegmentEndPoints(leftSegmentKey,&pntPier1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntPier2);
         CComPtr<IPoint2d> p1 = (pierFace == pgsTypes::Ahead ? pntBrg1 : pntBrg2);

         pntPier1.Release();
         pntEnd1.Release();
         pntBrg1.Release();
         pntBrg2.Release();
         pntEnd2.Release();
         pntPier2.Release();
         GetSegmentEndPoints(rightSegmentKey,&pntPier1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntPier2);
         CComPtr<IPoint2d> p2 = (pierFace == pgsTypes::Ahead ? pntBrg1 : pntBrg2);

         CComPtr<ILine2d> clBrgLine;
         clBrgLine.CoCreateInstance(CLSID_Line2d);
         clBrgLine->ThroughPoints(p1,p2);

         CComPtr<IPoint2d> pntBrg;
         alignment->Intersect(clBrgLine,pntAlignment,&pntBrg);

         CComPtr<IStation> station;
         Float64 offset;
         alignment->Offset(pntBrg,&station,&offset);

         CComPtr<IDirection> normal;
         alignment->Normal(CComVariant(station),&normal);
         Float64 dir;
         normal->get_Value(&dir);

         measureLine.CoCreateInstance(CLSID_Line2d);
         CComPtr<IPoint2d> p;
         CComPtr<IVector2d> v;
         measureLine->GetExplicit(&p,&v);

         v->put_Direction(dir);
         measureLine->SetExplicit(pntBrg,v);
      }
   }

   // loop over all girders and compute the spacing between the girders at the CL Pier
   std::vector<Float64> spaces;
   for ( GirderIndexType gdrIdx = 1; gdrIdx < nGirders; gdrIdx++ ) // start at gdrIdx 1 (spacing between gdrIdx 0 and 1)
   {
      // girder keys for girders on left and right side of the space
      CGirderKey leftGirderKey(grpIdx,gdrIdx-1);
      CGirderKey rightGirderKey(grpIdx,gdrIdx);

      // segments that touch the pier
      CSegmentKey leftGdrSegmentKey(GetSegmentAtPier(pierIdx,leftGirderKey));
      CSegmentKey rightGdrSegmentKey(GetSegmentAtPier(pierIdx,rightGirderKey));

      // girder line IDs for the segments
      GirderIDType leftGdrID  = ::GetGirderSegmentLineID(leftGdrSegmentKey);
      GirderIDType rightGdrID = ::GetGirderSegmentLineID(rightGdrSegmentKey);

      // get the girder lines so they can be intersected with the pier line
      CComPtr<IGirderLine> leftGirderLine, rightGirderLine;
      bridgeGeometry->FindGirderLine(leftGdrID,&leftGirderLine);
      bridgeGeometry->FindGirderLine(rightGdrID,&rightGirderLine);

      CComPtr<IPoint2d> pntLeft, pntRight; // point on left and right girder
      // spacing is distance between the points

      Float64 distance = -9999;
      if ( measureType == pgsTypes::NormalToItem )
      {
         // intersect measure line with girder paths

         // get the girder paths
         CComPtr<IPath> leftPath, rightPath;
         leftGirderLine->get_Path(&leftPath);
         rightGirderLine->get_Path(&rightPath);

         // intersect each path with the normal line
         leftPath->Intersect(measureLine,pntAlignment,&pntLeft);
         rightPath->Intersect(measureLine,pntAlignment,&pntRight);
      }
      else
      {
         ATLASSERT(measureType == pgsTypes::AlongItem);
         // geometry points are already computed... just get them

         switch ( measureLocation )
         {
         case pgsTypes::AtPierLine:
            leftGirderLine->get_PierPoint(pierFace == pgsTypes::Back ? etEnd : etStart,&pntLeft);
            rightGirderLine->get_PierPoint(pierFace == pgsTypes::Back ? etEnd : etStart,&pntRight);
            break;

         case pgsTypes::AtCenterlineBearing:
            leftGirderLine->get_BearingPoint(pierFace == pgsTypes::Back ? etEnd : etStart,&pntLeft);
            rightGirderLine->get_BearingPoint(pierFace == pgsTypes::Back ? etEnd : etStart,&pntRight);
            break;

         default:
            ATLASSERT(false);
         }
      }

      pntLeft->DistanceEx(pntRight,&distance); // distance along line

      ATLASSERT( 0 < distance );
      spaces.push_back(distance);
   }

   return spaces;
}

Float64 CBridgeAgentImp::GetGirderOffset(GirderIndexType gdrIdx,PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::OffsetMeasurementType offsetMeasureDatum)
{
   VALIDATE(GIRDER);
   CComPtr<IPoint2d> pntLeft, pntAlignment, pntBridge, pntRight;
   GetPierPoints(pierIdx,&pntLeft,&pntAlignment,&pntBridge,&pntRight);

   GroupIndexType grpIdx = GetGirderGroupAtPier(pierIdx,pierFace);
   CSegmentKey segmentKey(GetSegmentAtPier(pierIdx,CGirderKey(grpIdx,gdrIdx)));

   CComPtr<IPoint2d> pntSegment;
   bool bResult = GetSegmentPierIntersection(segmentKey,pierIdx,&pntSegment);
   ATLASSERT(bResult == true);

   // Create a coordinate system with origin at pntAlignment/pntBridge and the x-axis
   // pointing toward the right side of the alignment and in the direction of the CL pier
   // Transform pntSegment into this coordinate system. If the X value of the point in 
   // the new coordinate system is the offset from the offset datum

   CComPtr<IDirection> dirPier;
   GetPierDirection(pierIdx,&dirPier);
   dirPier->IncrementBy(CComVariant(M_PI)); // rotation 180 so X is to the right of the alignment
   Float64 dir;
   dirPier->get_Value(&dir);

   CComPtr<ICoordinateXform2d> xform;
   xform.CoCreateInstance(CLSID_CoordinateXform2d);
   if (offsetMeasureDatum == pgsTypes::omtAlignment )
   {
      xform->putref_NewOrigin(pntAlignment);
   }
   else
   {
      xform->putref_NewOrigin(pntBridge);
   }

   xform->put_RotationAngle(dir);

   CComPtr<IPoint2d> pnt;
   xform->XformEx(pntSegment,xfrmOldToNew,&pnt);

   Float64 offset;
   pnt->get_X(&offset);

#if defined _DEBUG
   Float64 Y;
   pnt->get_Y(&Y);
   ATLASSERT(IsZero(Y));
#endif

   return offset;
}

std::vector<SpaceBetweenGirder> CBridgeAgentImp::GetGirderSpacing(Float64 station)
{
   // Get the spacing between girders at the specified location. Spacing is measured normal to the alignment.
   // Identical adjacent spaces are grouped together

   // NOTE: this should be delegated to Generic Bridge Model or Bridge Geometry Model

   VALIDATE( GIRDER );
   std::vector<SpaceBetweenGirder> vSpacing;

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   const CGirderGroupData* pGroup = NULL;
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pThisGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      Float64 startStation = pThisGroup->GetPier(pgsTypes::metStart)->GetStation();
      Float64 endStation   = pThisGroup->GetPier(pgsTypes::metEnd)->GetStation();
      if ( ::InRange(startStation,station,endStation) )
      {
         pGroup = pThisGroup;
         break;
      }
   }

   if ( pGroup == NULL )
   {
      return vSpacing;
   }

   GroupIndexType grpIdx = pGroup->GetIndex();
   GirderIndexType nGirders = pGroup->GetGirderCount();

   if ( nGirders <= 1 )
   {
      return vSpacing;
   }

   CComPtr<IAlignment> alignment;
   GetAlignment(&alignment);

   CComPtr<IDirection> normal;
   alignment->Normal(CComVariant(station),&normal);
   Float64 dirNormal;
   normal->get_Value(&dirNormal);

   CComPtr<IPoint2d> pntAlignment;
   alignment->LocatePoint(CComVariant(station),omtAlongDirection,0.00,CComVariant(normal),&pntAlignment);

   CComPtr<IVector2d> v;
   v.CoCreateInstance(CLSID_Vector2d);
   v->put_Direction(dirNormal);

   CComPtr<ILine2d> line;
   line.CoCreateInstance(CLSID_Line2d);
   line->SetExplicit(pntAlignment,v);

   SpaceBetweenGirder gdrSpacing;
   gdrSpacing.firstGdrIdx = 0;
   gdrSpacing.lastGdrIdx  = 0;
   gdrSpacing.spacing     = -1;

   CComPtr<IPoint2d> pntIntersection1;
   SegmentIndexType segIdx = GetSegmentIndex(CGirderKey(grpIdx,0),line,&pntIntersection1);
   if ( segIdx == INVALID_INDEX )
   {
      return std::vector<SpaceBetweenGirder>();
   }

   for ( GirderIndexType gdrIdx = 1; gdrIdx < nGirders; gdrIdx++ )
   {
      CComPtr<IPoint2d> pntIntersection2;
      segIdx = GetSegmentIndex(CGirderKey(grpIdx,gdrIdx),line,&pntIntersection2);
      if ( segIdx == INVALID_INDEX )
      {
         return std::vector<SpaceBetweenGirder>();
      }

      Float64 space;
      pntIntersection1->DistanceEx(pntIntersection2,&space);
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

      pntIntersection1 = pntIntersection2;
   }

   return vSpacing;
}

std::vector<Float64> CBridgeAgentImp::GetGirderSpacing(SpanIndexType spanIdx,Float64 Xspan)
{
   // Get the spacing between girders at the specified location. Spacing is measured normal to the alignment.
   // Identical adjacent spaces are grouped together

   // NOTE: this should be delegated to Generic Bridge Model or Bridge Geometry Model

   VALIDATE( GIRDER );
   std::vector<Float64> vSpacing;

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);

   GroupIndexType grpIdx = pGroup->GetIndex();
   GirderIndexType nGirders = pGroup->GetGirderCount();

   if ( nGirders <= 1 )
   {
      return vSpacing;
   }

   const CPierData2* pPier = pSpan->GetPier(pgsTypes::metStart);
   Float64 station = pPier->GetStation();
   station += Xspan;

   CComPtr<IAlignment> alignment;
   GetAlignment(&alignment);

   CComPtr<IDirection> normal;
   alignment->Normal(CComVariant(station),&normal);
   Float64 dirNormal;
   normal->get_Value(&dirNormal);

   CComPtr<IPoint2d> pntAlignment;
   alignment->LocatePoint(CComVariant(station),omtAlongDirection,0.00,CComVariant(normal),&pntAlignment);

   CComPtr<IVector2d> v;
   v.CoCreateInstance(CLSID_Vector2d);
   v->put_Direction(dirNormal);

   CComPtr<ILine2d> line;
   line.CoCreateInstance(CLSID_Line2d);
   line->SetExplicit(pntAlignment,v);

   CComPtr<IPoint2d> pntIntersection1;
   SegmentIndexType segIdx = GetSegmentIndex(CGirderKey(grpIdx,0),line,&pntIntersection1);
   if ( segIdx == INVALID_INDEX )
   {
      return std::vector<Float64>();
   }

   for ( GirderIndexType gdrIdx = 1; gdrIdx < nGirders; gdrIdx++ )
   {
      CComPtr<IPoint2d> pntIntersection2;
      segIdx = GetSegmentIndex(CGirderKey(grpIdx,gdrIdx),line,&pntIntersection2);
      if ( segIdx == INVALID_INDEX )
      {
         return std::vector<Float64>();
      }

      Float64 space;
      pntIntersection1->DistanceEx(pntIntersection2,&space);

      vSpacing.push_back(space);

      pntIntersection1 = pntIntersection2;
   }

   return vSpacing;
}

void CBridgeAgentImp::GetSpacingAlongGirder(const CGirderKey& girderKey,Float64 Xg,Float64* leftSpacing,Float64* rightSpacing)
{
   pgsPointOfInterest poi = ConvertGirderCoordinateToPoi(girderKey,Xg);
   GetSpacingAlongGirder(poi,leftSpacing,rightSpacing);
}

void CBridgeAgentImp::GetSpacingAlongGirder(const pgsPointOfInterest& poi,Float64* leftSpacing,Float64* rightSpacing)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   Float64 Xpoi = poi.GetDistFromStart();

   GirderIDType leftGdrID, gdrID, rightGdrID;
   ::GetAdjacentSuperstructureMemberIDs(segmentKey,&leftGdrID,&gdrID,&rightGdrID);

   m_BridgeGeometryTool->GirderSpacingBySSMbr(m_Bridge,gdrID,Xpoi,leftGdrID, leftSpacing);
   m_BridgeGeometryTool->GirderSpacingBySSMbr(m_Bridge,gdrID,Xpoi,rightGdrID,rightSpacing);
}

std::vector<std::pair<SegmentIndexType,Float64>> CBridgeAgentImp::GetSegmentLengths(const CSpanKey& spanKey)
{
   // NOTE: this method looks like it is something that should be in WBFLBridgeGeometry
   ASSERT_SPAN_KEY(spanKey);

   std::vector<std::pair<SegmentIndexType,Float64>> seg_lengths;

   // Returns the structural span length for (spanIdx,gdrIdx) measured along the centerline of the girder
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();

   // Get the group this span belongs to
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup( pBridgeDesc->GetSpan(spanKey.spanIndex) );
   GroupIndexType grpIdx = pGroup->GetIndex();
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();

   // Get the girder in this group
   const CSplicedGirderData* pGirder = pGroup->GetGirder(spanKey.girderIndex);

   Float64 span_length = 0;

   // Determine which segments are part of this span
   SegmentIndexType nSegments = pGirder->GetSegmentCount();
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
      CSegmentKey segmentKey( pSegment->GetSegmentKey() );
      
      SpanIndexType startSpanIdx = pSegment->GetSpanIndex(pgsTypes::metStart);
      SpanIndexType endSpanIdx   = pSegment->GetSpanIndex(pgsTypes::metEnd);

      if ( startSpanIdx <= spanKey.spanIndex && spanKey.spanIndex <= endSpanIdx )
      {
         // this segment starts, ends, or is completely in the span

         Float64 distance = -99999;
         if ( startSpanIdx < spanKey.spanIndex && spanKey.spanIndex == endSpanIdx )
         {
            // segment starts in a previous span but ends in this span

            // Only count the length of segment in this span
            // Intersect the girder line for this segment with the
            // pier line at the start of this span
            // We want the distance from the intersection point to the end of the segment
            //
            //             |<----Segment---->|
            //    |        |       |         |       |
            //    |--------.-----------------.-------|
            //    |   spanIdx-1    |   spanIdx       |
            //                     |
            //                     |<------->|
            //                     |  Length of segment in this span
            //                     |
            //                     +- CL Pier

            CComPtr<ISuperstructureMemberSegment> segment;
            GetSegment(segmentKey,&segment);
            CComPtr<IGirderLine> girderLine;
            segment->get_GirderLine(&girderLine);
            CComPtr<IPath> path;
            girderLine->get_Path(&path);

            PierIndexType pierIdx = (PierIndexType)spanKey.spanIndex; // index of the pier at the start of the span
            CComPtr<IPierLine> pierLine;
            GetPierLine(pierIdx,&pierLine);
            CComPtr<ILine2d> clPier;
            pierLine->get_Centerline(&clPier);

            CComPtr<IPoint2d> pntNearest;
            pierLine->get_AlignmentPoint(&pntNearest);

            CComPtr<IPoint2d> pntIntersect;
            path->Intersect(clPier,pntNearest,&pntIntersect);

            CComPtr<IPoint2d> pntSegmentEnd;
            girderLine->get_PierPoint(etEnd,&pntSegmentEnd);

            pntSegmentEnd->DistanceEx(pntIntersect,&distance); // distance from CL Pier to CL Pier/TS at right end of segment

            if ( segIdx == nSegments-1 && grpIdx == nGroups-1 && spanKey.spanIndex == nSpans-1 )
            {
               // last segment in the last group, adjust distance so that it is measured from CL-Bearing at right end of segment
               Float64 brg_offset;
               girderLine->get_BearingOffset(etEnd,&brg_offset);
               distance -= brg_offset;
            }
         }
         else if ( startSpanIdx == spanKey.spanIndex && spanKey.spanIndex < endSpanIdx )
         {
            // Segment starts in ths span but ends in a later span
            //
            // Only count the length of segment in this span
            // Intersect the girder line for this segment with the
            // pier line at the end of this span
            // We want the distance from the start of the segment to the intersection point
            //
            //             |<----Segment---->|
            //    |        |                 |       |
            //    |--------.-----------------.-------|
            //    |  spanIdx       |   spanIdx+1     |
            //                     |
            //             |<----->|
            //                 Length of segment in this span
            //                     |
            //                     +-- CL Pier

            CComPtr<ISuperstructureMemberSegment> segment;
            GetSegment(segmentKey,&segment);
            CComPtr<IGirderLine> girderLine;
            segment->get_GirderLine(&girderLine);
            CComPtr<IPath> path;
            girderLine->get_Path(&path);

            PierIndexType pierIdx = (PierIndexType)(spanKey.spanIndex+1);
            CComPtr<IPierLine> pierLine;
            GetPierLine(pierIdx,&pierLine);
            CComPtr<ILine2d> clPier;
            pierLine->get_Centerline(&clPier);

            CComPtr<IPoint2d> pntNearest;
            pierLine->get_AlignmentPoint(&pntNearest);

            CComPtr<IPoint2d> pntIntersect;
            path->Intersect(clPier,pntNearest,&pntIntersect);

            CComPtr<IPoint2d> pntSegmentStart;
            girderLine->get_PierPoint(etStart,&pntSegmentStart);

            pntSegmentStart->DistanceEx(pntIntersect,&distance); // distance from the CL Pier/TS on the left end of the segment to the CL Pier

            if ( segIdx == 0 && grpIdx == 0 && spanKey.spanIndex == 0 )
            {
               // first segment in the last group, adjust distance so that it is measured from CL-Bearing at left end of segment
               Float64 brg_offset;
               girderLine->get_BearingOffset(etStart,&brg_offset);
               distance -= brg_offset;
            }
         }
         else if ( startSpanIdx < spanKey.spanIndex && spanKey.spanIndex < endSpanIdx )
         {
            // Segments starts in a previous span and end in a later span

            // Intersect the girder line for this segment with the pier
            // lines at the start and end of the span
            //
            //             |<----------Segment--------------->|
            //    |        |       |                 |        |   |
            //    |--------.----------------------------------.---|
            //    |                |   spanIdx       |            |
            //    |                |                 |            |
            //                     |<--------------->|
            //                     |     Length of segment in this span
            //                     |                 |
            //                     |                 +--- CL Pier
            //                     +--- CL Pier

            CComPtr<ISuperstructureMemberSegment> segment;
            GetSegment(segmentKey,&segment);
            CComPtr<IGirderLine> girderLine;
            segment->get_GirderLine(&girderLine);
            CComPtr<IPath> path;
            girderLine->get_Path(&path);

            CComPtr<IPierLine> startPierLine, endPierLine;
            GetPierLine(spanKey.spanIndex,&startPierLine);
            GetPierLine(spanKey.spanIndex+1,&endPierLine);

            CComPtr<ILine2d> clStartPier, clEndPier;
            startPierLine->get_Centerline(&clStartPier);
            endPierLine->get_Centerline(&clEndPier);

            CComPtr<IPoint2d> pntNearest;
            startPierLine->get_AlignmentPoint(&pntNearest);

            CComPtr<IPoint2d> pntStartIntersect;
            path->Intersect(clStartPier,pntNearest,&pntStartIntersect);

            pntNearest.Release();
            endPierLine->get_AlignmentPoint(&pntNearest);

            CComPtr<IPoint2d> pntEndIntersect;
            path->Intersect(clEndPier,pntNearest,&pntEndIntersect);

            pntStartIntersect->DistanceEx(pntEndIntersect,&distance);
         }
         else
         {
            // segment is completely within the span
            CComPtr<IGirderLine> girderLine;
            GetGirderLine(segmentKey,&girderLine);
            girderLine->get_LayoutLength(&distance); // use the CL-Pier to CL-Pier length to account for closure joints

            if ( segIdx == 0 && grpIdx == 0 && spanKey.spanIndex == 0 )
            {
               // first segment in the first group, measure from CL-Bearing
               Float64 brg_offset;
               girderLine->get_BearingOffset(etStart,&brg_offset);
               distance -= brg_offset;
            }
            
            if ( segIdx == nSegments-1 && grpIdx == nGroups-1 && spanKey.spanIndex == nSpans-1 )
            {
               // last segment in the last group, measure from CL-Bearing
               Float64 brg_offset;
               girderLine->get_BearingOffset(etEnd,&brg_offset);
               distance -= brg_offset;
            }
         }

         ATLASSERT(0 < distance);

         seg_lengths.push_back(std::make_pair(segIdx,distance));
      } // next segment

      // Once the segment starts after this span, break out of the loop
      if ( spanKey.spanIndex < startSpanIdx )
      {
         break;
      }

   } // next span

   return seg_lengths;
}

Float64 CBridgeAgentImp::GetSpanLength(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   return GetSpanLength(CSpanKey(spanIdx,gdrIdx));
}

Float64 CBridgeAgentImp::GetSpanLength(const CSpanKey& spanKey)
{
   ASSERT_SPAN_KEY(spanKey);

   PierIndexType startPierIdx = (PierIndexType)spanKey.spanIndex;
   PierIndexType endPierIdx = startPierIdx + 1;
   GroupIndexType grpIdx = GetGirderGroupAtPier(startPierIdx,pgsTypes::Ahead);
   GroupIndexType nGroups = GetGirderGroupCount();
   SegmentIndexType nSegments = GetSegmentCount(grpIdx,spanKey.girderIndex);

   Float64 span_length = 0;
   std::vector<std::pair<SegmentIndexType,Float64>> seg_lengths(GetSegmentLengths(spanKey));
   std::vector<std::pair<SegmentIndexType,Float64>>::iterator iter(seg_lengths.begin());
   std::vector<std::pair<SegmentIndexType,Float64>>::iterator iterEnd(seg_lengths.end());
   for ( ; iter != iterEnd; iter++ )
   {
      span_length += (*iter).second;

      // At boundary piers, but not the first or last pier in the bridge, the segment 
      // length returned by GetSegmentLengths go to the centerline of the pier. If
      // there is a simple span boundary condition between the groups the span
      // length we want for loading is the span length to the CL Bearing, not CL Pier.
      // Adjust the segment length with the bearing offset.
      // This adjustment as already been made for the first segment in the first group
      // and the last segment in the last group

      if ( (*iter).first == 0 && grpIdx != 0 )
      {
         // this is the first segment in a group (but not the first group)
         bool bContinuousBack, bContinuousAhead;
         IsContinuousAtPier(startPierIdx,&bContinuousBack,&bContinuousAhead);
         bool bIntegralBack, bIntegralAhead;
         IsIntegralAtPier(startPierIdx,&bIntegralBack,&bIntegralAhead);
         if ( !(bContinuousAhead || bIntegralAhead) )
         {
            CSegmentKey segmentKey(grpIdx,spanKey.girderIndex,(*iter).first);
            Float64 brgOffset = GetSegmentStartBearingOffset(segmentKey);
            span_length -= brgOffset;
         }
      }

      if ( (*iter).first == nSegments-1 && grpIdx != nGroups-1 )
      {
         // this is the last segment in a group (but not the last group)
         bool bContinuousBack, bContinuousAhead;
         IsContinuousAtPier(endPierIdx,&bContinuousBack,&bContinuousAhead);
         bool bIntegralBack, bIntegralAhead;
         IsIntegralAtPier(endPierIdx,&bIntegralBack,&bIntegralAhead);
         if ( !(bContinuousBack || bIntegralBack) )
         {
            CSegmentKey segmentKey(grpIdx,spanKey.girderIndex,(*iter).first);
            Float64 brgOffset = GetSegmentEndBearingOffset(segmentKey);
            span_length -= brgOffset;
         }
      }
   }

   return span_length;
}

Float64 CBridgeAgentImp::GetFullSpanLength(const CSpanKey& spanKey)
{
   Float64 span_length = 0;
   std::vector<std::pair<SegmentIndexType,Float64>> seg_lengths(GetSegmentLengths(spanKey));
   std::vector<std::pair<SegmentIndexType,Float64>>::iterator iter(seg_lengths.begin());
   std::vector<std::pair<SegmentIndexType,Float64>>::iterator iterEnd(seg_lengths.end());
   for ( ; iter != iterEnd; iter++ )
   {
      span_length += (*iter).second;
   }
   return span_length;
}

Float64 CBridgeAgentImp::GetGirderlineLength(GirderIndexType gdrLineIdx)
{
   Float64 Lgl = 0;
   GroupIndexType nGroups = GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType nGirders = GetGirderCount(grpIdx);
      CGirderKey girderKey(grpIdx,Min(gdrLineIdx,nGirders-1));
      Float64 Lg = GetGirderLength(girderKey);
      Lgl += Lg;
   }
   return Lgl;
}

Float64 CBridgeAgentImp::GetCantileverLength(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::MemberEndType endType)
{
   ATLASSERT(spanIdx != ALL_SPANS && gdrIdx != ALL_GIRDERS);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   PierIndexType pierIdx = (PierIndexType)spanIdx;
   if ( endType == pgsTypes::metEnd )
   {
      pierIdx++;
   }

   const CPierData2* pPier = pIBridgeDesc->GetPier(pierIdx);
   if ( !pPier->HasCantilever() )
   {
      return 0;
   }

   // Since cantilevers can only happen on the first or last spans, we can easily infer the segment key
   CSegmentKey segmentKey(0,gdrIdx,0);
   if ( 0 < spanIdx ) 
   {
      GroupIndexType nGroups = pIBridgeDesc->GetGirderGroupCount();
      const CSplicedGirderData* pGirder = pIBridgeDesc->GetGirder(CGirderKey(nGroups-1,gdrIdx));
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      segmentKey.groupIndex = nGroups-1;
      segmentKey.segmentIndex = nSegments-1;
   }

   Float64 Lc = 0;
   if ( endType == pgsTypes::metStart )
   {
      Lc = GetSegmentStartEndDistance(segmentKey);
   }
   else
   {
      Lc = GetSegmentEndEndDistance(segmentKey);
   }

#if defined _DEBUG
   Float64 LcTest;
   if ( endType == pgsTypes::metStart )
   {
      std::vector<pgsPointOfInterest> vPoi = GetPointsOfInterest(segmentKey,POI_RELEASED_SEGMENT | POI_0L,POIFIND_AND);
      ATLASSERT(vPoi.size() == 1);
      pgsPointOfInterest poiStart = vPoi.front();

      vPoi = GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_0L,POIFIND_AND);
      ATLASSERT(vPoi.size() == 1);
      pgsPointOfInterest poiEnd = vPoi.front();

      LcTest = poiEnd.GetDistFromStart() - poiStart.GetDistFromStart();
   }
   else
   {
      std::vector<pgsPointOfInterest> vPoi = GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_10L,POIFIND_AND);
      ATLASSERT(vPoi.size() == 1);
      pgsPointOfInterest poiStart = vPoi.front();

      vPoi = GetPointsOfInterest(segmentKey,POI_RELEASED_SEGMENT | POI_10L,POIFIND_AND);
      ATLASSERT(vPoi.size() == 1);
      pgsPointOfInterest poiEnd = vPoi.front();

      LcTest = poiEnd.GetDistFromStart() - poiStart.GetDistFromStart();
   }
   ATLASSERT(IsEqual(Lc,LcTest));
#endif // _DEBUG

   return Lc;
}

Float64 CBridgeAgentImp::GetCantileverLength(const CSpanKey& spanKey,pgsTypes::MemberEndType endType)
{
   ASSERT_SPAN_KEY(spanKey);
   return GetCantileverLength(spanKey.spanIndex,spanKey.girderIndex,endType);
}

Float64 CBridgeAgentImp::GetSegmentStartEndDistance(const CSegmentKey& segmentKey)
{
   // distance from CL Bearing to start of girder, measured along CL Girder
   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);
   Float64 length;
   girder->get_LeftEndDistance(&length);
   return length;
}

Float64 CBridgeAgentImp::GetSegmentEndEndDistance(const CSegmentKey& segmentKey)
{
   // distance from CL Bearing to end of girder, measured along CL Girder
   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);
   Float64 length;
   girder->get_RightEndDistance(&length);
   return length;
}

Float64 CBridgeAgentImp::GetSegmentOffset(const CSegmentKey& segmentKey,Float64 station)
{
   VALIDATE( GIRDER );
   Float64 offset;
   HRESULT hr = m_BridgeGeometryTool->GirderPathOffset(m_Bridge,::GetSuperstructureMemberID(segmentKey.groupIndex,segmentKey.girderIndex),segmentKey.segmentIndex,CComVariant(station),&offset);
   ATLASSERT( SUCCEEDED(hr) );
   return offset;
}

void CBridgeAgentImp::GetPoint(const CSegmentKey& segmentKey,Float64 Xpoi,IPoint2d** ppPoint)
{
   VALIDATE( BRIDGE );
   CComPtr<IPoint2d> pntPier1,pntEnd1,pntBrg1,pntBrg2,pntEnd2,pntPier2;
   GetSegmentEndPoints(segmentKey,&pntPier1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntPier2);

   CComPtr<ILocate2> locate;
   m_CogoEngine->get_Locate(&locate);
   HRESULT hr = locate->PointOnLine(pntEnd1,pntEnd2,Xpoi,0.0,ppPoint);
   ATLASSERT( SUCCEEDED(hr) );
}

void CBridgeAgentImp::GetPoint(const pgsPointOfInterest& poi,IPoint2d** ppPoint)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   GetPoint(segmentKey,poi.GetDistFromStart(),ppPoint);
}

bool CBridgeAgentImp::GetSegmentPierIntersection(const CSegmentKey& segmentKey,PierIndexType pierIdx,IPoint2d** ppPoint)
{
   VALIDATE(BRIDGE);
   CComPtr<IGirderLine> girderLine;
   GetGirderLine(segmentKey,&girderLine);

   CComPtr<IPierLine> pierLine;
   GetPierLine(pierIdx,&pierLine);

   CComPtr<IPath> path;
   girderLine->get_Path(&path);

   CComPtr<ILine2d> line;
   pierLine->get_Centerline(&line);

   CComPtr<IPoint2d> nearest;
   pierLine->get_AlignmentPoint(&nearest);

   HRESULT hr = path->IntersectEx(line,nearest,VARIANT_FALSE,VARIANT_FALSE,ppPoint);
   if ( FAILED(hr) )
   {
      ATLASSERT(hr == COGO_E_NOINTERSECTION);
      return false;
   }
   return true;
}

bool CBridgeAgentImp::GetSegmentTempSupportIntersection(const CSegmentKey& segmentKey,SupportIndexType tsIdx,IPoint2d** ppPoint)
{
   VALIDATE(BRIDGE);
   CComPtr<IGirderLine> girderLine;
   GetGirderLine(segmentKey,&girderLine);

   // recall that temporary supports are modeled with pier line objects in the bridge geometry model
   CComPtr<IPierLine> pierLine;
   GetTemporarySupportLine(tsIdx,&pierLine);

   CComPtr<IPath> path;
   girderLine->get_Path(&path);

   CComPtr<ILine2d> line;
   pierLine->get_Centerline(&line);

   CComPtr<IPoint2d> nearest;
   pierLine->get_AlignmentPoint(&nearest);

   HRESULT hr = path->IntersectEx(line,nearest,VARIANT_FALSE,VARIANT_FALSE,ppPoint);
   if ( FAILED(hr) )
   {
      ATLASSERT(hr == COGO_E_NOINTERSECTION);
      return false;
   }
   return true;
}

void CBridgeAgentImp::GetStationAndOffset(const CSegmentKey& segmentKey,Float64 Xpoi,Float64* pStation,Float64* pOffset)
{
   VALIDATE( BRIDGE );

   CComPtr<IPoint2d> point;
   GetPoint(segmentKey,Xpoi,&point);

   GetStationAndOffset(point,pStation,pOffset);
}

void CBridgeAgentImp::GetStationAndOffset(const pgsPointOfInterest& poi,Float64* pStation,Float64* pOffset)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   GetStationAndOffset(segmentKey,poi.GetDistFromStart(),pStation,pOffset);
}

Float64 CBridgeAgentImp::ConvertSegmentToBridgeLineCoordinate(const CSegmentKey& segmentKey,Float64 Xs)
{
   return ConvertPoiToBridgeLineCoordinate(pgsPointOfInterest(segmentKey,Xs));
}

bool CBridgeAgentImp::IsInteriorGirder(const CGirderKey& girderKey)
{
   CComPtr<ISuperstructureMember> ssMbr;
   GetSuperstructureMember(girderKey,&ssMbr);

   LocationType location;
   ssMbr->get_LocationType(&location);

   return location == ltInteriorGirder ? true : false;
}

bool CBridgeAgentImp::IsExteriorGirder(const CGirderKey& girderKey)
{
   return !IsInteriorGirder(girderKey);
}

bool CBridgeAgentImp::IsLeftExteriorGirder(const CGirderKey& girderKey)
{
   CComPtr<ISuperstructureMember> ssMbr;
   GetSuperstructureMember(girderKey,&ssMbr);

   LocationType location;
   ssMbr->get_LocationType(&location);

   return location == ltLeftExteriorGirder ? true : false;
}

bool CBridgeAgentImp::IsRightExteriorGirder(const CGirderKey& girderKey)
{
   CComPtr<ISuperstructureMember> ssMbr;
   GetSuperstructureMember(girderKey,&ssMbr);

   LocationType location;
   ssMbr->get_LocationType(&location);

   return location == ltRightExteriorGirder ? true : false;
}

bool CBridgeAgentImp::IsObtuseCorner(const CSpanKey& spanKey,pgsTypes::MemberEndType endType)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanKey.spanIndex);
   GirderIndexType nGirders = pSpan->GetGirderCount();

   // in general, only the exterior and first interior girders are correctable
   bool bIsLeftSideCorrectableGirder = (spanKey.girderIndex < 2 ? true : false);
   bool bIsRightSideCorrectableGirder = (nGirders-2 <= spanKey.girderIndex ? true : false);
   if ( pBridgeDesc->GetDeckDescription()->DeckType == pgsTypes::sdtNone )
   {
      // There is not a composite deck so we consider this a deck system bridge.
      // Per LRFD 4.6.2.2.3c (2014) "In determining the end shear in deck system
      // bridges, the skew correction at the obtuse corner shall be applied to
      // all the beams". 
      bIsLeftSideCorrectableGirder  = true;
      bIsRightSideCorrectableGirder = true;
   }

   if ( !bIsLeftSideCorrectableGirder && !bIsRightSideCorrectableGirder )
   {
      // girder is not skew corrected so say we are not in an obtuse corner
      return false;
   }
   else
   {
      PierIndexType startPierIdx = pSpan->GetPrevPier()->GetIndex();
      PierIndexType endPierIdx   = pSpan->GetNextPier()->GetIndex();

      CComPtr<IAngle> objStartSkewAngle;
      GetPierSkew(startPierIdx,&objStartSkewAngle);
      Float64 startSkewAngle;
      objStartSkewAngle->get_Value(&startSkewAngle);

      CComPtr<IAngle> objEndSkewAngle;
      GetPierSkew(endPierIdx,&objEndSkewAngle);
      Float64 endSkewAngle;
      objEndSkewAngle->get_Value(&endSkewAngle);

      if ( IsZero(startSkewAngle) && IsZero(endSkewAngle) )
      {
         return false;
      }

      if ( ::Sign(startSkewAngle) == ::Sign(endSkewAngle) )
      {
         // Both ends are skewed the same direction
         if ( startSkewAngle < 0 )
         {
            // Both ends are skewed to the right

            //              --------------------------------------
            //             / -***------ gdrIdx = 0 ------------- /
            //            / -***------ gdrIdx = 1 ------------- /
            //           /                                     /
            //          /                                     /
            //         / ------ gdrIdx = nGirders-2-----***- /
            //        / ------ gdrIdx = nGirders-1 ----***- /
            //        --------------------------------------
            //
            // *** = Obtuse corner
            //
            if ( (endType == pgsTypes::metStart && bIsLeftSideCorrectableGirder) ||
                 (endType == pgsTypes::metEnd   && bIsRightSideCorrectableGirder )
               )
            {
               return true;
            }
         }
         else
         {
            // both ends are skewed to the left
            ATLASSERT(0 < startSkewAngle);
            ATLASSERT(0 < endSkewAngle);

            //   --------------------------------------
            //   \ ---------- gdrIdx = 0 ---------***- \
            //    \ ---------- gdrIdx = 1 ---------***- \
            //     \                                     \
            //      \                                     \
            //       \ -***-- gdrIdx = nGirders-2--------- \
            //        \ -***-- gdrIdx = nGirders-1 -------- \
            //         --------------------------------------
            //
            // *** = Obtuse corner
            //
            if ( (endType == pgsTypes::metStart && bIsRightSideCorrectableGirder) ||
                 (endType == pgsTypes::metEnd   && bIsLeftSideCorrectableGirder)
               )
            {
               return true;
            }
         }
      }
      else
      {
         // Ends are skewed in opposite directions
         //
         // Obtuse corners are the diagonally opposite corners that are closest together
         // (i.e. shortest distance)

         //   --------------------------------------------------
         //   \ ---------- gdrIdx = 0 ------------------------ /
         //    \ ---------- gdrIdx = 1 ---------------------- /
         //     \                                            /
         //      \                                          /
         //       \ ------ gdrIdx = nGirders-2------------ /
         //        \ ------ gdrIdx = nGirders-1 --------- /
         //         --------------------------------------

         if ( IsZero(startSkewAngle) )
         {
            // start of span is normal to alignment
            if ( endType == pgsTypes::metStart )
            {
               // can't have an obtuse corner for no skew
               return false;
            }

            ATLASSERT( !IsZero(endSkewAngle) );
            if ( (0 < endSkewAngle && bIsLeftSideCorrectableGirder) ||
                 (endSkewAngle < 0 && bIsRightSideCorrectableGirder) )
            {
               return true;
            }
            else
            {
               return false;
            }
         }

         if ( IsZero(endSkewAngle) )
         {
            // end of span is normal to alignment
            if ( endType == pgsTypes::metEnd )
            {
               // can't have an obtuse corner for no skew
               return false;
            }

            ATLASSERT( !IsZero(startSkewAngle) );
            if ( (0 < startSkewAngle && bIsRightSideCorrectableGirder) ||
                 (startSkewAngle < 0 && bIsLeftSideCorrectableGirder) )
            {
               return true;
            }
            else
            {
               return false;
            }
         }

         GroupIndexType grpIdx = GetGirderGroupAtPier((PierIndexType)spanKey.spanIndex,pgsTypes::Ahead);
         pgsPointOfInterest poiStartLeft  = GetPierPointOfInterest(CGirderKey(grpIdx,0),startPierIdx);
         pgsPointOfInterest poiEndLeft    = GetPierPointOfInterest(CGirderKey(grpIdx,0),endPierIdx);
         pgsPointOfInterest poiStartRight = GetPierPointOfInterest(CGirderKey(grpIdx,nGirders-1),startPierIdx);
         pgsPointOfInterest poiEndRight   = GetPierPointOfInterest(CGirderKey(grpIdx,nGirders-1),endPierIdx);

         CComPtr<IPoint2d> pntStartLeft, pntEndLeft, pntStartRight, pntEndRight;
         GetPoint(poiStartLeft, &pntStartLeft);
         GetPoint(poiEndLeft,   &pntEndLeft);
         GetPoint(poiStartRight,&pntStartRight);
         GetPoint(poiEndRight,  &pntEndRight);

         Float64 d1,d2;
         pntStartLeft->DistanceEx(pntEndRight,&d1);
         pntStartRight->DistanceEx(pntEndLeft,&d2);

         if ( IsEqual(d1,d2) )
         {
            // if the diagonals are the same length... both corners on one side are obtuse
            if ( startSkewAngle < 0 )
            {
               // obtuse corners are on the left side
               ATLASSERT( 0 <= endSkewAngle );
               if ( bIsLeftSideCorrectableGirder )
               {
                  return true;
               }
            }
            else
            {
               // obtuse corners are on the right side
               ATLASSERT( 0 <= startSkewAngle);
               ATLASSERT( endSkewAngle < 0 );
               if ( bIsRightSideCorrectableGirder )
               {
                  return true;
               }
            }
         }
         else if ( d1 < d2 )
         {
            // shortest distance is from start,left to end,right so obtuse corners are
            // at the start,right and end,left
            if ( (endType == pgsTypes::metStart && bIsLeftSideCorrectableGirder) ||
                 (endType == pgsTypes::metEnd && bIsRightSideCorrectableGirder) )
            {
               return true;
            }
         }
         else
         {
            ATLASSERT(d2 < d1);
            // shortest distance is from the start,right to end,left so the obtuse corners are
            // at the start,left and end,right
            if ( (endType == pgsTypes::metStart && bIsRightSideCorrectableGirder) ||
                 (endType == pgsTypes::metEnd && bIsLeftSideCorrectableGirder) )
            {
               return true;
            }
         }
      }
   }

   return false;
}

void CBridgeAgentImp::GetPierDiaphragmSize(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,Float64* pW,Float64* pH)
{
   VALIDATE( BRIDGE );
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData2* pPierData = pBridgeDesc->GetPier(pierIdx);
   ATLASSERT( pPierData );

   if ( pPierData->IsInteriorPier() )
   {
      // if this pier has a continuous segment connection type, there is only
      // one diaphragm at the pier... the data for that diaphragm is on the back side
      // the ahead side data isn't used.
      // Use half the width for the ahead and back sides
      *pH = pPierData->GetDiaphragmHeight(pgsTypes::Back);
      *pW = pPierData->GetDiaphragmWidth(pgsTypes::Back)/2;
   }
   else
   {
      *pH = pPierData->GetDiaphragmHeight(pierFace);
      *pW = pPierData->GetDiaphragmWidth(pierFace);
   }

   if ( *pH < 0 )
   {
      *pH = ComputePierDiaphragmHeight(pierIdx,pierFace);
   }

   if ( *pW < 0 )
   {
      *pW = ComputePierDiaphragmWidth(pierIdx,pierFace);
      if ( pPierData->IsInteriorPier() )
      {
         // see note above
         *pW /= 2;
      }
   }
}

Float64 CBridgeAgentImp::GetSegmentStartBearingOffset(const CSegmentKey& segmentKey)
{
   // returns distance from CL pier to the bearing line,
   // measured along CL girder
   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);
   Float64 length;
   girder->get_LeftBearingOffset(&length);
   return length;
}

Float64 CBridgeAgentImp::GetSegmentEndBearingOffset(const CSegmentKey& segmentKey)
{
   // returns distance from CL pier to the bearing line,
   // measured along CL girder
   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);
   Float64 length;
   girder->get_RightBearingOffset(&length);
   return length;
}

Float64 CBridgeAgentImp::GetSegmentStartSupportWidth(const CSegmentKey& segmentKey)
{
   // returns the support width
   // measured along the CL girder
   Float64 support_width;
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   const CClosureJointData* pClosure = pSegment->GetStartClosure();
   if ( pClosure )
   {
      if ( pClosure->GetPier() )
      {
         support_width = pClosure->GetPier()->GetSupportWidth(pgsTypes::Ahead);
      }
      else
      {
         support_width = pClosure->GetTemporarySupport()->GetSupportWidth();
      }
   }
   else
   {
      const CSpanData2* pSpan = pSegment->GetSpan(pgsTypes::metStart);
      support_width = pSpan->GetPrevPier()->GetSupportWidth(pgsTypes::Ahead);
   }

   return support_width;
}

Float64 CBridgeAgentImp::GetSegmentEndSupportWidth(const CSegmentKey& segmentKey)
{
   // returns the support width
   // measured along the CL girder
   Float64 support_width;
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   const CClosureJointData* pClosure = pSegment->GetEndClosure();
   if ( pClosure )
   {
      if ( pClosure->GetPier() )
      {
         support_width = pClosure->GetPier()->GetSupportWidth(pgsTypes::Back);
      }
      else
      {
         support_width = pClosure->GetTemporarySupport()->GetSupportWidth();
      }
   }
   else
   {
      const CSpanData2* pSpan = pSegment->GetSpan(pgsTypes::metEnd);
      support_width = pSpan->GetNextPier()->GetSupportWidth(pgsTypes::Back);
   }

   return support_width;
}

Float64 CBridgeAgentImp::GetCLPierToCLBearingDistance(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::MeasurementType measure)
{
   VALIDATE( BRIDGE );

   Float64 dist; // distance from CL pier to CL Bearing along the CL girder
   switch( endType )
   {
      case pgsTypes::metStart: // at start of span
         dist = GetSegmentStartBearingOffset(segmentKey);
         break;

      case pgsTypes::metEnd: // at end of span
         dist = GetSegmentEndBearingOffset(segmentKey);
         break;
   }

   if ( measure == pgsTypes::NormalToItem )
   {
      // we want dist to be measured normal to the pier
      // adjust the distance
      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);

      const CPierData2* pPier;
      const CTemporarySupportData* pTS;
      pSegment->GetSupport(endType,&pPier,&pTS);

      CComPtr<IDirection> supportDirection;

      if ( pPier )
      {
         GetPierDirection(pPier->GetIndex(),&supportDirection);
      }
      else
      {
         GetTemporarySupportDirection(pTS->GetIndex(),&supportDirection);
      }


      CComPtr<IDirection> dirSegment;
      GetSegmentBearing(segmentKey,&dirSegment);

      CComPtr<IAngle> angleBetween;
      supportDirection->AngleBetween(dirSegment,&angleBetween);

      Float64 angle;
      angleBetween->get_Value(&angle);

      dist *= sin(angle);
   }


   return dist;
}

Float64 CBridgeAgentImp::GetCLPierToSegmentEndDistance(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::MeasurementType measure)
{
   VALIDATE( BRIDGE );
   Float64 distCLPierToCLBearingAlongGirder = GetCLPierToCLBearingDistance(segmentKey,endType,pgsTypes::AlongItem);
   Float64 endDist;

   switch ( endType )
   {
   case pgsTypes::metStart: // at start of span
      endDist = GetSegmentStartEndDistance(segmentKey);
      break;

   case pgsTypes::metEnd: // at end of span
      endDist = GetSegmentEndEndDistance(segmentKey);
      break;
   }

   Float64 distCLPierToEndGirderAlongGirder = distCLPierToCLBearingAlongGirder - endDist;

   if ( measure == pgsTypes::NormalToItem )
   {
      // we want dist to be measured normal to the pier
      // adjust the distance
      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);

      const CPierData2* pPier;
      const CTemporarySupportData* pTS;
      pSegment->GetSupport(endType,&pPier,&pTS);

      CComPtr<IDirection> supportDirection;

      if ( pPier )
      {
         GetPierDirection(pPier->GetIndex(),&supportDirection);
      }
      else
      {
         GetTemporarySupportDirection(pTS->GetIndex(),&supportDirection);
      }


      CComPtr<IDirection> dirSegment;
      GetSegmentBearing(segmentKey,&dirSegment);

      CComPtr<IAngle> angleBetween;
      supportDirection->AngleBetween(dirSegment,&angleBetween);

      Float64 angle;
      angleBetween->get_Value(&angle);

      distCLPierToEndGirderAlongGirder *= sin(angle);
   }

   return distCLPierToEndGirderAlongGirder;
}

void CBridgeAgentImp::GetSegmentBearing(const CSegmentKey& segmentKey,IDirection** ppBearing)
{
   VALIDATE( GIRDER );
   CComPtr<IGirderLine> girderLine;
   GetGirderLine(segmentKey,&girderLine);
   girderLine->get_Direction(ppBearing);
}

void CBridgeAgentImp::GetSegmentAngle(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,IAngle** ppAngle)
{
   CComPtr<IDirection> gdrDir;
   GetSegmentBearing(segmentKey,&gdrDir);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);

   const CPierData2* pPier;
   const CTemporarySupportData* pTS;
   pSegment->GetSupport(endType,&pPier,&pTS);

   CComPtr<IDirection> supportDirection;

   if ( pPier )
   {
      GetPierDirection(pPier->GetIndex(),&supportDirection);
   }
   else
   {
      GetTemporarySupportDirection(pTS->GetIndex(),&supportDirection);
   }

   supportDirection->AngleBetween(gdrDir,ppAngle);
}

CSegmentKey CBridgeAgentImp::GetSegmentAtPier(PierIndexType pierIdx,const CGirderKey& girderKey)
{
   ATLASSERT(girderKey.girderIndex != ALL_GIRDERS);

   // Gets the girder segment that touches the pier 
   // When two segments touch a pier, the segment will be in the girder group defined by the girderKey
   // If both girders are in the same group, the left segment is returned
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx);
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);

      const CPierData2* pPierData = pBridgeDesc->GetPier(pierIdx);
      Float64 pierStation = pPierData->GetStation();

      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
         Float64 startStation,endStation;
         pSegment->GetStations(&startStation,&endStation);

         if ( ::InRange(startStation,pierStation,endStation) )
         {
            CSegmentKey segmentKey(girderKey,segIdx);
            return segmentKey;
         }
      } // next segment
   } // next group

   ATLASSERT(false); // should never get here
   return CSegmentKey();
}

void CBridgeAgentImp::GetSpansForSegment(const CSegmentKey& segmentKey,SpanIndexType* pStartSpanIdx,SpanIndexType* pEndSpanIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPrecastSegmentData* pSegment = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex)->GetGirder(segmentKey.girderIndex)->GetSegment(segmentKey.segmentIndex);
   const CSpanData2* pStartSpan = pSegment->GetSpan(pgsTypes::metStart);
   const CSpanData2* pEndSpan   = pSegment->GetSpan(pgsTypes::metEnd);

   *pStartSpanIdx = pStartSpan->GetIndex();
   *pEndSpanIdx   = pEndSpan->GetIndex();
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
GDRCONFIG CBridgeAgentImp::GetSegmentConfiguration(const CSegmentKey& segmentKey)
{
   // Make sure girder is properly modeled before beginning
   VALIDATE( GIRDER );

   GDRCONFIG config;

   config.SegmentKey = segmentKey;

   // Get the girder
   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

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

   // Get jacking force
   GET_IFACE(ISegmentData,pSegmentData);  
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
   const CGirderMaterial* pMaterial = pSegmentData->GetSegmentMaterial(segmentKey);

   pgsTypes::AdjustableStrandType adj_type = pStrands->GetAdjustableStrandType();
   config.PrestressConfig.AdjustableStrandType = adj_type;

   for ( Uint16 i = 0; i < 3; i++ )
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;

      config.PrestressConfig.Pjack[strandType] = pStrands->GetPjack(strandType);

      // Convert debond data
      // Use tool to compute strand position index from grid index used by CDebondInfo
      ConfigStrandFillTool fillTool( fillVec[strandType] );

      const std::vector<CDebondData>& vDebond(pStrands->GetDebonding(strandType));
      std::vector<CDebondData>::const_iterator iter(vDebond.begin());
      std::vector<CDebondData>::const_iterator end(vDebond.end());
      for ( ; iter != end; iter++ )
      {
         const CDebondData& debond_data = *iter;

         // convert grid index to strands index
         StrandIndexType index1, index2;
         fillTool.GridIndexToStrandPositionIndex(debond_data.strandTypeGridIdx, &index1, &index2);

         DEBONDCONFIG di;
         ATLASSERT(index1 != INVALID_INDEX);
         di.strandIdx         = index1;
         di.DebondLength[pgsTypes::metStart] = debond_data.Length[pgsTypes::metStart];
         di.DebondLength[pgsTypes::metEnd]   = debond_data.Length[pgsTypes::metEnd];

         config.PrestressConfig.Debond[i].push_back(di);

         if ( index2 != INVALID_INDEX )
         {
            di.strandIdx         = index2;
            di.DebondLength[pgsTypes::metStart] = debond_data.Length[pgsTypes::metStart];
            di.DebondLength[pgsTypes::metEnd]   = debond_data.Length[pgsTypes::metEnd];

            config.PrestressConfig.Debond[i].push_back(di);
         }
      }

      // sorts based on strand index
      std::sort(config.PrestressConfig.Debond[strandType].begin(),config.PrestressConfig.Debond[strandType].end());

      for ( Uint16 j = 0; j < 2; j++ )
      {
         pgsTypes::MemberEndType endType = pgsTypes::MemberEndType(j);
         const std::vector<GridIndexType>& gridIndicies(pStrands->GetExtendedStrands(strandType,endType));
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

   config.PrestressConfig.TempStrandUsage = pStrands->GetTemporaryStrandUsage();

   // Get concrete properties
   // NOTE: Since this is only for PGSuper (and not for time-step analysis) the concrete
   // model is LRFD and that is a "stepped" model (f'ci then steps up to f'c)
   IntervalIndexType releaseIntervalIdx = m_IntervalManager.GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType haulingIntervalIdx = m_IntervalManager.GetHaulingInterval(segmentKey); // steps up to f'c at hauling (see Concrete Manager)
   config.Fc       = GetSegmentFc(segmentKey,haulingIntervalIdx);
   config.Fci      = GetSegmentFc(segmentKey,releaseIntervalIdx);
   config.ConcType = GetSegmentConcreteType(segmentKey);
   config.bHasFct  = DoesSegmentConcreteHaveAggSplittingStrength(segmentKey);
   config.Fct      = GetSegmentConcreteAggSplittingStrength(segmentKey);

   config.Ec  = GetSegmentEc(segmentKey,haulingIntervalIdx);
   config.Eci = GetSegmentEc(segmentKey,releaseIntervalIdx);
   config.bUserEci = pMaterial->Concrete.bUserEci;
   config.bUserEc  = pMaterial->Concrete.bUserEc;

   // Slab offset
   GetSlabOffset(segmentKey,&config.SlabOffset[pgsTypes::metStart],&config.SlabOffset[pgsTypes::metEnd]);

   // Stirrup data
	const CShearData2* pShearData = GetShearData(segmentKey);

   WriteShearDataToStirrupConfig(pShearData, config.StirrupConfig);

   return config;
}

bool CBridgeAgentImp::DoesPierDiaphragmLoadGirder(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace)
{
   VALIDATE( BRIDGE );
  
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData2* pPierData = pBridgeDesc->GetPier(pierIdx);
   ATLASSERT( pPierData );

   // if there isn't a span on the specified face of this pier, then
   // the loads from the span on that face can't be applied
   if ( pPierData->GetSpan(pierFace) == NULL )
   {
      return false;
   }

   return (pPierData->GetDiaphragmLoadType(pierFace) != ConnectionLibraryEntry::DontApply);
}

Float64 CBridgeAgentImp::GetPierDiaphragmLoadLocation(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType)
{
   VALIDATE( BRIDGE );
  
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pSplicedGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const CPierData2* pPierData = pSplicedGirder->GetPier(endType);
   pgsTypes::PierFaceType pierFace = (endType == pgsTypes::metStart ? pgsTypes::Ahead : pgsTypes::Back);
#if defined _DEBUG
   if ( endType == pgsTypes::metStart )
   {
      ATLASSERT( segmentKey.segmentIndex == 0 );
   }
   else
   {
      ATLASSERT( segmentKey.segmentIndex == pSplicedGirder->GetSegmentCount()-1 );
   }
#endif

   Float64 dist;
   if (pPierData->GetDiaphragmLoadType(pierFace) == ConnectionLibraryEntry::ApplyAtSpecifiedLocation)
   {
      // return distance adjusted for skew
      dist = pPierData->GetDiaphragmLoadLocation(pierFace);

      CComPtr<IAngle> angle;
      GetPierSkew(pPierData->GetIndex(),&angle);
      Float64 value;
      angle->get_Value(&value);

      dist /=  cos ( fabs(value) );
   }
   else if (pPierData->GetDiaphragmLoadType(pierFace) == ConnectionLibraryEntry::ApplyAtBearingCenterline)
   {
      // same as bearing offset
      dist = GetSegmentEndBearingOffset(segmentKey);
   }
   else
   {
      dist = 0.0;
   }

   return dist;
}

std::vector<IntermedateDiaphragm> CBridgeAgentImp::GetPrecastDiaphragms(const CSegmentKey& segmentKey)
{
   ASSERT_SEGMENT_KEY(segmentKey);

   const GirderLibraryEntry* pGirderEntry = GetGirderLibraryEntry(segmentKey);

   Float64 segment_length   = GetSegmentLength(                segmentKey );
   bool   bIsInterior       = IsInteriorGirder(                segmentKey );

   Float64 span_length = GetSegmentLayoutLength(segmentKey);

   // base the span length on the maximum span length in this span
   // we want the same number of diaphragms on every girder
   GirderIndexType nGirders = this->GetGirderCount(segmentKey.groupIndex);
   Float64 max_span_length = 0;
   for ( GirderIndexType i = 0; i < nGirders; i++ )
   {
      max_span_length = Max(max_span_length, GetSegmentSpanLength( CSegmentKey(segmentKey.groupIndex,i,segmentKey.segmentIndex)) );
   }


   // These are indicies into the generic bridge object. They could be piers or temporary support
   PierIndexType startPierIdx = GetGenericBridgePierIndex(segmentKey,pgsTypes::metStart);
   PierIndexType endPierIdx   = GetGenericBridgePierIndex(segmentKey,pgsTypes::metEnd);

   // get the actual generic bridge pier objects
   CComPtr<IPierCollection> piers;
   m_Bridge->get_Piers(&piers);
   CComPtr<IBridgePier> objPier1, objPier2;
   piers->get_Item(startPierIdx,&objPier1);
   piers->get_Item(endPierIdx,  &objPier2);

   // get the skew angles
   CComPtr<IAngle> objSkew1, objSkew2;
   objPier1->get_SkewAngle(&objSkew1);
   objPier2->get_SkewAngle(&objSkew2);

   Float64 skew1, skew2;
   objSkew1->get_Value(&skew1);
   objSkew2->get_Value(&skew2);

   std::vector<IntermedateDiaphragm> diaphragms;

   const GirderLibraryEntry::DiaphragmLayoutRules& rules = pGirderEntry->GetDiaphragmLayoutRules();

   GirderLibraryEntry::DiaphragmLayoutRules::const_iterator ruleIter(rules.begin());
   GirderLibraryEntry::DiaphragmLayoutRules::const_iterator ruleIterEnd(rules.end());
   for ( ; ruleIter != ruleIterEnd; ruleIter++ )
   {
      const GirderLibraryEntry::DiaphragmLayoutRule& rule = *ruleIter;

      if ( rule.Construction != GirderLibraryEntry::ctCastingYard )
      {
         continue; // not our kind of rule
      }

      if ( max_span_length <= rule.MinSpan || rule.MaxSpan < max_span_length )
      {
         continue; // this rule doesn't apply
      }

      // determine location of diaphragm load (from the reference point - whatever that is, see below)
      Float64 distance;
      ATLASSERT( rule.MeasureType != GirderLibraryEntry::mtFractionOfSpanLength );
      if ( rule.MeasureType == GirderLibraryEntry::mtFractionOfGirderLength )
      {
         distance = rule.Location*segment_length;
      }
      else if ( rule.MeasureType == GirderLibraryEntry::mtAbsoluteDistance )
      {
         distance = rule.Location;
      }

      // adjust location so that it is measured from the start end of the segment
      ATLASSERT( rule.MeasureLocation != GirderLibraryEntry::mlBearing );
      Float64 location1,location2;
      if ( rule.MeasureLocation == GirderLibraryEntry::mlCenterlineOfGirder )
      {
         // reference point is the center line of the girder so go back from the centerline
         location1 = segment_length/2 - distance;
         location2 = segment_length/2 + distance; // locate the diaphragm -/+ from cl girder
      }
      else
      {
         ATLASSERT(rule.MeasureLocation == GirderLibraryEntry::mlEndOfGirder);
         location1 = distance;
         location2 = segment_length - distance;
      }

      if ( location1 < 0.0 || span_length < location1 || location2 < 0.0 || span_length < location2 )
      {
         GET_IFACE(IEAFStatusCenter,pStatusCenter);
         std::_tstring str(_T("An interior diaphragm is located off the precast element. The diaphragm load will not be applied. Check the diaphragm rules."));

         pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID,m_scidInformationalError,str.c_str());
         pStatusCenter->Add(pStatusItem);
         break;
      }

      for ( int i = 0; i < 2; i++ )
      {
         if ( i == 1 && IsEqual(location1,location2) )
         {
            break;
         }

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

         // location the diaphragm
         diaphragm.Location = (i == 0 ? location1 : location2);
         Float64 skew = ::LinInterp(diaphragm.Location,skew1,skew2,span_length);
         pgsPointOfInterest poi(segmentKey,diaphragm.Location);

         if ( diaphragm.m_bCompute )
         {
            // determine length (width) of the diaphragm
            Float64 W = 0;
            WebIndexType nWebs = GetWebCount(segmentKey);
            if ( 1 < nWebs )
            {
               SpacingIndexType nSpaces = nWebs-1;

               // add up the spacing between the centerlines of webs in the girder cross section
               for ( SpacingIndexType spaceIdx = 0; spaceIdx < nSpaces; spaceIdx++ )
               {
                  Float64 s = GetWebSpacing(poi,spaceIdx);
                  W += s;
               }

               // deduct the thickness of the webs
               for ( WebIndexType webIdx = 0; webIdx < nWebs; webIdx++ )
               {
                  Float64 t = GetWebThickness(poi,webIdx);

                  if ( webIdx == 0 || webIdx == nWebs-1 )
                  {
                     W -= t/2;
                  }
                  else
                  {
                     W -= t;
                  }
               }

               diaphragm.W = W/cos(skew);
            }
         }
         diaphragms.push_back(diaphragm);
      } // next diaphragm
   } // next rule

   return diaphragms;
}

std::vector<IntermedateDiaphragm> CBridgeAgentImp::GetCastInPlaceDiaphragms(const CSpanKey& spanKey)
{
   // NOTE: in future versions we may want to update the diaphragm model to match the model in TxDOT BGS
   // right now it is assumed that diaphramgs for a straight line. BGS and the Bridge Geometry Model support
   // more robust layouts

   ASSERT_SPAN_KEY(spanKey);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanKey.spanIndex);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   const GirderLibraryEntry* pGirderEntry = pGroup->GetGirderLibraryEntry(spanKey.girderIndex);

   CGirderKey girderKey(pGroup->GetIndex(),spanKey.girderIndex);

   SegmentIndexType nSegments = pGroup->GetGirder(spanKey.girderIndex)->GetSegmentCount();

   Float64 span_length   = GetSpanLength(spanKey);
   Float64 girder_length = GetGirderLength(girderKey);

   Float64 start_end_size = 0; // distance from start CL Bearing to start face of girder
   // if this span is in the middle of a group (ie. the girder doesn't start or end in this span,
   // then the end size is zero because the girder spans over the piers)
   if ( pGroup->GetPierIndex(pgsTypes::metStart) == spanKey.spanIndex )
   {
      start_end_size = GetSegmentStartEndDistance(CSegmentKey(girderKey,0));
   }

   Float64 end_end_size = 0; // distance from end CL Bearing to end face of girder
   if ( pGroup->GetPierIndex(pgsTypes::metEnd) == spanKey.spanIndex+1 )
   {
      end_end_size = GetSegmentEndEndDistance(CSegmentKey(girderKey,nSegments-1));
   }

   bool bIsInterior = IsInteriorGirder( girderKey );

   // base the span length on the maximum span length in this span
   // we want the same number of diaphragms on every girder
   GirderIndexType nGirders = pGroup->GetGirderCount();
   Float64 max_span_length = 0;
   for ( GirderIndexType i = 0; i < nGirders; i++ )
   {
      SegmentIndexType segIdx = 0;
      max_span_length = Max(max_span_length, GetSpanLength( CSpanKey(spanKey.spanIndex,i) ));
   }

   PierIndexType startPierIdx = spanKey.spanIndex;
   PierIndexType endPierIdx   = startPierIdx+1;

   CComPtr<IAngle> objSkew1, objSkew2;
   GetPierSkew(startPierIdx,&objSkew1);
   GetPierSkew(endPierIdx,&objSkew2);

   Float64 skew1, skew2;
   objSkew1->get_Value(&skew1);
   objSkew2->get_Value(&skew2);

   std::vector<IntermedateDiaphragm> diaphragms;

   const GirderLibraryEntry::DiaphragmLayoutRules& rules = pGirderEntry->GetDiaphragmLayoutRules();

   GirderLibraryEntry::DiaphragmLayoutRules::const_iterator ruleIter(rules.begin());
   GirderLibraryEntry::DiaphragmLayoutRules::const_iterator ruleIterEnd(rules.end());
   for ( ; ruleIter != ruleIterEnd; ruleIter++ )
   {
      const GirderLibraryEntry::DiaphragmLayoutRule& rule = *ruleIter;

      if ( rule.Construction != GirderLibraryEntry::ctBridgeSite )
      {
         continue; // not our kind of rule
      }

      if ( max_span_length <= rule.MinSpan || rule.MaxSpan < max_span_length )
      {
         continue; // this rule doesn't apply
      }

      // determine location of diaphragm load (from the reference point - whatever that is, see below)
      Float64 location1, location2;
      if ( rule.MeasureType == GirderLibraryEntry::mtFractionOfSpanLength )
      {
         location1 = rule.Location*span_length;
         location2 = (1 - rule.Location)*span_length;
      }
      else if ( rule.MeasureType == GirderLibraryEntry::mtFractionOfGirderLength )
      {
         location1 = rule.Location*girder_length;
         location2 = (1 - rule.Location)*girder_length;
      }
      else if ( rule.MeasureType == GirderLibraryEntry::mtAbsoluteDistance )
      {
         location1 = rule.Location;
         if ( 1 < nSegments )
         {
            location2 = span_length - rule.Location;
         }
         else
         {
            location2 = girder_length - rule.Location;
         }
      }

      // adjust location so that it is measured from the CL Bearing
      if ( rule.MeasureLocation == GirderLibraryEntry::mlBearing )
      {
         // do nothing
      }
      else if ( rule.MeasureLocation == GirderLibraryEntry::mlCenterlineOfGirder )
      {
         // reference point is the center line of the girder or span so go back from the centerline

         if ( 1 < nSegments )
         {
            // for spliced girders, mlCenterlineOfGirder actually means CL Span since the
            // girders are more than one span long (in general). The diaphragm rule definition 
            // dialog for the girder library entry restricts the input such that the meaning
            // of mlCenterlineOfGirder is CL Span for spliced girders
#if defined _DEBUG
            GET_IFACE(IDocumentType,pDocType);
            ATLASSERT(pDocType->IsPGSpliceDocument());
#endif
            // locate the diaphragm -/+ from cl span
            location1 = span_length/2 - rule.Location;
            location2 = span_length/2 + rule.Location;
         }
         else
         {
#if defined _DEBUG
            GET_IFACE(IDocumentType,pDocType);
            ATLASSERT(pDocType->IsPGSuperDocument());
            ATLASSERT(nSegments == 1);
#endif
            // locate the diaphragm -/+ from cl girder
            location1 = girder_length/2 - rule.Location;
            location2 = girder_length/2 + rule.Location;

            // location is from the start face of the girder.. make it from the CL Bearing = Start of Span
            location1 -= start_end_size;
            location2 -= start_end_size;
         }
      }

      if ( location1 < 0.0 || span_length < location1 || location2 < 0.0 || span_length < location2 )
      {
         GET_IFACE(IEAFStatusCenter,pStatusCenter);
         std::_tstring str(_T("An interior diaphragm is located outside of the span length. The diaphragm load will not be applied. Check the diaphragm rules for this girder."));

         pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID,m_scidInformationalError,str.c_str());
         pStatusCenter->Add(pStatusItem);
         break;
      }

      for ( int i = 0; i < 2; i++ )
      {
         if ( i == 1 && IsEqual(location1,location2) )
         {
            // don't add two identical diaphragms at the same location
            break;
         }

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

         // location the diaphragm
         diaphragm.Location = (i == 0 ? location1 : location2);
         Float64 skew = ::LinInterp(diaphragm.Location,skew1,skew2,start_end_size + span_length + end_end_size);

         pgsPointOfInterest poi = ConvertGirderCoordinateToPoi(girderKey,diaphragm.Location);

         Float64 left_spacing, right_spacing;
         GetSpacingAlongGirder(girderKey,diaphragm.Location,&left_spacing,&right_spacing);

         ATLASSERT( rule.Type == GirderLibraryEntry::dtExternal );
         
         // determine length (width) of the diaphragm as girder spacing minus web width
         Float64 W = 0;
         WebIndexType nWebs = GetWebCount(girderKey);

         Float64 web_location_left  = GetWebLocation(poi,0);
         Float64 web_location_right = GetWebLocation(poi,nWebs-1);

         Float64 tweb_left  = GetWebThickness(poi,0)/2;
         Float64 tweb_right = GetWebThickness(poi,nWebs-1)/2;

         if ( bIsInterior )
         {
            W = (left_spacing/2 + right_spacing/2 - fabs(web_location_left) - tweb_left - fabs(web_location_right) - tweb_right);
         }
         else
         {
            W = (spanKey.girderIndex == 0 ? right_spacing/2 - fabs(web_location_right) - tweb_right : left_spacing/2 - fabs(web_location_left) - tweb_left );
         }

         diaphragm.W = W/cos(skew);

         if ( !diaphragm.m_bCompute )
         {
            // P is weight/length of external application
            // make P the total weight here
            diaphragm.P *= diaphragm.W;
         }

         diaphragms.push_back(diaphragm);
      } // next diaphragm
   } // next rule

   return diaphragms;
}

pgsTypes::SupportedDeckType CBridgeAgentImp::GetDeckType()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2*   pDeck       = pBridgeDesc->GetDeckDescription();
   return pDeck->DeckType;
}

Float64 CBridgeAgentImp::GetSlabOffset(GroupIndexType grpIdx,PierIndexType pierIdx,GirderIndexType gdrIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData*    pGroup      = pBridgeDesc->GetGirderGroup(grpIdx);

   return pGroup->GetSlabOffset(pierIdx,gdrIdx);
}

Float64 CBridgeAgentImp::GetSlabOffset(const pgsPointOfInterest& poi)
{
   IntervalIndexType castDeckIntervalIdx = m_IntervalManager.GetCastDeckInterval();
   Float64 d = GetElevationAdjustment(castDeckIntervalIdx,poi); // this is the relative change from "A" at the start of the bridge

   Float64 Astart = GetSlabOffset(0,0,poi.GetSegmentKey().girderIndex);

   Float64 A = Astart - d;

   return A;
}

Float64 CBridgeAgentImp::GetSlabOffset(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   // compute the "A" dimension at the poi by linearly interpolating the value
   // between the start and end bearings (this assumes no camber in the girder)
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   Float64 Xpoi = poi.GetDistFromStart();

   Float64 Astart = config.SlabOffset[pgsTypes::metStart];
   Float64 Aend   = config.SlabOffset[pgsTypes::metEnd];
   Float64 span_length = GetSegmentSpanLength(segmentKey);

   Float64 dist_to_start_brg = GetSegmentStartEndDistance(segmentKey);

   Float64 dist_from_brg_to_poi = Xpoi - dist_to_start_brg;

   Float64 slab_offset = ::LinInterp(dist_from_brg_to_poi,Astart,Aend,span_length);

   return slab_offset;
}

void CBridgeAgentImp::GetSlabOffset(const CSegmentKey& segmentKey,Float64* pStart,Float64* pEnd)
{
   ASSERT_SEGMENT_KEY(segmentKey);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);

   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   const CPierData2* pStartPier;
   const CTemporarySupportData* pStartTS;
   pSegment->GetSupport(pgsTypes::metStart,&pStartPier,&pStartTS);

   const CPierData2* pEndPier;
   const CTemporarySupportData* pEndTS;
   pSegment->GetSupport(pgsTypes::metEnd,&pEndPier,&pEndTS);

   const CGirderGroupData* pGroup = pSegment->GetGirder()->GetGirderGroup();

   Float64 Ao = 0;
   if ( pStartTS != NULL || pEndTS != NULL )
   {
      const CPierData2* pFirstPier = pIBridgeDesc->GetPier(0);
      Ao = pIBridgeDesc->GetGirderGroup(0)->GetSlabOffset(0,segmentKey.girderIndex);
   }

   if ( pStartPier )
   {
      *pStart = pGroup->GetSlabOffset(pStartPier->GetIndex(),segmentKey.girderIndex);
   }
   else
   {
      Float64 D = pStartTS->GetElevationAdjustment();
      *pStart = Ao - D;
   }

   if ( pEndPier )
   {
      *pEnd = pGroup->GetSlabOffset(pEndPier->GetIndex(),segmentKey.girderIndex);
   }
   else
   {
      Float64 D = pEndTS->GetElevationAdjustment();
      *pEnd = Ao - D;
   }
}

Float64 CBridgeAgentImp::GetElevationAdjustment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   VALIDATE(BRIDGE);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   Float64 elevAdj = 0;
   IntervalIndexType erectSegmentIntervalIdx = GetErectSegmentInterval(segmentKey);
   if ( erectSegmentIntervalIdx <= intervalIdx )
   {
      std::map<CSegmentKey,mathLinFunc2d>::iterator found(m_ElevationAdjustmentEquations.find(segmentKey));
      ATLASSERT(found != m_ElevationAdjustmentEquations.end());

      mathLinFunc2d& fn = found->second;

      Float64 Xpoi = poi.GetDistFromStart();
      elevAdj = fn.Evaluate(Xpoi);
   }

   return elevAdj;
}

Float64 CBridgeAgentImp::GetRotationAdjustment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   VALIDATE(BRIDGE);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   Float64 slopeAdj = 0;
   IntervalIndexType erectSegmentIntervalIdx = GetErectSegmentInterval(segmentKey);
   if ( erectSegmentIntervalIdx <= intervalIdx )
   {
      std::map<CSegmentKey,mathLinFunc2d>::iterator found(m_ElevationAdjustmentEquations.find(segmentKey));
      ATLASSERT(found != m_ElevationAdjustmentEquations.end());

      mathLinFunc2d& fn = found->second;

      slopeAdj = fn.GetSlope();
   }

   return slopeAdj;
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
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   return (pDeck->WearingSurface == pgsTypes::wstFutureOverlay || pDeck->WearingSurface == pgsTypes::wstOverlay);
}

bool CBridgeAgentImp::IsFutureOverlay()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   if ( pDeck->WearingSurface == pgsTypes::wstSacrificialDepth )
   {
      return false; // definately not a future overlay (not even an overlay)
   }

   if (pDeck->WearingSurface == pgsTypes::wstFutureOverlay)
   {
      return true; // explicitly an overlay
   }

   ATLASSERT(pDeck->WearingSurface == pgsTypes::wstOverlay);

   IntervalIndexType overlayIntervalIdx  = GetOverlayInterval();
   IntervalIndexType liveLoadIntervalIdx = GetLiveLoadInterval();
   if ( liveLoadIntervalIdx < overlayIntervalIdx )
   {
      return true; // overlay is applied after the bridge is open to traffic so it is a future overlay
   }

   return false; // has an overlay, but it goes not before at the interval when the bridge opens to traffic
}

Float64 CBridgeAgentImp::GetOverlayWeight()
{
   Float64 depth, density;
   m_Bridge->get_WearingSurfaceDepth(&depth);
   m_Bridge->get_WearingSurfaceDensity(&density);
   
   Float64 weight = depth*density*unitSysUnitsMgr::GetGravitationalAcceleration();
   return weight;
}

Float64 CBridgeAgentImp::GetOverlayDepth()
{
   Float64 depth;
   m_Bridge->get_WearingSurfaceDepth(&depth);
   return depth;
}

Float64 CBridgeAgentImp::GetSacrificalDepth()
{
   Float64 depth;
   m_Bridge->get_SacrificialDepth(&depth);

#if defined _DEBUG
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   if ( HasOverlay() && !IsFutureOverlay() )
   {
      ATLASSERT(IsZero(depth));
   }
   else
   {
      ATLASSERT(IsEqual(depth,pDeck->SacrificialDepth));
   }

#endif

   return depth;
}

Float64 CBridgeAgentImp::GetFillet()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();
   if ( pDeck->DeckType == pgsTypes::sdtNone )
   {
      return 0.0;
   }
   else
   {
      return pDeck->Fillet;
   }
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
   {
      return 0.00;
   }

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

Float64 CBridgeAgentImp::GetLeftSlabOverhang(Float64 Xb)
{
   VALIDATE( BRIDGE );
   Float64 left, right;
   HRESULT hr = GetSlabOverhangs(Xb,&left,&right);
   ATLASSERT(hr == S_OK);
   return left;
}

Float64 CBridgeAgentImp::GetRightSlabOverhang(Float64 Xb)
{
   VALIDATE( BRIDGE );
   Float64 left, right;
   HRESULT hr = GetSlabOverhangs(Xb,&left,&right);
   ATLASSERT(hr == S_OK);
   return right;
}

Float64 CBridgeAgentImp::GetLeftSlabOverhang(SpanIndexType spanIdx,Float64 Xspan)
{
   VALIDATE( BRIDGE );
   Float64 pierStation = this->GetPierStation(spanIdx);
   Float64 station = pierStation + Xspan;
   Float64 firstPierStation = GetPierStation(0);
   Float64 Xb = station - firstPierStation;
   return GetLeftSlabOverhang(Xb);
}

Float64 CBridgeAgentImp::GetRightSlabOverhang(SpanIndexType spanIdx,Float64 Xspan)
{
   VALIDATE( BRIDGE );
   Float64 pierStation = GetPierStation(spanIdx);
   Float64 station = pierStation + Xspan;
   Float64 firstPierStation = GetPierStation(0);
   Float64 Xb = station - firstPierStation;
   return GetRightSlabOverhang(Xb);
}

Float64 CBridgeAgentImp::GetLeftSlabOverhang(PierIndexType pierIdx)
{
   VALIDATE( BRIDGE );

   if ( GetDeckType() == pgsTypes::sdtNone )
   {
      return 0.0;
   }

   pgsTypes::PierFaceType pierFace = pgsTypes::Ahead;
   if ( pierIdx == 0 )
   {
      pierFace = pgsTypes::Ahead;
   }
   else if ( pierIdx == GetSpanCount() )
   {
      pierFace = pgsTypes::Back;
   }

   GroupIndexType grpIdx = GetGirderGroupAtPier(pierIdx,pierFace);
   GirderIDType gdrID = ::GetSuperstructureMemberID(grpIdx,0);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
   Float64 start_of_group_station = pGroup->GetPier(pgsTypes::metStart)->GetStation();

   Float64 pier_station = GetPierStation(pierIdx);

   Float64 distance = pier_station - start_of_group_station;

   Float64 overhang;
   HRESULT hr = m_BridgeGeometryTool->DeckOverhangBySSMbr(m_Bridge,gdrID,distance,NULL,qcbLeft,&overhang);
   ATLASSERT( SUCCEEDED(hr) );

   return overhang;
}

Float64 CBridgeAgentImp::GetRightSlabOverhang(PierIndexType pierIdx)
{
   VALIDATE( BRIDGE );

   if ( GetDeckType() == pgsTypes::sdtNone )
   {
      return 0.0;
   }

   pgsTypes::PierFaceType pierFace = pgsTypes::Ahead;
   if ( pierIdx == 0 )
   {
      pierFace = pgsTypes::Ahead;
   }
   else if ( pierIdx == GetSpanCount() )
   {
      pierFace = pgsTypes::Back;
   }

   GroupIndexType grpIdx = GetGirderGroupAtPier(pierIdx,pierFace);
   GirderIDType gdrID = ::GetSuperstructureMemberID(grpIdx,0);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
   Float64 start_of_group_station = pGroup->GetPier(pgsTypes::metStart)->GetStation();

   Float64 pier_station = GetPierStation(pierIdx);

   Float64 distance = pier_station - start_of_group_station;
   
   Float64 overhang;
   HRESULT hr = m_BridgeGeometryTool->DeckOverhangBySSMbr(m_Bridge,gdrID,distance,NULL,qcbRight,&overhang);
   ATLASSERT( SUCCEEDED(hr) );

   return overhang;
}

Float64 CBridgeAgentImp::GetLeftSlabEdgeOffset(PierIndexType pierIdx)
{
   VALIDATE( BRIDGE );
   Float64 start_station = GetPierStation(0);
   Float64 pier_station  = GetPierStation(pierIdx);
   Float64 Xb = pier_station - start_station;
   return GetLeftSlabEdgeOffset(Xb);
}

Float64 CBridgeAgentImp::GetRightSlabEdgeOffset(PierIndexType pierIdx)
{
   VALIDATE( BRIDGE );
   Float64 start_station = GetPierStation(0);
   Float64 pier_station  = GetPierStation(pierIdx);
   Float64 Xb = pier_station - start_station;
   return GetRightSlabEdgeOffset(Xb);
}

Float64 CBridgeAgentImp::GetLeftSlabEdgeOffset(Float64 Xb)
{
   std::map<Float64,Float64>::iterator found = m_LeftSlabEdgeOffset.find(Xb);
   if ( found != m_LeftSlabEdgeOffset.end() )
   {
      return found->second;
   }

   VALIDATE( BRIDGE );
   Float64 station = GetPierStation(0);
   station += Xb;
   CComPtr<IStation> offsetStation;
   Float64 offset;
   m_BridgeGeometryTool->DeckOffset(m_Bridge,station,NULL,qcbLeft,&offsetStation,&offset);

   m_LeftSlabEdgeOffset.insert(std::make_pair(Xb,offset));

   return offset;
}

Float64 CBridgeAgentImp::GetRightSlabEdgeOffset(Float64 Xb)
{
   std::map<Float64,Float64>::iterator found = m_RightSlabEdgeOffset.find(Xb);
   if ( found != m_RightSlabEdgeOffset.end() )
   {
      return found->second;
   }

   VALIDATE( BRIDGE );
   Float64 station = GetPierStation(0);
   station += Xb;
   CComPtr<IStation> offsetStation;
   Float64 offset;
   m_BridgeGeometryTool->DeckOffset(m_Bridge,station,NULL,qcbRight,&offsetStation,&offset);

   m_RightSlabEdgeOffset.insert(std::make_pair(Xb,offset));

   return offset;
}

Float64 CBridgeAgentImp::GetLeftCurbOffset(Float64 Xb)
{
   VALIDATE( BRIDGE );
   Float64 station = GetPierStation(0);
   station += Xb;
   CComPtr<IStation> offsetStation;
   Float64 offset;
   m_BridgeGeometryTool->CurbOffset(m_Bridge,station,NULL,qcbLeft,&offsetStation,&offset);
   return offset;
}

Float64 CBridgeAgentImp::GetRightCurbOffset(Float64 Xb)
{
   VALIDATE( BRIDGE );
   Float64 station = GetPierStation(0);
   station += Xb;
   CComPtr<IStation> offsetStation;
   Float64 offset;
   m_BridgeGeometryTool->CurbOffset(m_Bridge,station,NULL,qcbRight,&offsetStation,&offset);
   return offset;
}

Float64 CBridgeAgentImp::GetLeftCurbOffset(SpanIndexType spanIdx,Float64 Xspan)
{
   Float64 pierStation = GetPierStation(spanIdx);
   Float64 station = pierStation + Xspan;
   Float64 firstPierStation = GetPierStation(0);
   Float64 Xb = station - firstPierStation;
   return GetLeftCurbOffset(Xb);
}

Float64 CBridgeAgentImp::GetRightCurbOffset(SpanIndexType spanIdx,Float64 Xspan)
{
   Float64 pierStation = GetPierStation(spanIdx);
   Float64 station = pierStation + Xspan;
   Float64 firstPierStation = GetPierStation(0);
   Float64 Xb = station - firstPierStation;
   return GetRightCurbOffset(Xb);
}

Float64 CBridgeAgentImp::GetLeftCurbOffset(PierIndexType pierIdx)
{
   Float64 start_station = GetPierStation(0);
   Float64 pier_station = GetPierStation(pierIdx);
   Float64 Xb = pier_station - start_station;
   return GetLeftCurbOffset(Xb);
}

Float64 CBridgeAgentImp::GetRightCurbOffset(PierIndexType pierIdx)
{
   Float64 start_station = GetPierStation(0);
   Float64 pier_station = GetPierStation(pierIdx);
   Float64 Xb = pier_station - start_station;
   return GetRightCurbOffset(Xb);
}

Float64 CBridgeAgentImp::GetCurbToCurbWidth(const pgsPointOfInterest& poi)
{
   Float64 Xb = ConvertPoiToBridgeLineCoordinate(poi);
   return GetCurbToCurbWidth(Xb);
}

Float64 CBridgeAgentImp::GetCurbToCurbWidth(const CSegmentKey& segmentKey,Float64 Xs)
{
   Float64 Xb = ConvertSegmentToBridgeLineCoordinate(segmentKey,Xs);
   return GetCurbToCurbWidth(Xb);
}

Float64 CBridgeAgentImp::GetCurbToCurbWidth(Float64 Xb)
{
   Float64 left_offset, right_offset;
   left_offset = GetLeftCurbOffset(Xb);
   right_offset = GetRightCurbOffset(Xb);
   return right_offset - left_offset;
}

Float64 CBridgeAgentImp::GetLeftInteriorCurbOffset(Float64 Xb)
{
   VALIDATE( BRIDGE );
   Float64 station = GetPierStation(0);
   station += Xb;
   CComPtr<IStation> offsetStation;
   Float64 offset;
   m_BridgeGeometryTool->InteriorCurbOffset(m_Bridge,station,NULL,qcbLeft,&offsetStation,&offset);
   return offset;
}

Float64 CBridgeAgentImp::GetRightInteriorCurbOffset(Float64 Xb)
{
   VALIDATE( BRIDGE );
   Float64 station = GetPierStation(0);
   station += Xb;
   CComPtr<IStation> offsetStation;
   Float64 offset;
   m_BridgeGeometryTool->InteriorCurbOffset(m_Bridge,station,NULL,qcbRight,&offsetStation,&offset);
   return offset;
}

Float64 CBridgeAgentImp::GetLeftOverlayToeOffset(Float64 Xb)
{
   Float64 slab_edge = GetLeftSlabEdgeOffset(Xb);

   CComPtr<ISidewalkBarrier> pSwBarrier;
   m_Bridge->get_LeftBarrier(&pSwBarrier);

   Float64 toe_width;
   pSwBarrier->get_OverlayToeWidth(&toe_width);

   return slab_edge + toe_width;
}

Float64 CBridgeAgentImp::GetRightOverlayToeOffset(Float64 Xb)
{
   Float64 slab_edge = GetRightSlabEdgeOffset(Xb);

   CComPtr<ISidewalkBarrier> pSwBarrier;
   m_Bridge->get_RightBarrier(&pSwBarrier);

   Float64 toe_width;
   pSwBarrier->get_OverlayToeWidth(&toe_width);

   return slab_edge - toe_width;
}

Float64 CBridgeAgentImp::GetLeftOverlayToeOffset(const pgsPointOfInterest& poi)
{
   Float64 Xb = ConvertPoiToBridgeLineCoordinate(poi);
   return GetLeftOverlayToeOffset(Xb);
}

Float64 CBridgeAgentImp::GetRightOverlayToeOffset(const pgsPointOfInterest& poi)
{
   Float64 Xb = ConvertPoiToBridgeLineCoordinate(poi);
   return GetRightOverlayToeOffset(Xb);
}

void CBridgeAgentImp::GetSlabPerimeter(CollectionIndexType nPoints,IPoint2dCollection** points)
{
   VALIDATE( BRIDGE );

   CComPtr<IBridgeGeometry> geometry;
   m_Bridge->get_BridgeGeometry(&geometry);

   CComPtr<IDeckBoundary> deckBoundary;
   geometry->get_DeckBoundary(&deckBoundary);

   deckBoundary->get_Perimeter(nPoints,points);
}

void CBridgeAgentImp::GetSpanPerimeter(SpanIndexType spanIdx,CollectionIndexType nPoints,IPoint2dCollection** points)
{
   VALIDATE( BRIDGE );

   PierIndexType startPierIdx = spanIdx;
   PierIndexType endPierIdx   = startPierIdx + 1;

   CComPtr<IBridgeGeometry> geometry;
   m_Bridge->get_BridgeGeometry(&geometry);

   CComPtr<IDeckBoundary> deckBoundary;
   geometry->get_DeckBoundary(&deckBoundary);

   PierIDType startPierID = ::GetPierLineID(startPierIdx);
   PierIDType endPierID   = ::GetPierLineID(endPierIdx);
   
   deckBoundary->get_PerimeterEx(nPoints,startPierID,endPierID,points);
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

void CBridgeAgentImp::GetLeftCurbLinePoint(Float64 station, IDirection* direction,IPoint2d** point)
{
   GetCurbLinePoint(station,direction,qcbLeft,point);
}

void CBridgeAgentImp::GetLeftCurbLinePoint(Float64 station, IDirection* direction,IPoint3d** point)
{
   GetCurbLinePoint(station,direction,qcbLeft,point);
}

void CBridgeAgentImp::GetRightCurbLinePoint(Float64 station, IDirection* direction,IPoint2d** point)
{
   GetCurbLinePoint(station,direction,qcbRight,point);
}

void CBridgeAgentImp::GetRightCurbLinePoint(Float64 station, IDirection* direction,IPoint3d** point)
{
   GetCurbLinePoint(station,direction,qcbRight,point);
}

Float64 CBridgeAgentImp::GetPierStation(PierIndexType pierIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IBridgeGeometry> geometry;
   m_Bridge->get_BridgeGeometry(&geometry);

   CComPtr<IPierLine> pier;
   PierIDType pierID = ::GetPierLineID(pierIdx);
   geometry->FindPierLine( pierID ,&pier);

   CComPtr<IStation> station;
   pier->get_Station(&station);

   Float64 value;
   station->get_Value(&value);

   return value;
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

   // If at start or end of bridge, adjust for cantilevered girders
   if ( startSpanIdx == 0 )
   {
      CComPtr<IPoint2d> pntPier1, pntEnd1, pntBrg1, pntBrg2, pntEnd2, pntPier2;
      GetSegmentEndPoints(CSegmentKey(0,0,0),&pntPier1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntPier2);

      CComPtr<IPoint2d> p1 = pntEnd1;

      pntPier1.Release();
      pntEnd1.Release();
      pntBrg1.Release();
      pntBrg2.Release();
      pntEnd2.Release();
      pntPier2.Release();

      GirderIndexType nGirders = GetGirderCountBySpan(startSpanIdx);
      GetSegmentEndPoints(CSegmentKey(0,nGirders-1,0),&pntPier1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntPier2);

      CComPtr<IPoint2d> p2 = pntEnd1;

      CComPtr<ILine2d> line;
      line.CoCreateInstance(CLSID_Line2d);
      HRESULT hr = line->ThroughPoints(p1,p2);

      CComPtr<IPoint2d> pntIntersect;
      if ( SUCCEEDED(hr) )
      {
         alignment->IntersectEx(line,p1,VARIANT_TRUE,VARIANT_TRUE,&pntIntersect);
      }
      else
      {
         // p1 and p2 are at the same point... can't create a line
         alignment->ProjectPoint(p1,&pntIntersect);
      }

      CComPtr<IStation> objStation;
      Float64 offset;
      alignment->Offset(pntIntersect,&objStation,&offset);

      Float64 station;
      objStation->get_Value(&station);
      if ( station < startStation )
      {
         startStation = station;
      }
   }


   SpanIndexType nSpans = GetSpanCount();
   if ( endSpanIdx == nSpans-1 )
   {
      GroupIndexType grpIdx = GetGirderGroupIndex(endSpanIdx);
      SegmentIndexType segIdx = GetSegmentCount(grpIdx,0)-1;
      CComPtr<IPoint2d> pntPier1, pntEnd1, pntBrg1, pntBrg2, pntEnd2, pntPier2;
      GetSegmentEndPoints(CSegmentKey(grpIdx,0,segIdx),&pntPier1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntPier2);

      CComPtr<IPoint2d> p1 = pntEnd2;

      pntPier1.Release();
      pntEnd1.Release();
      pntBrg1.Release();
      pntBrg2.Release();
      pntEnd2.Release();
      pntPier2.Release();

      GirderIndexType nGirders = GetGirderCountBySpan(endSpanIdx);
      GetSegmentEndPoints(CSegmentKey(grpIdx,nGirders-1,segIdx),&pntPier1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntPier2);
      
      CComPtr<IPoint2d> p2 = pntEnd2;

      CComPtr<ILine2d> line;
      line.CoCreateInstance(CLSID_Line2d);
      HRESULT hr = line->ThroughPoints(p1,p2);

      CComPtr<IPoint2d> pntIntersect;
      if ( SUCCEEDED(hr) )
      {
         alignment->IntersectEx(line,p1,VARIANT_TRUE,VARIANT_TRUE,&pntIntersect);
      }
      else
      {
         // p1 and p2 are at the same point... can't create a line
         alignment->ProjectPoint(p1,&pntIntersect);
      }

      CComPtr<IStation> objStation;
      Float64 offset;
      alignment->Offset(pntIntersect,&objStation,&offset);

      Float64 station;
      objStation->get_Value(&station);

      if ( endStation < station )
      {
         endStation = station;
      }
   }

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
      if (start_normal_station_right < station && station < end_normal_station_right)
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

   station = endStation;
   for ( pntIdx = 0; pntIdx < nPoints; pntIdx++ )
   {
      if (start_normal_station_left < station && station < end_normal_station_left)
      {
         CComPtr<IDirection> objDirection;
         alignment->Normal(CComVariant(station), &objDirection);

         CComPtr<IPoint2d> point;
         HRESULT hr = m_BridgeGeometryTool->DeckEdgePoint(m_Bridge,station,objDirection,qcbLeft,&point);
         ATLASSERT( SUCCEEDED(hr) );

         thePoints->Add(point);
      }

      station -= stationInc;
   }

   thePoints->Add(objStartPointLeft);

   (*points) = thePoints;
   (*points)->AddRef();
}

Float64 CBridgeAgentImp::GetAheadBearingStation(PierIndexType pierIdx,const CGirderKey& girderKey)
{
   // returns the station, where a line projected from the intersection of the
   // centerline of bearing and the centerline of girder, intersects the alignment
   // at a right angle
   VALIDATE( BRIDGE );

   // Get the segment that intersects this pier
   CComPtr<ISuperstructureMemberSegment> segment;
   GetSegmentAtPier(pierIdx,girderKey,&segment);

   // Get the girder line for that segment
   CComPtr<IGirderLine> girderLine;
   segment->get_GirderLine(&girderLine);

   // Get the pier line
   CComPtr<IPierLine> pierLine;
   GetPierLine(pierIdx,&pierLine);

   // Get intersection of alignment and pier
   CComPtr<IPoint2d> pntPierAlignment;
   pierLine->get_AlignmentPoint(&pntPierAlignment);

   // Get the intersection of the pier line and the girder line
   CComPtr<ILine2d> clPier;
   pierLine->get_Centerline(&clPier);

   CComPtr<IPath> gdrPath;
   girderLine->get_Path(&gdrPath);

   CComPtr<IPoint2d> pntPierGirder;
   gdrPath->Intersect(clPier,pntPierAlignment,&pntPierGirder);

   // Get the offset from the CL pier to the point of bearing, measured along the CL girder
   CComPtr<IDirection> girderDirection;
   girderLine->get_Direction(&girderDirection);

   Float64 brgOffset;
   pierLine->GetBearingOffset(pfAhead,girderDirection,&brgOffset);

   // Locate the intersection of the CL girder and the point of bearing
   CComPtr<ILocate2> locate;
   m_CogoEngine->get_Locate(&locate);
   CComPtr<IPoint2d> pntBrgGirder;
   locate->ByDistDir(pntPierGirder,brgOffset,CComVariant(girderDirection),0.0,&pntBrgGirder);

   // Get station and offset for this point
   CComPtr<IStation> objStation;
   Float64 station, offset;
   CComPtr<IAlignment> alignment;
   GetAlignment(&alignment);
   alignment->Offset(pntBrgGirder,&objStation,&offset);
   objStation->get_Value(&station);

   return station;
}

Float64 CBridgeAgentImp::GetBackBearingStation(PierIndexType pierIdx,const CGirderKey& girderKey)
{
   // returns the station, where a line projected from the intersection of the
   // centerline of bearing and the centerline of girder, intersects the alignment
   // at a right angle
   VALIDATE( BRIDGE );

   // Get the segment that intersects this pier
   CComPtr<ISuperstructureMemberSegment> segment;
   GetSegmentAtPier(pierIdx,girderKey,&segment);

   // Get the girder line for that segment
   CComPtr<IGirderLine> girderLine;
   segment->get_GirderLine(&girderLine);

   // Get the pier line
   CComPtr<IPierLine> pierLine;
   GetPierLine(pierIdx,&pierLine);

   // Get intersection of alignment and pier
   CComPtr<IPoint2d> pntPierAlignment;
   pierLine->get_AlignmentPoint(&pntPierAlignment);

   // Get the intersection of the pier line and the girder line
   CComPtr<ILine2d> clPier;
   pierLine->get_Centerline(&clPier);

   CComPtr<IPath> gdrPath;
   girderLine->get_Path(&gdrPath);

   CComPtr<IPoint2d> pntPierGirder;
   gdrPath->Intersect(clPier,pntPierAlignment,&pntPierGirder);

   // Get the offset from the CL pier to the point of bearing, measured along the CL girder
   CComPtr<IDirection> girderDirection;
   girderLine->get_Direction(&girderDirection);

   Float64 brgOffset;
   pierLine->GetBearingOffset(pfBack,girderDirection,&brgOffset);

   // Locate the intersection of the CL girder and the point of bearing
   CComPtr<ILocate2> locate;
   m_CogoEngine->get_Locate(&locate);
   CComPtr<IPoint2d> pntBrgGirder;
   locate->ByDistDir(pntPierGirder,-brgOffset,CComVariant(girderDirection),0.0,&pntBrgGirder);

   // Get station and offset for this point
   CComPtr<IStation> objStation;
   Float64 station, offset;
   CComPtr<IAlignment> alignment;
   GetAlignment(&alignment);
   alignment->Offset(pntBrgGirder,&objStation,&offset);
   objStation->get_Value(&station);

   return station;
}

void CBridgeAgentImp::GetPierDirection(PierIndexType pierIdx,IDirection** ppDirection)
{
   VALIDATE( BRIDGE );

   CComPtr<IBridgeGeometry> bridgeGeometry;
   m_Bridge->get_BridgeGeometry(&bridgeGeometry);

   PierIDType pierLineID = ::GetPierLineID(pierIdx);

   CComPtr<IPierLine> line;
   bridgeGeometry->FindPierLine(pierLineID,&line);

   line->get_Direction(ppDirection);
}

void CBridgeAgentImp::GetPierSkew(PierIndexType pierIdx,IAngle** ppAngle)
{
   VALIDATE( BRIDGE );

   CComPtr<IBridgeGeometry> bridgeGeometry;
   m_Bridge->get_BridgeGeometry(&bridgeGeometry);

   PierIDType pierLineID = ::GetPierLineID(pierIdx);

   CComPtr<IPierLine> line;
   bridgeGeometry->FindPierLine(pierLineID,&line);

   line->get_Skew(ppAngle);
}

void CBridgeAgentImp::GetPierPoints(PierIndexType pierIdx,IPoint2d** left,IPoint2d** alignment,IPoint2d** bridge,IPoint2d** right)
{
   VALIDATE( BRIDGE );

   CComPtr<IBridgeGeometry> geometry;
   m_Bridge->get_BridgeGeometry(&geometry);

   CComPtr<IPierLine> pierLine;
   geometry->FindPierLine(::GetPierLineID(pierIdx),&pierLine);

   pierLine->get_AlignmentPoint(alignment);
   pierLine->get_BridgePoint(bridge);
   pierLine->get_LeftPoint(left);
   pierLine->get_RightPoint(right);
}

void CBridgeAgentImp::IsContinuousAtPier(PierIndexType pierIdx,bool* pbLeft,bool* pbRight)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData2* pPierData = pBridgeDesc->GetPier(pierIdx);
   ATLASSERT( pPierData );

   *pbLeft  = pPierData->IsContinuous();
   *pbRight = *pbLeft;

   if ( pierIdx == 0 && *pbRight == false )
   {
      // first pier and not already continuous... see if
      // there is a cantilever as that will make it continuous
      bool bCantileverStart, bCantileverEnd;
      ModelCantilevers(CSegmentKey(0,0,0),&bCantileverStart,&bCantileverEnd);
      *pbRight = bCantileverStart;
      *pbLeft  = bCantileverStart;
   }

   if ( pierIdx == pBridgeDesc->GetPierCount()-1 && *pbLeft == false )
   {
      // last pier and not already continuous... see if
      // there is a cantilever as that will make it continuous
      GroupIndexType grpIdx = pBridgeDesc->GetGirderGroupCount()-1;
      SegmentIndexType segIdx = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirder(0)->GetSegmentCount()-1;
      bool bCantileverStart, bCantileverEnd;
      ModelCantilevers(CSegmentKey(grpIdx,0,segIdx),&bCantileverStart,&bCantileverEnd);
      *pbRight = bCantileverEnd;
      *pbLeft  = bCantileverEnd;
   }
}

void CBridgeAgentImp::IsIntegralAtPier(PierIndexType pierIdx,bool* pbLeft,bool* pbRight)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData2* pPierData = pBridgeDesc->GetPier(pierIdx);
   ATLASSERT( pPierData );

   pPierData->IsIntegral(pbLeft,pbRight);
}

void CBridgeAgentImp::GetContinuityEventIndex(PierIndexType pierIdx,EventIndexType* pBack,EventIndexType* pAhead)
{
   VALIDATE( BRIDGE );

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();
   EventIndexType castDeckEventIdx = pTimelineMgr->GetCastDeckEventIndex();

   const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);
   if ( pPier->IsBoundaryPier() )
   {
      pgsTypes::BoundaryConditionType cType = pPier->GetBoundaryConditionType();

      if ( cType == pgsTypes::bctContinuousBeforeDeck || cType == pgsTypes::bctIntegralBeforeDeck )
      {
         *pBack  = castDeckEventIdx;
         *pAhead = castDeckEventIdx;
      }
      else if ( cType == pgsTypes::bctIntegralBeforeDeckHingeBack )
      {
         *pBack  = castDeckEventIdx+1;
         *pAhead = castDeckEventIdx;
      }
      else if ( cType == pgsTypes::bctIntegralBeforeDeckHingeAhead )
      {
         *pBack  = castDeckEventIdx;
         *pAhead = castDeckEventIdx+1;
      }
      else // ContinuousAfterDeck, IntegralAfterDeck, 
           // IntegralAfterDeckHingeAhead, IntegralAfterDeckHingeBack
           // Hinged, Roller
      {
         *pBack  = castDeckEventIdx+1;
         *pAhead = castDeckEventIdx+1;
      }
   }
   else
   {
      pgsTypes::PierSegmentConnectionType cType = pPier->GetSegmentConnectionType();
      if ( cType == pgsTypes::psctContinousClosureJoint || cType == pgsTypes::psctIntegralClosureJoint )
      {
         const CClosureJointData* pClosure = pPier->GetClosureJoint(0);
         EventIndexType castClosureEventIdx = pTimelineMgr->GetCastClosureJointEventIndex(pClosure->GetID());
         *pBack  = castClosureEventIdx+1;
         *pAhead = castClosureEventIdx+1;
      }
      else if ( cType == pgsTypes::psctContinuousSegment || cType == pgsTypes::psctIntegralSegment )
      {
         // which segment is continuous over this pier?
         GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
         for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
         {
            const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
            const CSplicedGirderData* pGirder = pGroup->GetGirder(0);
            SegmentIndexType nSegments = pGirder->GetSegmentCount();
            for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
            {
               const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
               const CSpanData2* pStartSpan = pSegment->GetSpan(pgsTypes::metStart);
               const CSpanData2* pEndSpan   = pSegment->GetSpan(pgsTypes::metEnd);
               PierIndexType leftPierIdx = pStartSpan->GetNextPier()->GetIndex();
               PierIndexType rightPierIdx = pEndSpan->GetPrevPier()->GetIndex();
               if ( leftPierIdx <= pierIdx && pierIdx <= rightPierIdx )
               {
                  // we found the segment
                  EventIndexType erectSegmentIdx = pTimelineMgr->GetSegmentErectionEventIndex(pSegment->GetID());
                  *pBack  = erectSegmentIdx;
                  *pAhead = erectSegmentIdx;
                  return; // we found the answer
               }
            }
         }
         ATLASSERT(false); // if we got this far, the segment was not found and it should have been
      }
      else
      {
         ATLASSERT(false); // is there a new connection type?
      }
   }
}

bool CBridgeAgentImp::GetPierLocation(PierIndexType pierIdx,const CSegmentKey& segmentKey,Float64* pXs)
{
   CComPtr<IPoint2d> pntSupport1,pntEnd1,pntBrg1,pntBrg2,pntEnd2,pntSupport2;
   GetSegmentEndPoints(segmentKey,&pntSupport1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntSupport2);

   CComPtr<IPoint2d> pntPier;
   if ( !GetSegmentPierIntersection(segmentKey,pierIdx,&pntPier) )
   {
      return false;
   }

   CComPtr<IMeasure2> measure;
   m_CogoEngine->get_Measure(&measure);

   // Measure offset from pntEnd1 so that offset is in the segment coordinate system
   CComPtr<IDirection> direction; // direction from end to pier point
   Float64 offset; // this is always a positive value because it is a distance
   measure->Inverse(pntEnd1,pntPier,&offset,&direction);
   if ( IsZero(offset) )
   {
      *pXs = 0;
   }
   else
   {
      // need to determine if the intersection with the pier line is before or after the
      // start end of the segment. we do this by comparing directions
      CComPtr<IDirection> dirSegment;
      measure->Direction(pntEnd1,pntEnd2,&dirSegment); // direction of segment

      Float64 d1,d2;
      direction->get_Value(&d1);
      dirSegment->get_Value(&d2);

      if ( IsEqual(d1,d2) )
      {
         // same direction so offset is fine
         *pXs = offset;
      }
      else
      {
         // opposite directions so pier point is before the start of the segment
         // so offset is negative in segment coordinates
         *pXs = -offset;
      }
   }
 
   return true;
}

bool CBridgeAgentImp::GetPierLocation(const CGirderKey& girderKey,PierIndexType pierIdx,Float64* pXgp)
{
   ASSERT_GIRDER_KEY(girderKey);
   PierIndexType startPierIdx, endPierIdx;
   GetGirderGroupPiers(girderKey.groupIndex,&startPierIdx,&endPierIdx);
   if ( pierIdx < startPierIdx || endPierIdx < pierIdx )
   {
      ATLASSERT(false); // why aren't you asking for a pier that is in the group?
      return false;
   }

   SegmentIndexType nSegments = GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      Float64 Xs;
      bool bResult = GetPierLocation(pierIdx,segmentKey,&Xs);
      if ( bResult == true )
      {
         Float64 Xsp = ConvertSegmentCoordinateToSegmentPathCoordinate(segmentKey,Xs);
         Float64 Xgp = ConvertSegmentPathCoordinateToGirderPathCoordinate(segmentKey,Xsp);
         *pXgp = Xgp;
         return true;
      }
   }

   return false;
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
      {
         return false;
      }

      CComPtr<IDirection> normal;
      alignment->Normal(CComVariant(station),&normal);

      CComPtr<IAngle> angle;
      brg->AngleBetween(normal,&angle);

      Float64 value;
      angle->get_Value(&value);

      while ( PI_OVER_2 < value )
      {
         value -= M_PI;
      }

      *pSkew = value;
   }
   else
   {
      // defined by skew angle
      CComPtr<IAngle> angle;
      angle.CoCreateInstance(CLSID_Angle);
      HRESULT hr = angle->FromString(CComBSTR(strOrientation));
      if ( FAILED(hr) )
      {
         return false;
      }

      Float64 value;
      angle->get_Value(&value);
      *pSkew = value;
   }

   return true;
}

pgsTypes::PierModelType CBridgeAgentImp::GetPierModelType(PierIndexType pierIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPierData2* pPier = pIBridgeDesc->GetPier(pierIdx);
   return pPier->GetPierModelType();
}

ColumnIndexType CBridgeAgentImp::GetColumnCount(PierIndexType pierIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPierData2* pPier = pIBridgeDesc->GetPier(pierIdx);
   if ( pPier->GetPierModelType() == pgsTypes::pmtIdealized )
   {
      return 0;
   }
   else
   {
      return pPier->GetColumnCount();
   }
}

void CBridgeAgentImp::GetColumnProperties(PierIndexType pierIdx,ColumnIndexType colIdx,bool bSkewAdjust,Float64* pHeight,Float64* pA,Float64* pI)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPierData2* pPier = pIBridgeDesc->GetPier(pierIdx);
   if ( pPier->GetPierModelType() == pgsTypes::pmtIdealized )
   {
      *pHeight = 0;
      *pA = 0;
      *pI = 0;
   }
   else
   {
#pragma Reminder("UPDATE: this information should be coming from the generic bridge model")
      const CColumnData& columnData = pPier->GetColumnData(colIdx);
      Float64 A, I;
      Float64 D1, D2;
      columnData.GetColumnDimensions(&D1,&D2);
      if ( columnData.GetColumnShape() == CColumnData::cstCircle )
      {
         A = M_PI*D1*D1/4;
         I = M_PI*D1*D1*D1*D1/64;
      }
      else
      {
         A = D1*D2;

         // in centroidal coodinates
         Float64 Ix = D1*D2*D2*D2/12;
         Float64 Iy = D2*D1*D1*D1/12;

         // use Mohr's circle to get ix about an axis that is normal to the alignment
         if ( bSkewAdjust )
         {
            CComPtr<IMohrCircle> mc;
            mc.CoCreateInstance(CLSID_MohrCircle);
            mc->put_Sii(Ix);
            mc->put_Sjj(Iy);
            mc->put_Sij(0);

            CComPtr<IAngle> objSkew;
            GetPierSkew(pierIdx,&objSkew);
            Float64 skew;
            objSkew->get_Value(&skew);

            Float64 ix;
            mc->ComputeSxx(-skew,&ix);
            ATLASSERT(0 < ix);
            I = ix;
         }
         else
         {
            I = Ix;
         }
      }

      *pA = A;
      *pI = I;

      if ( columnData.GetColumnHeightMeasurementType() == CColumnData::chtHeight )
      {
         *pHeight = columnData.GetColumnHeight();
      }
      else
      {
         Float64 top_roadway_elevation = GetElevation(pPier->GetStation(),0.0);
         Float64 superstructure_depth = GetSuperstructureDepth(pierIdx);
        
#pragma Reminder("WORKING HERE - need a better way to determine cross beam depth")
         // Need to use the WBFLGenericBridge::BridgePier object for pier geometry
         Float64 h1, h2, lt, eso;
         pPier->GetXBeamDimensions(pgsTypes::pstLeft,&h1,&h2,&lt,&eso);
         Float64 cross_beam_depth = h1 + h2;

#pragma Reminder("UPDATE: need to account for bearing recess, bearing depth, and grout pad depth")
         Float64 bottom_column_elevation = columnData.GetColumnHeight();
         Float64 column_height = top_roadway_elevation - superstructure_depth - cross_beam_depth - bottom_column_elevation;
         *pHeight = column_height;
      }
   }
}

pgsTypes::BoundaryConditionType CBridgeAgentImp::GetBoundaryConditionType(PierIndexType pierIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPierData2* pPier = pIBridgeDesc->GetPier(pierIdx);
   ATLASSERT(pPier->IsBoundaryPier());
   return pPier->GetBoundaryConditionType();
}

pgsTypes::PierSegmentConnectionType CBridgeAgentImp::GetPierSegmentConnectionType(PierIndexType pierIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPierData2* pPier = pIBridgeDesc->GetPier(pierIdx);
   ATLASSERT(pPier->IsInteriorPier());
   return pPier->GetSegmentConnectionType();
}

bool CBridgeAgentImp::IsAbutment(PierIndexType pierIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPierData2* pPier = pIBridgeDesc->GetPier(pierIdx);
   return pPier->IsAbutment();
}

bool CBridgeAgentImp::IsPier(PierIndexType pierIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPierData2* pPier = pIBridgeDesc->GetPier(pierIdx);
   return pPier->IsPier();
}

bool CBridgeAgentImp::HasCantilever(PierIndexType pierIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPierData2* pPier = pIBridgeDesc->GetPier(pierIdx);
   return pPier->HasCantilever();
}

bool CBridgeAgentImp::IsInteriorPier(PierIndexType pierIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPierData2* pPier = pIBridgeDesc->GetPier(pierIdx);
   return pPier->IsInteriorPier();
}

bool CBridgeAgentImp::IsBoundaryPier(PierIndexType pierIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPierData2* pPier = pIBridgeDesc->GetPier(pierIdx);
   return pPier->IsBoundaryPier();
}

bool CBridgeAgentImp::ProcessNegativeMoments(SpanIndexType spanIdx)
{
   PierIndexType startPierIdx = PierIndexType(spanIdx == ALL_SPANS ? 0 : spanIdx);
   PierIndexType endPierIdx   = PierIndexType(spanIdx == ALL_SPANS ? GetSpanCount() : spanIdx+1);

   // Start by checking if there are cantilevers at the start/end of the span. If so,
   // there will be negative moments
   if ( startPierIdx == 0 )
   {
      CSegmentKey segmentKey(0,0,0);
      bool bStartCantilever, bEndCantilever;
      ModelCantilevers(segmentKey,&bStartCantilever,&bEndCantilever);
      if ( bStartCantilever )
      {
         // there will be negative moments in this span
         return true;
      }
   }

   PierIndexType nPiers = GetPierCount();
   if ( endPierIdx == nPiers-1 )
   {
      GroupIndexType nGroups = GetGirderGroupCount();
      SegmentIndexType nSegments = GetSegmentCount(nGroups-1,0);
      CSegmentKey segmentKey(nGroups-1,0,nSegments-1);
      bool bStartCantilever, bEndCantilever;
      ModelCantilevers(segmentKey,&bStartCantilever,&bEndCantilever);
      if ( bEndCantilever )
      {
         // there will be negative moments in this span
         return true;
      }
   }

   // don't need to process negative moments if this is a simple span design
   // or if there isn't any continuity
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   if ( analysisType == pgsTypes::Simple )
   {
      return false;
   }

   for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
   {
      bool bContinuousLeft,bContinuousRight;
      IsContinuousAtPier(pierIdx,&bContinuousLeft,&bContinuousRight);

      bool bIntegralLeft,bIntegralRight;
      IsIntegralAtPier(pierIdx,&bIntegralLeft,&bIntegralRight);

      if ( bContinuousLeft || bContinuousRight || bIntegralLeft || bIntegralRight )
      {
         return true;
      }
   }

   return false;
}

void CBridgeAgentImp::GetTemporarySupportLocation(SupportIndexType tsIdx,GirderIndexType gdrIdx,SpanIndexType* pSpanIdx,Float64* pXspan)
{
   // validate the bridge geometry because we are going to use it
   VALIDATE( BRIDGE );

   CComPtr<IBridgeGeometry> geometry;
   m_Bridge->get_BridgeGeometry(&geometry);


   // Figure out which span the temporary support is in
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTemporarySupportData* pTS = pBridgeDesc->GetTemporarySupport(tsIdx);
   const CSpanData2* pSpan = pTS->GetSpan();
   *pSpanIdx = pSpan->GetIndex();

   // get the spliced girder object
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   GroupIndexType grpIdx = pGroup->GetIndex();
   const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);

   // get some geometric information about the temporary support
   Float64 tsStation = pTS->GetStation();

   CComPtr<IPierLine> tsLine;
   geometry->FindPierLine(::GetTempSupportLineID(tsIdx),&tsLine);

   CComPtr<ILine2d> clTS;
   tsLine->get_Centerline(&clTS);

   CComPtr<IPoint2d> pntNearest;
   tsLine->get_AlignmentPoint(&pntNearest);

   // find where the temporary support intersects the girder line.... sum up the distances
   // to find the location from the start of the span
   Float64 location = 0;
   std::vector<std::pair<SegmentIndexType,Float64>> seg_lengths(GetSegmentLengths(CSpanKey(*pSpanIdx,gdrIdx)));
   std::vector<std::pair<SegmentIndexType,Float64>>::iterator iter(seg_lengths.begin());
   std::vector<std::pair<SegmentIndexType,Float64>>::iterator iterEnd(seg_lengths.end());
   for ( ; iter != iterEnd; iter++ )
   {
      SegmentIndexType segIdx = (*iter).first;
      Float64 seg_length = (*iter).second; // length of segment in this span

      // get the segment
      const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
      CSegmentKey segmentKey = pSegment->GetSegmentKey();

      // get the station boundary of the segment
      Float64 startStation, endStation;
      pSegment->GetStations(&startStation,&endStation);

      if ( IsEqual(tsStation,startStation) )
      {
         // Temp Support is at the start of this segment
         break;
      }

      if ( IsEqual(tsStation,endStation) )
      {
         // Temp Support is at the end of this segment. Add this segment's length
         location += seg_length;
         break;
      }

      if ( startStation < tsStation && tsStation < endStation )
      {
         // Temp support is in the middle of this segment. Determine how far it is from
         // the start of this segment to the temporary support. Add this distance to the running total

         // get the girder line
         LineIDType segID = ::GetGirderSegmentLineID(segmentKey);
         CComPtr<IGirderLine> girderLine;
         geometry->FindGirderLine(segID,&girderLine);

         // get the path (the actual geometry of the girder line)
         CComPtr<IPath> gdrPath;
         girderLine->get_Path(&gdrPath);

         // intersect the girder path and the CL of the temporar support
         CComPtr<IPoint2d> pntIntersect;
         gdrPath->Intersect(clTS,pntNearest,&pntIntersect);

         // get the distance and offset of intersection point from the start of the segment
         // dist is the distance from the start of this segment... if the segment starts in the
         // previous span this distance needs to be reduced by the length of this segment in the
         // previous span
         Float64 dist,offset;
         gdrPath->Offset(pntIntersect,&dist,&offset);
         ATLASSERT(IsZero(offset));

         if ( segIdx == 0 )
         {
            Float64 brgOffset = GetSegmentStartBearingOffset(CSegmentKey(grpIdx,gdrIdx,0));
            dist -= brgOffset; // want to measure location from start of span which begins at CL Bearing
         }

         Float64 length_in_prev_span = 0;
         if ( pSegment->GetSpanIndex(pgsTypes::metStart) < *pSpanIdx )
         {
            // part of this segment is in the previous span...
            Float64 true_segment_length = this->GetSegmentLayoutLength(CSegmentKey(grpIdx,gdrIdx,segIdx));
            length_in_prev_span = true_segment_length - seg_length;
         }
         dist -= length_in_prev_span;

         ATLASSERT( 0 < dist && dist < seg_length );

         // add in this distance
         location += dist;
         break;
      }

      // the temporary support doesn't intersect this segment... just add up the segment length
      // and go to the next segment
      location += seg_length;
   }

   *pXspan = location;

   ATLASSERT(*pXspan <= GetSpanLength(*pSpanIdx,gdrIdx));
}

bool CBridgeAgentImp::GetTemporarySupportLocation(SupportIndexType tsIdx,const CSegmentKey& segmentKey,Float64* pXs)
{
   CComPtr<IPoint2d> pntSupport1,pntEnd1,pntBrg1,pntBrg2,pntEnd2,pntSupport2;
   GetSegmentEndPoints(segmentKey,&pntSupport1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntSupport2);

   CComPtr<IPoint2d> pntTS;
   if ( !GetSegmentTempSupportIntersection(segmentKey,tsIdx,&pntTS) )
   {
      return false;
   }

   // Measure offset from pntSupport1 so that offset is in the segment coordinate system
   pntTS->DistanceEx(pntSupport1,pXs);
 
   return true;
}

Float64 CBridgeAgentImp::GetTemporarySupportLocation(SupportIndexType tsIdx,GirderIndexType gdrIdx)
{
   Float64 Xgp = 0;
   GroupIndexType nGroups = GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType nGirders = GetGirderCount(grpIdx);
      GirderIndexType thisGdrIdx = Min(gdrIdx,nGirders-1);
      SegmentIndexType nSegments = GetSegmentCount(grpIdx,thisGdrIdx);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(grpIdx,thisGdrIdx,segIdx);
         Float64 Xs;
         bool bResult = GetTemporarySupportLocation(tsIdx,segmentKey,&Xs);
         if ( bResult == true )
         {
            Xgp += Xs;
            return Xgp;
         }
         else
         {
            Xgp += GetSegmentLayoutLength(segmentKey);
         }
      }
   }

   ATLASSERT(false); // should never get here
   return -99999;

}

pgsTypes::TemporarySupportType CBridgeAgentImp::GetTemporarySupportType(SupportIndexType tsIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTemporarySupportData* pTS = pBridgeDesc->GetTemporarySupport(tsIdx);

   return pTS->GetSupportType();
}

pgsTypes::TempSupportSegmentConnectionType CBridgeAgentImp::GetSegmentConnectionTypeAtTemporarySupport(SupportIndexType tsIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CTemporarySupportData* pTS = pBridgeDesc->GetTemporarySupport(tsIdx);
   return pTS->GetConnectionType();
}

void CBridgeAgentImp::GetSegmentsAtTemporarySupport(GirderIndexType gdrIdx,SupportIndexType tsIdx,CSegmentKey* pLeftSegmentKey,CSegmentKey* pRightSegmentKey)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   pBridgeDesc->GetSegmentsAtTemporarySupport(tsIdx,pLeftSegmentKey,pRightSegmentKey);
   pLeftSegmentKey->girderIndex = gdrIdx;
   pRightSegmentKey->girderIndex = gdrIdx;
}

void CBridgeAgentImp::GetTemporarySupportDirection(SupportIndexType tsIdx,IDirection** ppDirection)
{
   VALIDATE( BRIDGE );

   CComPtr<IBridgeGeometry> bridgeGeometry;
   m_Bridge->get_BridgeGeometry(&bridgeGeometry);

   PierIDType pierLineID = ::GetTempSupportLineID(tsIdx);

   CComPtr<IPierLine> line;
   bridgeGeometry->FindPierLine(pierLineID,&line);

   line->get_Direction(ppDirection);
}

/////////////////////////////////////////////////////////////////////////
// IMaterials
Float64 CBridgeAgentImp::GetSegmentFc28(const CSegmentKey& segmentKey)
{
   Float64 time_at_casting = m_ConcreteManager.GetSegmentCastingTime(segmentKey);
   Float64 age = 28.0; // days
   Float64 t = time_at_casting + age;
   return m_ConcreteManager.GetSegmentFc(segmentKey,t);
}

Float64 CBridgeAgentImp::GetClosureJointFc28(const CSegmentKey& closureKey)
{
   Float64 time_at_casting = m_ConcreteManager.GetClosureJointCastingTime(closureKey);
   Float64 age = 28.0; // days
   Float64 t = time_at_casting + age;
   return m_ConcreteManager.GetClosureJointFc(closureKey,t);
}

Float64 CBridgeAgentImp::GetDeckFc28()
{
   Float64 time_at_casting = m_ConcreteManager.GetDeckCastingTime();
   Float64 age = 28.0; // days
   Float64 t = time_at_casting + age;
   return m_ConcreteManager.GetDeckFc(t);
}

Float64 CBridgeAgentImp::GetRailingSystemFc28(pgsTypes::TrafficBarrierOrientation orientation)
{
   Float64 time_at_casting = m_ConcreteManager.GetRailingSystemCastingTime(orientation);
   Float64 age = 28.0; // days
   Float64 t = time_at_casting + age;
   return m_ConcreteManager.GetRailingSystemFc(orientation,t);
}

Float64 CBridgeAgentImp::GetPierFc28(IndexType pierIdx)
{
   return m_ConcreteManager.GetPierConcrete(pierIdx)->GetFc();
}

Float64 CBridgeAgentImp::GetSegmentEc28(const CSegmentKey& segmentKey)
{
   Float64 time_at_casting = m_ConcreteManager.GetSegmentCastingTime(segmentKey);
   Float64 age = 28.0; // days
   Float64 t = time_at_casting + age;
   return m_ConcreteManager.GetSegmentEc(segmentKey,t);
}

Float64 CBridgeAgentImp::GetClosureJointEc28(const CSegmentKey& closureKey)
{
   Float64 time_at_casting = m_ConcreteManager.GetClosureJointCastingTime(closureKey);
   Float64 age = 28.0; // days
   Float64 t = time_at_casting + age;
   return m_ConcreteManager.GetClosureJointEc(closureKey,t);
}

Float64 CBridgeAgentImp::GetDeckEc28()
{
   Float64 time_at_casting = m_ConcreteManager.GetDeckCastingTime();
   Float64 age = 28.0; // days
   Float64 t = time_at_casting + age;
   return m_ConcreteManager.GetDeckEc(t);
}

Float64 CBridgeAgentImp::GetRailingSystemEc28(pgsTypes::TrafficBarrierOrientation orientation)
{
   Float64 time_at_casting = m_ConcreteManager.GetRailingSystemCastingTime(orientation);
   Float64 age = 28.0; // days
   Float64 t = time_at_casting + age;
   return m_ConcreteManager.GetRailingSystemEc(orientation,t);
}

Float64 CBridgeAgentImp::GetPierEc28(IndexType pierIdx)
{
   return m_ConcreteManager.GetPierConcrete(pierIdx)->GetE();
}

Float64 CBridgeAgentImp::GetSegmentWeightDensity(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx)
{
   Float64 age = GetSegmentConcreteAge(segmentKey,intervalIdx,pgsTypes::Middle);
   if ( age < 0 )
   {
      return 0;
   }

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   return pSegment->Material.Concrete.WeightDensity;
}

Float64 CBridgeAgentImp::GetClosureJointWeightDensity(const CClosureKey& closureKey,IntervalIndexType intervalIdx)
{
   Float64 age = GetClosureJointConcreteAge(closureKey,intervalIdx,pgsTypes::Middle);
   if ( age < 0 )
   {
      return 0;
   }

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CClosureJointData* pClosure = pIBridgeDesc->GetClosureJointData(closureKey);
   return pClosure->GetConcrete().WeightDensity;
}

Float64 CBridgeAgentImp::GetDeckWeightDensity(IntervalIndexType intervalIdx)
{
   Float64 age = GetDeckConcreteAge(intervalIdx,pgsTypes::Middle);
   if ( age < 0 )
   {
      return 0;
   }


   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CDeckDescription2* pDeck = pIBridgeDesc->GetDeckDescription();
   return pDeck->Concrete.WeightDensity;
}

Float64 CBridgeAgentImp::GetRailingSystemWeightDensity(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx)
{
   Float64 age = GetRailingSystemAge(orientation,intervalIdx,pgsTypes::Middle);
   if ( age < 0 )
   {
      return 0;
   }

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CRailingSystem* pRailingSystem;
   if ( orientation == pgsTypes::tboLeft )
   {
      pRailingSystem = pIBridgeDesc->GetBridgeDescription()->GetLeftRailingSystem();
   }
   else
   {
      pRailingSystem = pIBridgeDesc->GetBridgeDescription()->GetRightRailingSystem();
   }

   return pRailingSystem->Concrete.WeightDensity;
}

Float64 CBridgeAgentImp::GetSegmentConcreteAge(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   IntervalIndexType stressStrandIntervalIdx = m_IntervalManager.GetStressStrandInterval(segmentKey);
   if ( intervalIdx < stressStrandIntervalIdx )
   {
      return 0; // segment hasn't been cast yet
   }

   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   Float64 casting_day = m_IntervalManager.GetTime(stressStrandIntervalIdx,pgsTypes::Start);

   // age is the difference in time between point in time for the interval in question and the casting day
   Float64 age = time - casting_day;

   return age;
}

Float64 CBridgeAgentImp::GetClosureJointConcreteAge(const CSegmentKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   IntervalIndexType castClosureIntervalIdx = m_IntervalManager.GetCastClosureInterval(closureKey);
   if ( intervalIdx < castClosureIntervalIdx )
   {
      return 0; // closure joint hasn't been cast yet
   }

   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   Float64 casting_day = m_IntervalManager.GetTime(castClosureIntervalIdx,pgsTypes::Start);

   // age is the difference in time between point in time for the interval in question and the casting day
   Float64 age = time - casting_day;

   return age;
}

Float64 CBridgeAgentImp::GetDeckConcreteAge(IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   IntervalIndexType castDeckIntervalIdx = m_IntervalManager.GetCastDeckInterval();
   if ( intervalIdx < castDeckIntervalIdx )
   {
      return 0; // deck hasn't been cast yet
   }

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   // want age at specified time
   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   Float64 casting_day = m_IntervalManager.GetTime(castDeckIntervalIdx,pgsTypes::Start);

   // age is the difference in time between point in time for the interval in question and the casting day
   Float64 age = time - casting_day;

   return age;
}

Float64 CBridgeAgentImp::GetRailingSystemAge(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   IntervalIndexType railingSystemIntervalIdx = m_IntervalManager.GetInstallRailingSystemInterval();
   if ( intervalIdx < railingSystemIntervalIdx )
   {
      return 0; // railing system hasn't been cast yet
   }

   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   Float64 casting_day = m_IntervalManager.GetTime(railingSystemIntervalIdx,pgsTypes::Start);

   // age is the difference in time between point in time for the interval in question and the casting day
   Float64 age = time - casting_day;

   return age;
}

Float64 CBridgeAgentImp::GetSegmentFc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   return m_ConcreteManager.GetSegmentFc(segmentKey,time);
}

Float64 CBridgeAgentImp::GetClosureJointFc(const CClosureKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   return m_ConcreteManager.GetClosureJointFc(closureKey,time);
}

Float64 CBridgeAgentImp::GetDeckFc(IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   return m_ConcreteManager.GetDeckFc(time);
}

Float64 CBridgeAgentImp::GetRailingSystemFc(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   return m_ConcreteManager.GetRailingSystemFc(orientation,time);
}

Float64 CBridgeAgentImp::GetSegmentDesignFc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx)
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetLossMethod() == pgsTypes::TIME_STEP )
   {
      if ( pSpecEntry->GetLimitStateConcreteStrength() == pgsTypes::lscStrengthAtTimeOfLoading )
      {
         return GetSegmentFc(segmentKey,intervalIdx);
      }
      else
      {
         // basing allowable stresses and nominal strength on specified values
         GET_IFACE(IBridgeDescription,pIBridgeDesc);
         const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
         IntervalIndexType releaseIntervalIdx = GetPrestressReleaseInterval(segmentKey);
         IntervalIndexType liftingIntervalIdx = GetLiftSegmentInterval(segmentKey);
         IntervalIndexType storageIntervalIdx = GetStorageInterval(segmentKey);
         if ( intervalIdx == releaseIntervalIdx || 
              intervalIdx == liftingIntervalIdx || 
              intervalIdx == storageIntervalIdx )
         {
            // interval is associated with initial strength
            if ( pSegment->Material.Concrete.bHasInitial )
            {
               // an initial strength was specified.. return it
               return pSegment->Material.Concrete.Fci;
            }
            else
            {
               // initial strength was not specified, just compute it
               return GetSegmentFc(segmentKey,releaseIntervalIdx);
            }
         }
         else
         {
            // return the specified 28 day strength
            return pSegment->Material.Concrete.Fc;
         }
      }
   }
   else
   {
      return GetSegmentFc(segmentKey,intervalIdx);
   }
}

Float64 CBridgeAgentImp::GetClosureJointDesignFc(const CSegmentKey& closureKey,IntervalIndexType intervalIdx)
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetLossMethod() == pgsTypes::TIME_STEP )
   {
      if ( pSpecEntry->GetLimitStateConcreteStrength() == pgsTypes::lscStrengthAtTimeOfLoading )
      {
         return GetClosureJointFc(closureKey,intervalIdx);
      }
      else
      {
         // basing allowable stresses and nominatl strength on specified values
         GET_IFACE(IBridgeDescription,pIBridgeDesc);
         const CClosureJointData* pClosureJoint = pIBridgeDesc->GetClosureJointData(closureKey);
         IntervalIndexType castIntervalIdx      = GetCastClosureJointInterval(closureKey);
         IntervalIndexType compositeIntervalIdx = GetCompositeClosureJointInterval(closureKey);
         if ( intervalIdx <= castIntervalIdx )
         {
            return 0;
         }
         else if ( castIntervalIdx < intervalIdx && intervalIdx < compositeIntervalIdx )
         {
            // interval is associated with initial strength
            if ( pClosureJoint->GetConcrete().bHasInitial )
            {
               // an initial strength was specified.. return it
               return pClosureJoint->GetConcrete().Fci;
            }
            else
            {
               // initial strength was not specified, just compute it
               return GetClosureJointFc(closureKey,compositeIntervalIdx);
            }
         }
         else
         {
            // return the specified 28 day strength
            return pClosureJoint->GetConcrete().Fc;
         }
      }
   }
   else
   {
      return GetClosureJointFc(closureKey,intervalIdx);
   }
}

Float64 CBridgeAgentImp::GetDeckDesignFc(IntervalIndexType intervalIdx)
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetLossMethod() == pgsTypes::TIME_STEP )
   {
      if ( pSpecEntry->GetLimitStateConcreteStrength() == pgsTypes::lscStrengthAtTimeOfLoading )
      {
         return GetDeckFc(intervalIdx);
      }
      else
      {
         // basing allowable stresses and nominatl strength on specified values
         GET_IFACE(IBridgeDescription,pIBridgeDesc);
         const CDeckDescription2* pDeck = pIBridgeDesc->GetDeckDescription();
         IntervalIndexType castIntervalIdx      = GetCastDeckInterval();
         IntervalIndexType compositeIntervalIdx = GetCompositeDeckInterval();
         if ( intervalIdx <= castIntervalIdx )
         {
            return 0;
         }
         else if ( castIntervalIdx < intervalIdx && intervalIdx < compositeIntervalIdx )
         {
            // interval is associated with initial strength
            if ( pDeck->Concrete.bHasInitial )
            {
               // an initial strength was specified.. return it
               return pDeck->Concrete.Fci;
            }
            else
            {
               // initial strength was not specified, just compute it
               return GetDeckFc(compositeIntervalIdx);
            }
         }
         else
         {
            // return the specified 28 day strength
            return pDeck->Concrete.Fc;
         }
      }
   }
   else
   {
      return GetDeckFc(intervalIdx);
   }
}

Float64 CBridgeAgentImp::GetSegmentEc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   return m_ConcreteManager.GetSegmentEc(segmentKey,time);
}

Float64 CBridgeAgentImp::GetSegmentEc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,Float64 trialFc,bool* pbChanged)
{
#if defined _DEBUG
   // This function is for computing the modulus of elasticity during design for a trial concrete strength.
   // Design is not available for time-step analysis so this should not get called
   GET_IFACE(ILossParameters,pLossParams);
   ATLASSERT( pLossParams->GetLossMethod() != pgsTypes::TIME_STEP);
#endif

   GET_IFACE(ISegmentData,pSegmentData);

   const CGirderMaterial* pMaterial = pSegmentData->GetSegmentMaterial(segmentKey);

   // storage interval is the last interval for release strength
   IntervalIndexType storageIntervalIdx = GetStorageInterval(segmentKey);

   bool bInitial = (intervalIdx <= storageIntervalIdx ? true : false);
   Float64 E;
   if ( (bInitial && pMaterial->Concrete.bUserEci) || (!bInitial && pMaterial->Concrete.bUserEc) )
   {
      E = (bInitial ? pMaterial->Concrete.Eci : pMaterial->Concrete.Ec);
      *pbChanged = false;
   }
   else
   {
      E = lrfdConcreteUtil::ModE( trialFc, pMaterial->Concrete.StrengthDensity, false /*ignore LRFD range checks*/ );

      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         E *= (pMaterial->Concrete.EcK1*pMaterial->Concrete.EcK2);
      }

      Float64 Eorig = (bInitial ? pMaterial->Concrete.Eci : pMaterial->Concrete.Ec);
      *pbChanged = IsEqual(E,Eorig) ? false : true;
   }

   return E;
}

Float64 CBridgeAgentImp::GetClosureJointEc(const CClosureKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   return m_ConcreteManager.GetClosureJointEc(closureKey,time);
}

Float64 CBridgeAgentImp::GetClosureJointEc(const CClosureKey& closureKey,IntervalIndexType intervalIdx,Float64 trialFc,bool* pbChanged)
{
#if defined _DEBUG
   // This function is for computing the modulus of elasticity during design for a trial concrete strength.
   // Design is not available for time-step analysis so this should not get called
   GET_IFACE(ILossParameters,pLossParams);
   ATLASSERT( pLossParams->GetLossMethod() != pgsTypes::TIME_STEP);
#endif


   GET_IFACE(IBridgeDescription,pIBridgeDesc);

   const CSplicedGirderData* pGirder = pIBridgeDesc->GetGirder(closureKey);
   const CClosureJointData* pClosure = pGirder->GetClosureJoint(closureKey.segmentIndex);
   const CConcreteMaterial& concrete(pClosure->GetConcrete());

   IntervalIndexType compositeClosureIntervalIdx = GetCompositeClosureJointInterval(closureKey);

   bool bInitial = (intervalIdx <= compositeClosureIntervalIdx ? true : false);
   Float64 E;
   if ( (bInitial && concrete.bUserEci) || (!bInitial && concrete.bUserEc) )
   {
      E = (bInitial ? concrete.Eci : concrete.Ec);
      *pbChanged = false;
   }
   else
   {
      E = lrfdConcreteUtil::ModE( trialFc, concrete.StrengthDensity, false /*ignore LRFD range checks*/ );

      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         E *= (concrete.EcK1*concrete.EcK2);
      }

      Float64 Eorig = (bInitial ? concrete.Eci : concrete.Ec);
      *pbChanged = IsEqual(E,Eorig) ? false : true;
   }

   return E;
}

Float64 CBridgeAgentImp::GetDeckEc(IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   return m_ConcreteManager.GetDeckEc(time);
}

Float64 CBridgeAgentImp::GetRailingSystemEc(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   return m_ConcreteManager.GetRailingSystemEc(orientation,time);
}

Float64 CBridgeAgentImp::GetSegmentFlexureFr(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   return m_ConcreteManager.GetSegmentFlexureFr(segmentKey,time);
}

Float64 CBridgeAgentImp::GetSegmentShearFr(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   return m_ConcreteManager.GetSegmentShearFr(segmentKey,time);
}

Float64 CBridgeAgentImp::GetClosureJointFlexureFr(const CSegmentKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   return m_ConcreteManager.GetClosureJointFlexureFr(closureKey,time);
}

Float64 CBridgeAgentImp::GetClosureJointShearFr(const CClosureKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   return m_ConcreteManager.GetClosureJointShearFr(closureKey,time);
}

Float64 CBridgeAgentImp::GetDeckFlexureFr(IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   return m_ConcreteManager.GetDeckFlexureFr(time);
}

Float64 CBridgeAgentImp::GetDeckShearFr(IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   return m_ConcreteManager.GetDeckShearFr(time);
}

Float64 CBridgeAgentImp::GetSegmentAgingCoefficient(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx)
{
   // assume loading occurs at the middle of the interval
   Float64 middle = m_IntervalManager.GetTime(intervalIdx,pgsTypes::Middle);
   return m_ConcreteManager.GetSegmentAgingCoefficient(segmentKey,middle);
}

Float64 CBridgeAgentImp::GetClosureJointAgingCoefficient(const CClosureKey& closureKey,IntervalIndexType intervalIdx)
{
   Float64 middle = m_IntervalManager.GetTime(intervalIdx,pgsTypes::Middle);
   return m_ConcreteManager.GetClosureJointAgingCoefficient(closureKey,middle);
}

Float64 CBridgeAgentImp::GetDeckAgingCoefficient(IntervalIndexType intervalIdx)
{
   Float64 middle = m_IntervalManager.GetTime(intervalIdx,pgsTypes::Middle);
   return m_ConcreteManager.GetDeckAgingCoefficient(middle);
}

Float64 CBridgeAgentImp::GetRailingSystemAgingCoefficient(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx)
{
   Float64 middle = m_IntervalManager.GetTime(intervalIdx,pgsTypes::Middle);
   return m_ConcreteManager.GetRailingSystemAgingCoefficient(orientation,middle);
}

Float64 CBridgeAgentImp::GetSegmentAgeAdjustedEc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx)
{
   Float64 Ec = GetSegmentEc(segmentKey,intervalIdx);

   GET_IFACE(ILossParameters,pLossParams);
   if ( pLossParams->IgnoreCreepEffects() )
   {
      return Ec;
   }

   Float64 Y  = GetSegmentCreepCoefficient(segmentKey,intervalIdx,pgsTypes::Middle,intervalIdx,pgsTypes::End);
   Float64 X  = GetSegmentAgingCoefficient(segmentKey,intervalIdx);
   Float64 Ea = Ec/(1+X*Y);
   return Ea;
}

Float64 CBridgeAgentImp::GetClosureJointAgeAdjustedEc(const CSegmentKey& closureKey,IntervalIndexType intervalIdx)
{
   Float64 Ec = GetClosureJointEc(closureKey,intervalIdx);

   GET_IFACE(ILossParameters,pLossParams);
   if ( pLossParams->IgnoreCreepEffects() )
   {
      return Ec;
   }

   Float64 Y  = GetClosureJointCreepCoefficient(closureKey,intervalIdx,pgsTypes::Middle,intervalIdx,pgsTypes::End);
   Float64 X  = GetClosureJointAgingCoefficient(closureKey,intervalIdx);
   Float64 Ea = Ec/(1+X*Y);
   return Ea;
}

Float64 CBridgeAgentImp::GetDeckAgeAdjustedEc(IntervalIndexType intervalIdx)
{
   IntervalIndexType compositeDeckIntervalIdx = GetCompositeDeckInterval();
   if ( intervalIdx < compositeDeckIntervalIdx )
   {
      return 0; // deck isn't composite yet
   }

   Float64 Ec = GetDeckEc(intervalIdx);

   GET_IFACE(ILossParameters,pLossParams);
   if ( pLossParams->IgnoreCreepEffects() )
   {
      return Ec;
   }

   Float64 Y  = GetDeckCreepCoefficient(intervalIdx,pgsTypes::Middle,intervalIdx,pgsTypes::End);
   Float64 X  = GetDeckAgingCoefficient(intervalIdx);
   Float64 Ea = Ec/(1+X*Y);
   return Ea;
}

Float64 CBridgeAgentImp::GetRailingSystemAgeAdjustedEc(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx)
{
   Float64 Ec = GetRailingSystemEc(orientation,intervalIdx);

   GET_IFACE(ILossParameters,pLossParams);
   if ( pLossParams->IgnoreCreepEffects() )
   {
      return Ec;
   }

   Float64 Y  = GetRailingSystemCreepCoefficient(orientation,intervalIdx,pgsTypes::Middle,intervalIdx,pgsTypes::End);
   Float64 X  = GetRailingSystemAgingCoefficient(orientation,intervalIdx);
   Float64 Ea = Ec/(1+X*Y);
   return Ea;
}

Float64 CBridgeAgentImp::GetSegmentFreeShrinkageStrain(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   return m_ConcreteManager.GetSegmentFreeShrinkageStrain(segmentKey,time);
}

Float64 CBridgeAgentImp::GetClosureJointFreeShrinkageStrain(const CClosureKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   return m_ConcreteManager.GetClosureJointFreeShrinkageStrain(closureKey,time);
}

Float64 CBridgeAgentImp::GetDeckFreeShrinkageStrain(IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   return m_ConcreteManager.GetDeckFreeShrinkageStrain(time);
}

Float64 CBridgeAgentImp::GetRailingSystemFreeShrinakgeStrain(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   return m_ConcreteManager.GetRailingSystemFreeShrinkageStrain(orientation,time);
}

Float64 CBridgeAgentImp::GetSegmentFreeShrinkageStrain(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx)
{
   return GetSegmentFreeShrinkageStrain(segmentKey,intervalIdx,pgsTypes::End) - GetSegmentFreeShrinkageStrain(segmentKey,intervalIdx,pgsTypes::Start);
}

Float64 CBridgeAgentImp::GetClosureJointFreeShrinkageStrain(const CSegmentKey& closureKey,IntervalIndexType intervalIdx)
{
   return GetClosureJointFreeShrinkageStrain(closureKey,intervalIdx,pgsTypes::End) - GetClosureJointFreeShrinkageStrain(closureKey,intervalIdx,pgsTypes::Start);
}

Float64 CBridgeAgentImp::GetDeckFreeShrinkageStrain(IntervalIndexType intervalIdx)
{
   return GetDeckFreeShrinkageStrain(intervalIdx,pgsTypes::End) - GetDeckFreeShrinkageStrain(intervalIdx,pgsTypes::Start);
}

Float64 CBridgeAgentImp::GetRailingSystemFreeShrinakgeStrain(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx)
{
   return GetRailingSystemFreeShrinakgeStrain(orientation,intervalIdx,pgsTypes::End) - GetRailingSystemFreeShrinakgeStrain(orientation,intervalIdx,pgsTypes::Start);
}

Float64 CBridgeAgentImp::GetSegmentCreepCoefficient(const CSegmentKey& segmentKey,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   Float64 loading_time = m_IntervalManager.GetTime(loadingIntervalIdx,loadingTimeType);
   Float64 time         = m_IntervalManager.GetTime(intervalIdx,timeType);
   return m_ConcreteManager.GetSegmentCreepCoefficient(segmentKey,time,loading_time);
}

Float64 CBridgeAgentImp::GetClosureJointCreepCoefficient(const CClosureKey& closureKey,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   Float64 loading_time = m_IntervalManager.GetTime(loadingIntervalIdx,loadingTimeType);
   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   return m_ConcreteManager.GetClosureJointCreepCoefficient(closureKey,time,loading_time);
}

Float64 CBridgeAgentImp::GetDeckCreepCoefficient(IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType)
{
   Float64 loading_time = m_IntervalManager.GetTime(loadingIntervalIdx,loadingTimeType);
   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   return m_ConcreteManager.GetDeckCreepCoefficient(time,loading_time);
}

Float64 CBridgeAgentImp::GetRailingSystemCreepCoefficient(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType)
{
   Float64 loading_time = m_IntervalManager.GetTime(loadingIntervalIdx,loadingTimeType);
   Float64 time = m_IntervalManager.GetTime(intervalIdx,timeType);
   return m_ConcreteManager.GetRailingSystemCreepCoefficient(orientation,time,loading_time);
}

pgsTypes::ConcreteType CBridgeAgentImp::GetSegmentConcreteType(const CSegmentKey& segmentKey)
{
   return m_ConcreteManager.GetSegmentConcreteType(segmentKey);
}

bool CBridgeAgentImp::DoesSegmentConcreteHaveAggSplittingStrength(const CSegmentKey& segmentKey)
{
   return m_ConcreteManager.DoesSegmentConcreteHaveAggSplittingStrength(segmentKey);
}

Float64 CBridgeAgentImp::GetSegmentConcreteAggSplittingStrength(const CSegmentKey& segmentKey)
{
   return m_ConcreteManager.GetSegmentConcreteAggSplittingStrength(segmentKey);
}

Float64 CBridgeAgentImp::GetSegmentStrengthDensity(const CSegmentKey& segmentKey)
{
   return m_ConcreteManager.GetSegmentStrengthDensity(segmentKey);
}

Float64 CBridgeAgentImp::GetSegmentMaxAggrSize(const CSegmentKey& segmentKey)
{
   return m_ConcreteManager.GetSegmentMaxAggrSize(segmentKey);
}

Float64 CBridgeAgentImp::GetSegmentEccK1(const CSegmentKey& segmentKey)
{
   return m_ConcreteManager.GetSegmentEccK1(segmentKey);
}

Float64 CBridgeAgentImp::GetSegmentEccK2(const CSegmentKey& segmentKey)
{
   return m_ConcreteManager.GetSegmentEccK2(segmentKey);
}

Float64 CBridgeAgentImp::GetSegmentCreepK1(const CSegmentKey& segmentKey)
{
   return m_ConcreteManager.GetSegmentCreepK1(segmentKey);
}

Float64 CBridgeAgentImp::GetSegmentCreepK2(const CSegmentKey& segmentKey)
{
   return m_ConcreteManager.GetSegmentCreepK2(segmentKey);
}

Float64 CBridgeAgentImp::GetSegmentShrinkageK1(const CSegmentKey& segmentKey)
{
   return m_ConcreteManager.GetSegmentShrinkageK1(segmentKey);
}

Float64 CBridgeAgentImp::GetSegmentShrinkageK2(const CSegmentKey& segmentKey)
{
   return m_ConcreteManager.GetSegmentShrinkageK2(segmentKey);
}

const matConcreteBase* CBridgeAgentImp::GetSegmentConcrete(const CSegmentKey& segmentKey)
{
   return m_ConcreteManager.GetSegmentConcrete(segmentKey);
}

pgsTypes::ConcreteType CBridgeAgentImp::GetClosureJointConcreteType(const CClosureKey& closureKey)
{
   return m_ConcreteManager.GetClosureJointConcreteType(closureKey);
}

bool CBridgeAgentImp::DoesClosureJointConcreteHaveAggSplittingStrength(const CClosureKey& closureKey)
{
   return m_ConcreteManager.DoesClosureJointConcreteHaveAggSplittingStrength(closureKey);
}

Float64 CBridgeAgentImp::GetClosureJointConcreteAggSplittingStrength(const CClosureKey& closureKey)
{
   return m_ConcreteManager.GetClosureJointConcreteAggSplittingStrength(closureKey);
}

Float64 CBridgeAgentImp::GetClosureJointStrengthDensity(const CClosureKey& closureKey)
{
   return m_ConcreteManager.GetClosureJointStrengthDensity(closureKey);
}

Float64 CBridgeAgentImp::GetClosureJointMaxAggrSize(const CClosureKey& closureKey)
{
   return m_ConcreteManager.GetClosureJointMaxAggrSize(closureKey);
}

Float64 CBridgeAgentImp::GetClosureJointEccK1(const CClosureKey& closureKey)
{
   return m_ConcreteManager.GetClosureJointEccK1(closureKey);
}

Float64 CBridgeAgentImp::GetClosureJointEccK2(const CClosureKey& closureKey)
{
   return m_ConcreteManager.GetClosureJointEccK2(closureKey);
}

Float64 CBridgeAgentImp::GetClosureJointCreepK1(const CClosureKey& closureKey)
{
   return m_ConcreteManager.GetClosureJointCreepK1(closureKey);
}

Float64 CBridgeAgentImp::GetClosureJointCreepK2(const CClosureKey& closureKey)
{
   return m_ConcreteManager.GetClosureJointCreepK2(closureKey);
}

Float64 CBridgeAgentImp::GetClosureJointShrinkageK1(const CClosureKey& closureKey)
{
   return m_ConcreteManager.GetClosureJointShrinkageK1(closureKey);
}

Float64 CBridgeAgentImp::GetClosureJointShrinkageK2(const CClosureKey& closureKey)
{
   return m_ConcreteManager.GetClosureJointShrinkageK2(closureKey);
}

const matConcreteBase* CBridgeAgentImp::GetClosureJointConcrete(const CClosureKey& closureKey)
{
   return m_ConcreteManager.GetClosureJointConcrete(closureKey);
}

pgsTypes::ConcreteType CBridgeAgentImp::GetDeckConcreteType()
{
   return m_ConcreteManager.GetDeckConcreteType();
}

bool CBridgeAgentImp::DoesDeckConcreteHaveAggSplittingStrength()
{
   return m_ConcreteManager.DoesDeckConcreteHaveAggSplittingStrength();
}

Float64 CBridgeAgentImp::GetDeckConcreteAggSplittingStrength()
{
   return m_ConcreteManager.GetDeckConcreteAggSplittingStrength();
}

Float64 CBridgeAgentImp::GetDeckMaxAggrSize()
{
   return m_ConcreteManager.GetDeckMaxAggrSize();
}

Float64 CBridgeAgentImp::GetDeckEccK1()
{
   return m_ConcreteManager.GetDeckEccK1();
}

Float64 CBridgeAgentImp::GetDeckEccK2()
{
   return m_ConcreteManager.GetDeckEccK2();
}

Float64 CBridgeAgentImp::GetDeckCreepK1()
{
   return m_ConcreteManager.GetDeckCreepK1();
}

Float64 CBridgeAgentImp::GetDeckCreepK2()
{
   return m_ConcreteManager.GetDeckCreepK2();
}

Float64 CBridgeAgentImp::GetDeckShrinkageK1()
{
   return m_ConcreteManager.GetDeckShrinkageK1();
}

Float64 CBridgeAgentImp::GetDeckShrinkageK2()
{
   return m_ConcreteManager.GetDeckShrinkageK2();
}

const matConcreteBase* CBridgeAgentImp::GetDeckConcrete()
{
   return m_ConcreteManager.GetDeckConcrete();
}

const matPsStrand* CBridgeAgentImp::GetStrandMaterial(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType)
{
   GET_IFACE(ISegmentData,pSegmentData);
   return pSegmentData->GetStrandMaterial(segmentKey,strandType);
}

Float64 CBridgeAgentImp::GetStrandRelaxation(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,Float64 fpi,pgsTypes::StrandType strandType)
{
   if ( GetStrandCount(segmentKey,strandType) == 0 )
   {
      return 0.0; // no strands, no relaxation
   }

   const matPsStrand* pStrand = GetStrandMaterial(segmentKey,strandType);

   IntervalIndexType stressStrandIntervalIdx = m_IntervalManager.GetStressStrandInterval(segmentKey);
   if ( intervalIdx < stressStrandIntervalIdx )
   {
      // strand not stressed yet, so no relaxation
      return 0;
   }

   Float64 tStress = m_IntervalManager.GetTime(stressStrandIntervalIdx,pgsTypes::Start);
   Float64 tStart = m_IntervalManager.GetTime(intervalIdx,pgsTypes::Start);
   Float64 tEnd   = m_IntervalManager.GetTime(intervalIdx,pgsTypes::End);

   Float64 fr = GetRelaxation(fpi,pStrand,tStart,tEnd,tStress);
   return fr;
}

const matPsStrand* CBridgeAgentImp::GetTendonMaterial(const CGirderKey& girderKey)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPTData* pPTData = pIBridgeDesc->GetPostTensioning(girderKey);
   return pPTData->pStrand;
}

Float64 CBridgeAgentImp::GetTendonRelaxation(const CGirderKey& girderKey,DuctIndexType ductIdx,IntervalIndexType intervalIdx,Float64 fpi)
{
   if ( GetTendonStrandCount(girderKey,ductIdx) == 0 )
   {
      return 0.0; // no strand, no relaxation
   }

   const matPsStrand* pStrand = GetTendonMaterial(girderKey);

   IntervalIndexType stressTendonIntervalIdx = m_IntervalManager.GetStressTendonInterval(girderKey,ductIdx);
   if ( intervalIdx < stressTendonIntervalIdx )
   {
      // tendon not stressed yet, so no relaxation
      return 0;
   }

   Float64 tStress = m_IntervalManager.GetTime(stressTendonIntervalIdx,pgsTypes::Start);
   Float64 tStart  = m_IntervalManager.GetTime(intervalIdx,pgsTypes::Start);
   Float64 tEnd    = m_IntervalManager.GetTime(intervalIdx,pgsTypes::End);

   Float64 fr = GetRelaxation(fpi,pStrand,tStart,tEnd,tStress);
   return fr;
}

void CBridgeAgentImp::GetSegmentLongitudinalRebarProperties(const CSegmentKey& segmentKey,Float64* pE,Float64 *pFy,Float64* pFu)
{
   GET_IFACE(ILongitudinalRebar,pLongRebar);
   const CLongitudinalRebarData* pLRD = pLongRebar->GetSegmentLongitudinalRebarData(segmentKey);

   const matRebar* pRebar = lrfdRebarPool::GetInstance()->GetRebar(pLRD->BarType,pLRD->BarGrade,matRebar::bs3);
   *pE  = pRebar->GetE();
   *pFy = pRebar->GetYieldStrength();
   *pFu = pRebar->GetUltimateStrength();
}

void CBridgeAgentImp::GetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey,matRebar::Type* pType,matRebar::Grade* pGrade)
{
   GET_IFACE(ILongitudinalRebar,pLongRebar);
   const CLongitudinalRebarData* pLRD = pLongRebar->GetSegmentLongitudinalRebarData(segmentKey);
   (*pType) = pLRD->BarType;
   (*pGrade) = pLRD->BarGrade;
}

std::_tstring CBridgeAgentImp::GetSegmentLongitudinalRebarName(const CSegmentKey& segmentKey)
{
   GET_IFACE(ILongitudinalRebar,pLongRebar);
   return pLongRebar->GetSegmentLongitudinalRebarMaterial(segmentKey);
}

void CBridgeAgentImp::GetClosureJointLongitudinalRebarProperties(const CClosureKey& closureKey,Float64* pE,Float64 *pFy,Float64* pFu)
{
   GET_IFACE(ILongitudinalRebar,pLongRebar);
   const CLongitudinalRebarData* pLRD = pLongRebar->GetClosureJointLongitudinalRebarData(closureKey);

   const matRebar* pRebar = lrfdRebarPool::GetInstance()->GetRebar(pLRD->BarType,pLRD->BarGrade,matRebar::bs3);
   *pE  = pRebar->GetE();
   *pFy = pRebar->GetYieldStrength();
   *pFu = pRebar->GetUltimateStrength();
}

void CBridgeAgentImp::GetClosureJointLongitudinalRebarMaterial(const CClosureKey& closureKey,matRebar::Type* pType,matRebar::Grade* pGrade)
{
   GET_IFACE(ILongitudinalRebar,pLongRebar);
   const CLongitudinalRebarData* pLRD = pLongRebar->GetClosureJointLongitudinalRebarData(closureKey);
   (*pType) = pLRD->BarType;
   (*pGrade) = pLRD->BarGrade;
}

std::_tstring CBridgeAgentImp::GetClosureJointLongitudinalRebarName(const CClosureKey& closureKey)
{
   GET_IFACE(ILongitudinalRebar,pLongRebar);
   return pLongRebar->GetClosureJointLongitudinalRebarMaterial(closureKey);
}

void CBridgeAgentImp::GetSegmentTransverseRebarProperties(const CSegmentKey& segmentKey,Float64* pE,Float64 *pFy,Float64* pFu)
{
	GET_IFACE(IShear,pShear);
	const CShearData2* pShearData = pShear->GetSegmentShearData(segmentKey);
   const matRebar* pRebar = lrfdRebarPool::GetInstance()->GetRebar(pShearData->ShearBarType,pShearData->ShearBarGrade,matRebar::bs3);
   *pE  = pRebar->GetE();
   *pFy = pRebar->GetYieldStrength();
   *pFu = pRebar->GetUltimateStrength();

}

void CBridgeAgentImp::GetSegmentTransverseRebarMaterial(const CSegmentKey& segmentKey,matRebar::Type* pType,matRebar::Grade* pGrade)
{
	GET_IFACE(IShear,pShear);
	const CShearData2* pShearData = pShear->GetSegmentShearData(segmentKey);
   (*pType)  = pShearData->ShearBarType;
   (*pGrade) = pShearData->ShearBarGrade;
}

std::_tstring CBridgeAgentImp::GetSegmentTransverseRebarName(const CSegmentKey& segmentKey)
{
   GET_IFACE(IShear,pShear);
   return pShear->GetSegmentStirrupMaterial(segmentKey);
}

void CBridgeAgentImp::GetClosureJointTransverseRebarProperties(const CClosureKey& closureKey,Float64* pE,Float64 *pFy,Float64* pFu)
{
	GET_IFACE(IShear,pShear);
	const CShearData2* pShearData = pShear->GetClosureJointShearData(closureKey);
   const matRebar* pRebar = lrfdRebarPool::GetInstance()->GetRebar(pShearData->ShearBarType,pShearData->ShearBarGrade,matRebar::bs3);
   *pE  = pRebar->GetE();
   *pFy = pRebar->GetYieldStrength();
   *pFu = pRebar->GetUltimateStrength();

}

void CBridgeAgentImp::GetClosureJointTransverseRebarMaterial(const CClosureKey& closureKey,matRebar::Type* pType,matRebar::Grade* pGrade)
{
	GET_IFACE(IShear,pShear);
	const CShearData2* pShearData = pShear->GetClosureJointShearData(closureKey);
   (*pType)  = pShearData->ShearBarType;
   (*pGrade) = pShearData->ShearBarGrade;
}

std::_tstring CBridgeAgentImp::GetClosureJointTransverseRebarName(const CClosureKey& closureKey)
{
   GET_IFACE(IShear,pShear);
   return pShear->GetClosureJointStirrupMaterial(closureKey);
}

void CBridgeAgentImp::GetDeckRebarProperties(Float64* pE,Float64 *pFy,Float64* pFu)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CDeckDescription2* pDeck = pIBridgeDesc->GetDeckDescription();
   const matRebar* pRebar = lrfdRebarPool::GetInstance()->GetRebar(pDeck->DeckRebarData.TopRebarType,pDeck->DeckRebarData.TopRebarGrade,matRebar::bs3);
   *pE  = pRebar->GetE();
   *pFy = pRebar->GetYieldStrength();
   *pFu = pRebar->GetUltimateStrength();
}

std::_tstring CBridgeAgentImp::GetDeckRebarName()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CDeckDescription2* pDeck = pIBridgeDesc->GetDeckDescription();
   return lrfdRebarPool::GetMaterialName(pDeck->DeckRebarData.TopRebarType,pDeck->DeckRebarData.TopRebarGrade);
}

void CBridgeAgentImp::GetDeckRebarMaterial(matRebar::Type* pType,matRebar::Grade* pGrade)
{
   // top and bottom mat use the same material
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CDeckDescription2* pDeck = pIBridgeDesc->GetDeckDescription();
   (*pType)  = pDeck->DeckRebarData.TopRebarType;
   (*pGrade) = pDeck->DeckRebarData.TopRebarGrade;
}

Float64 CBridgeAgentImp::GetNWCDensityLimit()
{
   return m_ConcreteManager.GetNWCDensityLimit();
}

Float64 CBridgeAgentImp::GetLWCDensityLimit()
{
   return m_ConcreteManager.GetLWCDensityLimit();
}

Float64 CBridgeAgentImp::GetFlexureModRupture(Float64 fc,pgsTypes::ConcreteType type)
{
   return m_ConcreteManager.GetFlexureModRupture(fc,type);
}

Float64 CBridgeAgentImp::GetFlexureFrCoefficient(const CSegmentKey& segmentKey)
{
   return m_ConcreteManager.GetFlexureFrCoefficient(segmentKey);
}

Float64 CBridgeAgentImp::GetShearModRupture(Float64 fc,pgsTypes::ConcreteType type)
{
   return m_ConcreteManager.GetShearModRupture(fc,type);
}

Float64 CBridgeAgentImp::GetShearFrCoefficient(const CSegmentKey& segmentKey)
{
   return m_ConcreteManager.GetShearFrCoefficient(segmentKey);
}

Float64 CBridgeAgentImp::GetEconc(Float64 fc,Float64 density,Float64 K1,Float64 K2)
{
   return m_ConcreteManager.GetEconc(fc,density,K1,K2);
}

/////////////////////////////////////////////////////////////////////////
// ILongRebarGeometry
void CBridgeAgentImp::GetRebars(const pgsPointOfInterest& poi,IRebarSection** rebarSection)
{
   Float64 Xpoi = poi.GetDistFromStart();

   CComPtr<IPrecastGirder> girder;
   GetGirder(poi.GetSegmentKey(),&girder);

   CComPtr<IRebarLayout> rebar_layout;

   CClosureKey closureKey;
   if ( IsInClosureJoint(poi,&closureKey) )
   {
      girder->get_ClosureJointRebarLayout(&rebar_layout);
   }
   else
   {
      girder->get_RebarLayout(&rebar_layout);
   }

   IntervalIndexType nIntervals = GetIntervalCount();
   IntervalIndexType lastIntervalIdx = nIntervals-1;
   rebar_layout->CreateRebarSection(Xpoi,lastIntervalIdx,rebarSection);
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
   Float64 As_Top    = GetAsTopMat(poi,pgsTypes::drbAll,pgsTypes::drcAll);
   Float64 As_Bottom = GetAsBottomMat(poi,pgsTypes::drbAll,pgsTypes::drcAll);

   return As_Top + As_Bottom;
}

Float64 CBridgeAgentImp::GetDevLengthFactor(const pgsPointOfInterest& poi,IRebarSectionItem* rebarItem)
{
   Float64 fc;
   pgsTypes::ConcreteType type;
   bool isFct;
   Float64 fct;

   CClosureKey closureKey;
   Float64 bIsInClosureJoint = IsInClosureJoint(poi,&closureKey);
   if ( bIsInClosureJoint )
   {
      fc = GetClosureJointFc28(closureKey);
      type = GetClosureJointConcreteType(closureKey);
      isFct = DoesClosureJointConcreteHaveAggSplittingStrength(closureKey);
      fct = isFct ? GetClosureJointConcreteAggSplittingStrength(closureKey) : 0.0;
   }
   else
   {
      const CSegmentKey& segmentKey(poi.GetSegmentKey());
      fc = GetSegmentFc28(segmentKey);
      type = GetSegmentConcreteType(segmentKey);
      isFct = DoesSegmentConcreteHaveAggSplittingStrength(segmentKey);
      fct = isFct ? GetSegmentConcreteAggSplittingStrength(segmentKey) : 0.0;
   }

   return GetDevLengthFactor(poi, rebarItem, type, fc, isFct, fct);
}

Float64 CBridgeAgentImp::GetDevLengthFactor(const pgsPointOfInterest& poi,IRebarSectionItem* rebarItem, pgsTypes::ConcreteType type, Float64 fc, bool isFct, Float64 fct)
{
   Float64 fra = 1.0;

   CClosureKey closureKey;
   Float64 bIsInClosureJoint = IsInClosureJoint(poi,&closureKey);
   if ( bIsInClosureJoint )
   {
      // Assume that reinforcement in the closure joint is fully developed.
      // That is how the user input is specified.
      return fra;
   }


   CComPtr<IRebar> rebar;
   rebarItem->get_Rebar(&rebar);

   REBARDEVLENGTHDETAILS details = GetRebarDevelopmentLengthDetails(rebar, type, fc, isFct, fct);

   // Get distances from section cut to ends of bar
   Float64 start,end;
   rebarItem->get_LeftExtension(&start);
   rebarItem->get_RightExtension(&end);

   Float64 cut_length = Min(start, end);
   if (0.0 < cut_length)
   {
      Float64 fra = cut_length/details.ldb;
      fra = Min(fra, 1.0);

      return fra;
   }
   else
   {
      ATLASSERT(cut_length == 0.0); // sections shouldn't be cutting bars that don't exist
      return 0.0;
   }
}

Float64 CBridgeAgentImp::GetPPRTopHalf(const pgsPointOfInterest& poi)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Float64 As = GetAsDeckTopHalf(poi,false);
   As += GetAsGirderTopHalf(poi,false);

   Float64 Aps = GetApsTopHalf(poi,dlaNone);

   const matPsStrand* pstrand = GetStrandMaterial(segmentKey,pgsTypes::Permanent);
   ATLASSERT(pstrand!=0);
   Float64 fps = pstrand->GetYieldStrength();
   ATLASSERT(fps>0.0);

   Float64 E,fs,fu;
   CClosureKey closureKey;
   if ( IsInClosureJoint(poi,&closureKey) )
   {
      GetClosureJointLongitudinalRebarProperties(closureKey,&E,&fs,&fu);
   }
   else
   {
      GetSegmentLongitudinalRebarProperties(segmentKey,&E,&fs,&fu);
   }

   Float64 denom = Aps*fps + As*fs;
   Float64 ppr = IsZero(denom) ? 0.0 : (Aps*fps)/denom;

   return ppr;
}

Float64 CBridgeAgentImp::GetPPRTopHalf(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Float64 As = GetAsDeckTopHalf(poi,false);
   As += GetAsGirderTopHalf(poi,false);

   Float64 Aps = GetApsTopHalf(poi,config,dlaNone);

   const matPsStrand* pstrand = GetStrandMaterial(segmentKey,pgsTypes::Permanent);
   ATLASSERT(pstrand!=0);
   Float64 fps = pstrand->GetYieldStrength();
   ATLASSERT(fps>0.0);

   Float64 E,fs,fu;
   CClosureKey closureKey;
   if ( IsInClosureJoint(poi,&closureKey) )
   {
      GetClosureJointLongitudinalRebarProperties(closureKey,&E,&fs,&fu);
   }
   else
   {
      GetSegmentLongitudinalRebarProperties(segmentKey,&E,&fs,&fu);
   }

   Float64 denom = Aps*fps + As*fs;
   Float64 ppr = IsZero(denom) ? 0.0 : (Aps*fps)/denom;

   return ppr;
}

Float64 CBridgeAgentImp::GetPPRBottomHalf(const pgsPointOfInterest& poi)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Float64 As = GetAsBottomHalf(poi,false);

   Float64 Aps = GetApsBottomHalf(poi,dlaNone);

   const matPsStrand* pstrand = GetStrandMaterial(segmentKey,pgsTypes::Permanent);
   ATLASSERT(pstrand!=0);
   Float64 fps = pstrand->GetYieldStrength();
   ATLASSERT(fps>0.0);

   Float64 E,fs,fu;
   CClosureKey closureKey;
   if ( IsInClosureJoint(poi,&closureKey) )
   {
      GetClosureJointLongitudinalRebarProperties(closureKey,&E,&fs,&fu);
   }
   else
   {
      GetSegmentLongitudinalRebarProperties(segmentKey,&E,&fs,&fu);
   }

   Float64 denom = Aps*fps + As*fs;
   Float64 ppr = IsZero(denom) ? 0.0 : (Aps*fps)/denom;

   return ppr;
}

Float64 CBridgeAgentImp::GetPPRBottomHalf(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Float64 As = GetAsBottomHalf(poi,false);

   Float64 Aps = GetApsBottomHalf(poi,config,dlaNone);

   const matPsStrand* pstrand = GetStrandMaterial(segmentKey,pgsTypes::Permanent);
   ATLASSERT(pstrand!=0);
   Float64 fps = pstrand->GetYieldStrength();
   ATLASSERT(fps>0.0);

   Float64 E,fs,fu;
   CClosureKey closureKey;
   if ( IsInClosureJoint(poi,&closureKey) )
   {
      GetClosureJointLongitudinalRebarProperties(closureKey,&E,&fs,&fu);
   }
   else
   {
      GetSegmentLongitudinalRebarProperties(segmentKey,&E,&fs,&fu);
   }

   Float64 denom = Aps*fps + As*fs;
   Float64 ppr = IsZero(denom) ? 0.0 : (Aps*fps)/denom;

   return ppr;
}

REBARDEVLENGTHDETAILS CBridgeAgentImp::GetRebarDevelopmentLengthDetails(IRebar* rebar,pgsTypes::ConcreteType type, Float64 fc, bool isFct, Float64 Fct)
{
   USES_CONVERSION;
   CComBSTR name;
   rebar->get_Name(&name);

   matRebar::Size size = lrfdRebarPool::GetBarSize(OLE2CT(name));

   Float64 Ab, db, fy;
   rebar->get_NominalArea(&Ab);
   rebar->get_NominalDiameter(&db);
   rebar->get_YieldStrength(&fy);

   return lrfdRebar::GetRebarDevelopmentLengthDetails(size, Ab, db, fy, (matConcrete::Type)type, fc, isFct, Fct);
}

Float64 CBridgeAgentImp::GetCoverTopMat()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();
   const CDeckRebarData& rebarData = pDeck->DeckRebarData;

   return rebarData.TopCover;
}

Float64 CBridgeAgentImp::GetTopMatLocation(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory)
{
   return GetLocationDeckMats(poi,barType,barCategory,true,false);
}

Float64 CBridgeAgentImp::GetAsTopMat(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory)
{
   return GetAsDeckMats(poi,barType,barCategory,true,false);
}

Float64 CBridgeAgentImp::GetCoverBottomMat()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();
   const CDeckRebarData& rebarData = pDeck->DeckRebarData;

   return rebarData.BottomCover;
}

Float64 CBridgeAgentImp::GetBottomMatLocation(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory)
{
   return GetLocationDeckMats(poi,barType,barCategory,false,true);
}

Float64 CBridgeAgentImp::GetAsBottomMat(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory)
{
   return GetAsDeckMats(poi,barType,barCategory,false,true);
}

void CBridgeAgentImp::GetDeckReinforcing(const pgsPointOfInterest& poi,pgsTypes::DeckRebarMatType matType,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory,bool bAdjForDevLength,Float64* pAs,Float64* pYb)
{
   bool bTopMat    = (matType == pgsTypes::drmTop    ? true : false);
   bool bBottomMat = (matType == pgsTypes::drmBottom ? true : false);
   GetDeckMatData(poi,barType,barCategory,bTopMat,bBottomMat,bAdjForDevLength,pAs,pYb);
}

void CBridgeAgentImp::GetRebarLayout(const CSegmentKey& segmentKey, IRebarLayout** rebarLayout)
{
   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   CComPtr<IRebarLayout> rebar_layout;
   girder->get_RebarLayout(rebarLayout);
}

void CBridgeAgentImp::GetClosureJointRebarLayout(const CClosureKey& closureKey, IRebarLayout** rebarLayout)
{
   CComPtr<IPrecastGirder> girder;
   GetGirder(closureKey,&girder);

   CComPtr<IRebarLayout> rebar_layout;
   girder->get_ClosureJointRebarLayout(rebarLayout);
}

/////////////////////////////////////////////////////////////////////////
// IStirrupGeometry
bool CBridgeAgentImp::AreStirrupZonesSymmetrical(const CSegmentKey& segmentKey)
{
   const CShearData2* pShearData = GetShearData(segmentKey);
   return pShearData->bAreZonesSymmetrical;
}

ZoneIndexType CBridgeAgentImp::GetPrimaryZoneCount(const CSegmentKey& segmentKey)
{
   const CShearData2* pShearData = GetShearData(segmentKey);
   ZoneIndexType nZones = pShearData->ShearZones.size();

   if (nZones == 0)
   {
      return 0;
   }
   else
   {
      // determine the actual number of zones within the girder
	   GET_IFACE(IBridge,pBridge);
      if (pShearData->bAreZonesSymmetrical)
      {
         Float64 half_segment_length = pBridge->GetSegmentLength(segmentKey)/2.0;

         Float64 end_of_zone = 0.0;
         for (ZoneIndexType zoneIdx = 0; zoneIdx < nZones; zoneIdx++)
         {
            end_of_zone += pShearData->ShearZones[zoneIdx].ZoneLength;
            if (half_segment_length < end_of_zone)
            {
               return 2*(zoneIdx+1)-1;
            }
         }
         return nZones*2-1;
      }
      else
      {
         Float64 segment_length = pBridge->GetSegmentLength(segmentKey);

         Float64 end_of_zone = 0.0;
         for (ZoneIndexType zoneIdx = 0; zoneIdx < nZones; zoneIdx++)
         {
            end_of_zone += pShearData->ShearZones[zoneIdx].ZoneLength;
            if (segment_length < end_of_zone)
            {
               return (zoneIdx+1);
            }
         }

         return nZones;
      }
   }
}

void CBridgeAgentImp::GetPrimaryZoneBounds(const CSegmentKey& segmentKey, ZoneIndexType zone, Float64* start, Float64* end)
{
   const CShearData2* pShearData = GetShearData(segmentKey);
   Float64 segment_length = GetSegmentLength(segmentKey);

   if(pShearData->bAreZonesSymmetrical)
   {
      ZoneIndexType nz = GetPrimaryZoneCount(segmentKey);
      ATLASSERT(zone<nz);

      ZoneIndexType idx = GetPrimaryZoneIndex(segmentKey,pShearData,zone);

      // determine which side of girder zone is on
      enum side {OnLeft, OverCenter, OnRight} zside;
      if (zone == (nz-1)/2)
      {
         zside = OverCenter;
      }
      else if (idx==zone)
      {
         zside = OnLeft;
      }
      else
      {
         zside = OnRight;
      }

      ZoneIndexType zsiz = pShearData->ShearZones.size();
      Float64 l_end=0.0;
      Float64 l_start=0.0;
      for (ZoneIndexType i = 0; i <= idx; i++)
      {
         if (i==zsiz-1)
         {
            l_end+= BIG_LENGTH; // last zone in infinitely long
         }
         else
         {
            l_end+= pShearData->ShearZones[i].ZoneLength;
         }

         if (segment_length/2.0 <= l_end)
         {
            ATLASSERT(i==idx);  // better be last one or too many zones
         }

         if (i != idx)
         {
            l_start = l_end;
         }
      }

      if (zside == OnLeft)
      {
         *start = l_start;
         *end =   l_end;
      }
      else if (zside == OverCenter)
      {
         *start = l_start;
         *end =   segment_length - l_start;
      }
      else if (zside == OnRight)
      {
         *start = segment_length - l_end;
         *end =   segment_length - l_start;
      }
   }
   else
   {
      // Non-symmetrical zones
      ZoneIndexType idx = GetPrimaryZoneIndex(segmentKey,pShearData,zone);
      ZoneIndexType zsiz = pShearData->ShearZones.size();

      Float64 l_end   = 0.0;
      Float64 l_start = 0.0;
      for (ZoneIndexType i = 0; i <= idx; i++)
      {
         if (i == zsiz-1)
         {
            l_end = segment_length; // last zone goes to end of girder
         }
         else
         {
            l_end += pShearData->ShearZones[i].ZoneLength;
         }

         if (segment_length <= l_end)
         {
            l_end = segment_length;
            break;
         }

         if (i != idx)
         {
            l_start = l_end;
         }
      }

      *start = l_start;
      *end =   l_end;
   }
}

void CBridgeAgentImp::GetPrimaryVertStirrupBarInfo(const CSegmentKey& segmentKey,ZoneIndexType zone, matRebar::Size* pSize, Float64* pCount, Float64* pSpacing)
{
   const CShearData2* pShearData = GetShearData(segmentKey);
   ZoneIndexType idx = GetPrimaryZoneIndex(segmentKey,pShearData, zone);
   const CShearZoneData2& rzone = pShearData->ShearZones[idx];

   *pSize    = rzone.VertBarSize;
   *pCount   = rzone.nVertBars;
   *pSpacing = rzone.BarSpacing;
}

Float64 CBridgeAgentImp::GetPrimaryHorizInterfaceBarCount(const CSegmentKey& segmentKey,ZoneIndexType zone)
{
   const CShearData2* pShearData = GetShearData(segmentKey);
   ZoneIndexType idx = GetPrimaryZoneIndex(segmentKey,pShearData, zone);
   const CShearZoneData2& rzone = pShearData->ShearZones[idx];

   return rzone.nHorzInterfaceBars;
}

matRebar::Size CBridgeAgentImp::GetPrimaryConfinementBarSize(const CSegmentKey& segmentKey,ZoneIndexType zone)
{
   const CShearData2* pShearData = GetShearData(segmentKey);
   ZoneIndexType idx = GetPrimaryZoneIndex(segmentKey,pShearData, zone);
   const CShearZoneData2& rzone = pShearData->ShearZones[idx];

   return rzone.ConfinementBarSize;
}

ZoneIndexType CBridgeAgentImp::GetHorizInterfaceZoneCount(const CSegmentKey& segmentKey)
{
   const CShearData2* pShearData = GetShearData(segmentKey);
   ZoneIndexType nZones = pShearData->HorizontalInterfaceZones.size();
   if (nZones == 0)
   {
      return 0;
   }
   else
   {
      // determine the actual number of zones within the girder
	   GET_IFACE(IBridge,pBridge);
      if (pShearData->bAreZonesSymmetrical)
      {
         Float64 half_segment_length = pBridge->GetSegmentLength(segmentKey)/2.0;

         Float64 end_of_zone = 0.0;
         for (ZoneIndexType zoneIdx = 0; zoneIdx < nZones; zoneIdx++)
         {
            end_of_zone += pShearData->HorizontalInterfaceZones[zoneIdx].ZoneLength;
            if (half_segment_length < end_of_zone)
            {
               return 2*(zoneIdx+1)-1;
            }
         }
         return nZones*2-1;
      }
      else
      {
         Float64 segment_length = pBridge->GetSegmentLength(segmentKey);

         Float64 end_of_zone = 0.0;
         for (ZoneIndexType zoneIdx = 0; zoneIdx < nZones; zoneIdx++)
         {
            end_of_zone += pShearData->HorizontalInterfaceZones[zoneIdx].ZoneLength;
            if (segment_length < end_of_zone)
            {
               return (zoneIdx+1);
            }
         }

         return nZones;
      }
   }
}

void CBridgeAgentImp::GetHorizInterfaceZoneBounds(const CSegmentKey& segmentKey, ZoneIndexType zone, Float64* start, Float64* end)
{
   const CShearData2* pShearData = GetShearData(segmentKey);
   Float64 segment_length = GetSegmentLength(segmentKey);

   if(pShearData->bAreZonesSymmetrical)
   {
      ZoneIndexType nz = GetHorizInterfaceZoneCount(segmentKey);
      ATLASSERT(zone < nz);

      ZoneIndexType idx = GetHorizInterfaceZoneIndex(segmentKey,pShearData,zone);

      // determine which side of girder zone is on
      enum side {OnLeft, OverCenter, OnRight} zside;
      if (zone == (nz-1)/2)
      {
         zside = OverCenter;
      }
      else if (idx == zone)
      {
         zside = OnLeft;
      }
      else
      {
         zside = OnRight;
      }

      ZoneIndexType zsiz = pShearData->HorizontalInterfaceZones.size();
      Float64 l_end   = 0.0;
      Float64 l_start = 0.0;
      for (ZoneIndexType i = 0; i <= idx; i++)
      {
         if (i == zsiz-1)
         {
            l_end+= BIG_LENGTH; // last zone in infinitely long
         }
         else
         {
            l_end += pShearData->HorizontalInterfaceZones[i].ZoneLength;
         }

         if (segment_length/2.0 <= l_end)
         {
            ATLASSERT(i == idx);  // better be last one or too many zones
         }

         if (i != idx)
         {
            l_start = l_end;
         }
      }

      if (zside == OnLeft)
      {
         *start = l_start;
         *end =   l_end;
      }
      else if (zside == OverCenter)
      {
         *start = l_start;
         *end =   segment_length - l_start;
      }
      else if (zside == OnRight)
      {
         *start = segment_length - l_end;
         *end =   segment_length - l_start;
      }
   }
   else
   {
      // Non-symmetrical zones
      ZoneIndexType idx = GetHorizInterfaceZoneIndex(segmentKey,pShearData,zone);
      ZoneIndexType zsiz = pShearData->HorizontalInterfaceZones.size();

      Float64 l_end   = 0.0;
      Float64 l_start = 0.0;
      for (ZoneIndexType i = 0; i <= idx; i++)
      {
         if (i == zsiz-1)
         {
            l_end = segment_length; // last zone goes to end of girder
         }
         else
         {
            l_end += pShearData->HorizontalInterfaceZones[i].ZoneLength;
         }

         if (segment_length <= l_end)
         {
            l_end = segment_length;
            break;
         }

         if (i != idx)
         {
            l_start = l_end;
         }
      }

      *start = l_start;
      *end =   l_end;
   }
}

void CBridgeAgentImp::GetHorizInterfaceBarInfo(const CSegmentKey& segmentKey,ZoneIndexType zone, matRebar::Size* pSize, Float64* pCount, Float64* pSpacing)
{
   const CShearData2* pShearData = GetShearData(segmentKey);
   ZoneIndexType idx = GetHorizInterfaceZoneIndex(segmentKey,pShearData,zone);

   const CHorizontalInterfaceZoneData& rdata = pShearData->HorizontalInterfaceZones[idx];
   *pSize = rdata.BarSize;
   *pCount = rdata.nBars;
   *pSpacing = rdata.BarSpacing;
}

void CBridgeAgentImp::GetAddSplittingBarInfo(const CSegmentKey& segmentKey, matRebar::Size* pSize, Float64* pZoneLength, Float64* pnBars, Float64* pSpacing)
{
   const CShearData2* pShearData = GetShearData(segmentKey);
   *pSize = pShearData->SplittingBarSize;
   if (*pSize!=matRebar::bsNone)
   {
      *pZoneLength = pShearData->SplittingZoneLength;
      *pnBars = pShearData->nSplittingBars;
      *pSpacing = pShearData->SplittingBarSpacing;
   }
   else
   {
      *pZoneLength = 0.0;
      *pnBars = 0.0;
      *pSpacing = 0.0;
   }
}

void CBridgeAgentImp::GetAddConfinementBarInfo(const CSegmentKey& segmentKey, matRebar::Size* pSize, Float64* pZoneLength, Float64* pSpacing)
{
   const CShearData2* pShearData = GetShearData(segmentKey);
   *pSize = pShearData->ConfinementBarSize;
   if (*pSize != matRebar::bsNone)
   {
      *pZoneLength = pShearData->ConfinementZoneLength;
      *pSpacing = pShearData->ConfinementBarSpacing;
   }
   else
   {
      *pZoneLength = 0.0;
      *pSpacing = 0.0;
   }
}


Float64 CBridgeAgentImp::GetVertStirrupBarNominalDiameter(const pgsPointOfInterest& poi)
{
   const CShearData2* pShearData = GetShearData(poi.GetSegmentKey());
   
   const CShearZoneData2* pShearZoneData = GetPrimaryShearZoneDataAtPoi(poi, pShearData);

   matRebar::Size barSize = pShearZoneData->VertBarSize;
   if ( barSize!=matRebar::bsNone && !IsZero(pShearZoneData->BarSpacing) )
   {
      lrfdRebarPool* prp = lrfdRebarPool::GetInstance();
      const matRebar* pRebar = prp->GetRebar(pShearData->ShearBarType,pShearData->ShearBarGrade,barSize);

      return (pRebar ? pRebar->GetNominalDimension() : 0.0);
   }
   else
   {
      return 0.0;
   }
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

   const CShearData2* pShearData;
   CClosureKey closureKey;
   if ( IsInClosureJoint(poi,&closureKey) )
   {
      GET_IFACE(IShear,pShear);
      pShearData = pShear->GetClosureJointShearData(closureKey);
   }
   else
   {
      pShearData = GetShearData(poi.GetSegmentKey());
   }

   const CShearZoneData2* pShearZoneData = GetPrimaryShearZoneDataAtPoi(poi, pShearData);

   matRebar::Size barSize = pShearZoneData->VertBarSize;
   if ( barSize != matRebar::bsNone && !IsZero(pShearZoneData->BarSpacing) )
   {
      lrfdRebarPool* prp = lrfdRebarPool::GetInstance();
      const matRebar* pBar = prp->GetRebar(pShearData->ShearBarType,pShearData->ShearBarGrade,barSize);

      Abar    = pBar->GetNominalArea();
      nBars   = pShearZoneData->nVertBars;
      spacing = pShearZoneData->BarSpacing;

      // area of stirrups per unit length for this zone
      // (assume stirrups are smeared out along zone)
      if (0.0 < spacing)
      {
         avs = nBars * Abar / spacing;
      }
   }

   *pSize          = barSize;
   *pSingleBarArea = Abar;
   *pCount         = nBars;
   *pSpacing       = spacing;

   return avs;
}

bool CBridgeAgentImp::DoStirrupsEngageDeck(const CSegmentKey& segmentKey)
{
   // Just check if any stirrups engage deck
   const CShearData2* pShearData = GetShearData(segmentKey);

   CShearData2::ShearZoneConstIterator szIter(pShearData->ShearZones.begin());
   CShearData2::ShearZoneConstIterator szIterEnd(pShearData->ShearZones.end());
   for ( ; szIter != szIterEnd; szIter++ )
   {
      if (szIter->VertBarSize != matRebar::bsNone && 0 < szIter->nHorzInterfaceBars)
      {
         return true;
      }
   }

   CShearData2::HorizontalInterfaceZoneConstIterator hiIter(pShearData->HorizontalInterfaceZones.begin());
   CShearData2::HorizontalInterfaceZoneConstIterator hiIterEnd(pShearData->HorizontalInterfaceZones.end());
   for ( ; hiIter != hiIterEnd; hiIter++ )
   {
      if (hiIter->BarSize != matRebar::bsNone)
      {
         return true;
      }
   }

   return false;
}

bool CBridgeAgentImp::DoAllPrimaryStirrupsEngageDeck(const CSegmentKey& segmentKey)
{
   // Check if all vertical stirrups engage deck
   const CShearData2* pShearData = GetShearData(segmentKey);

   if (pShearData->ShearZones.empty())
   {
      return false;
   }
   else
   {
      for (CShearData2::ShearZoneConstIterator its = pShearData->ShearZones.begin(); its != pShearData->ShearZones.end(); its++)
      {
         // Make sure there are vertical bars, and at least as many horiz int bars
         if (its->VertBarSize==matRebar::bsNone ||
             its->nVertBars <= 0                ||
             its->nHorzInterfaceBars < its->nVertBars)
         {
            return false;
         }
      }
   }

   return true;
}
Float64 CBridgeAgentImp::GetPrimaryHorizInterfaceBarSpacing(const pgsPointOfInterest& poi)
{
   Float64 spacing = 0.0;

   const CShearData2* pShearData = GetShearData(poi.GetSegmentKey());

   // Horizontal legs in primary zones
   const CShearZoneData2* pShearZoneData = GetPrimaryShearZoneDataAtPoi(poi, pShearData);
   if (pShearZoneData->VertBarSize != matRebar::bsNone && 0.0 < pShearZoneData->nHorzInterfaceBars)
   {
      spacing = pShearZoneData->BarSpacing;
   }

   return spacing;
}

Float64 CBridgeAgentImp::GetPrimaryHorizInterfaceBarCount(const pgsPointOfInterest& poi)
{
   Float64 cnt = 0.0;

   const CShearData2* pShearData = GetShearData(poi.GetSegmentKey());

   // Horizontal legs in primary zones
   const CShearZoneData2* pShearZoneData = GetPrimaryShearZoneDataAtPoi(poi, pShearData);
   if (pShearZoneData->VertBarSize != matRebar::bsNone)
   {
      cnt = pShearZoneData->nHorzInterfaceBars;
   }

   return cnt;
}

Float64 CBridgeAgentImp::GetAdditionalHorizInterfaceBarSpacing(const pgsPointOfInterest& poi)
{
   Float64 spacing = 0.0;

   const CShearData2* pShearData = GetShearData(poi.GetSegmentKey());

   // Additional horizontal bars
   const CHorizontalInterfaceZoneData* pHIZoneData = GetHorizInterfaceShearZoneDataAtPoi( poi, pShearData );
   if ( pHIZoneData->BarSize != matRebar::bsNone )
   {
      spacing = pHIZoneData->BarSpacing;
   }

   return spacing;
}

Float64 CBridgeAgentImp::GetAdditionalHorizInterfaceBarCount(const pgsPointOfInterest& poi)
{
   Float64 cnt = 0.0;

   const CShearData2* pShearData = GetShearData(poi.GetSegmentKey());

   // Additional horizontal bars
   const CHorizontalInterfaceZoneData* pHIZoneData = GetHorizInterfaceShearZoneDataAtPoi( poi,pShearData );
   if ( pHIZoneData->BarSize != matRebar::bsNone )
   {
      cnt = pHIZoneData->nBars;
   }

   return cnt;
}

Float64 CBridgeAgentImp::GetPrimaryHorizInterfaceAvs(const pgsPointOfInterest& poi, matRebar::Size* pSize, Float64* pSingleBarArea, Float64* pCount, Float64* pSpacing)
{
   Float64 avs(0.0);
   Float64 Abar(0.0);
   Float64 nBars(0.0);
   Float64 spacing(0.0);

   const CShearData2* pShearData = GetShearData(poi.GetSegmentKey());
   
   // First get avs from primary bar zone
   const CShearZoneData2* pShearZoneData = GetPrimaryShearZoneDataAtPoi(poi,pShearData);

   matRebar::Size barSize = pShearZoneData->VertBarSize;

   if ( barSize != matRebar::bsNone && !IsZero(pShearZoneData->BarSpacing) && 0.0 < pShearZoneData->nHorzInterfaceBars )
   {
      lrfdRebarPool* prp = lrfdRebarPool::GetInstance();
      const matRebar* pbar = prp->GetRebar(pShearData->ShearBarType,pShearData->ShearBarGrade,barSize);

      Abar    = pbar->GetNominalArea();
      nBars   = pShearZoneData->nHorzInterfaceBars;
      spacing = pShearZoneData->BarSpacing;

      if (0.0 < spacing)
      {
         avs =   nBars * Abar / spacing;
      }
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

   const CShearData2* pShearData = GetShearData(poi.GetSegmentKey());
   
   const CHorizontalInterfaceZoneData* pHIZoneData = GetHorizInterfaceShearZoneDataAtPoi( poi, pShearData );

   matRebar::Size barSize = pHIZoneData->BarSize;

   if ( barSize != matRebar::bsNone && !IsZero(pHIZoneData->BarSpacing) && 0.0 < pHIZoneData->nBars )
   {
      lrfdRebarPool* prp = lrfdRebarPool::GetInstance();
      const matRebar* pbar = prp->GetRebar(pShearData->ShearBarType,pShearData->ShearBarGrade,barSize);

      Abar    = pbar->GetNominalArea();
      nBars   = pHIZoneData->nBars;
      spacing = pHIZoneData->BarSpacing;

      if (0.0 < spacing)
      {
         avs =  nBars * Abar / spacing;
      }
   }

   *pSize          = barSize;
   *pSingleBarArea = Abar;
   *pCount         = nBars;
   *pSpacing       = spacing;

   return avs;
}

Float64 CBridgeAgentImp::GetSplittingAv(const CSegmentKey& segmentKey,Float64 start,Float64 end)
{
   ATLASSERT(end>start);

   Float64 Av = 0.0;

   const CShearData2* pShearData = GetShearData(segmentKey);

   // Get component from primary bars
   if (pShearData->bUsePrimaryForSplitting)
   {
      Av += GetPrimarySplittingAv( segmentKey, start, end, pShearData);
   }

   // Component from additional splitting bars
   if (pShearData->SplittingBarSize != matRebar::bsNone && pShearData->nSplittingBars)
   {
      Float64 spacing = pShearData->SplittingBarSpacing;
      Float64 length = 0.0;
      if (spacing <= 0.0)
      {
         ATLASSERT(false); // UI should block this
      }
      else
      {
         // determine how much additional bars is in our start/end region
         Float64 zone_length = pShearData->SplittingZoneLength;
         // left end first
         if (start < zone_length)
         {
            Float64 zend = Min(end, zone_length);
            length = zend-start;
         }
         else
         {
            // try right end
            Float64 segment_length = GetSegmentLength(segmentKey);
            if (segment_length <= end)
            {
               Float64 zstart = Max(segment_length-zone_length, start);
               length = end-zstart;
            }
         }

         if (0.0 < length)
         {
            // We have bars in region. multiply av/s * length
            lrfdRebarPool* prp = lrfdRebarPool::GetInstance();
            const matRebar* pbar = prp->GetRebar(pShearData->ShearBarType, pShearData->ShearBarGrade, pShearData->SplittingBarSize);

            Float64 Abar = pbar->GetNominalArea();
            Float64 avs  = pShearData->nSplittingBars * Abar / spacing;

            Av += avs * length;
         }
      }
   }

   return Av;
}

Float64 CBridgeAgentImp::GetPrimarySplittingAv(const CSegmentKey& segmentKey,Float64 start,Float64 end, const CShearData2* pShearData)
{
   if (!pShearData->bUsePrimaryForSplitting)
   {
      ATLASSERT(false); // shouldn't be called for this case
      return 0.0;
   }

   // Get total amount of splitting steel between start and end
   Float64 Av = 0;

   ZoneIndexType nbrZones = GetPrimaryZoneCount(segmentKey);
   for ( ZoneIndexType zone = 0; zone < nbrZones; zone++ )
   {
      Float64 zoneStart, zoneEnd;
      GetPrimaryZoneBounds(segmentKey, zone, &zoneStart, &zoneEnd);

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
      ZoneIndexType idx = GetPrimaryZoneIndex(segmentKey, pShearData, zone);

      const CShearZoneData2& shearZoneData = pShearData->ShearZones[idx];

      matRebar::Size barSize = shearZoneData.VertBarSize; // splitting is same as vert bars
      if ( barSize != matRebar::bsNone && !IsZero(shearZoneData.BarSpacing) )
      {
         lrfdRebarPool* prp = lrfdRebarPool::GetInstance();
         const matRebar* pbar = prp->GetRebar(pShearData->ShearBarType,pShearData->ShearBarGrade,barSize);

         Float64 Abar   = pbar->GetNominalArea();

         // area of stirrups per unit length for this zone
         // (assume stirrups are smeared out along zone)
         Float64 avs = shearZoneData.nVertBars * Abar / shearZoneData.BarSpacing;

         Av += avs * length;
      }
   }

   return Av;
}

void CBridgeAgentImp::GetStartConfinementBarInfo(const CSegmentKey& segmentKey, Float64 requiredZoneLength, matRebar::Size* pSize, Float64* pProvidedZoneLength, Float64* pSpacing)
{
   const CShearData2* pShearData = GetShearData(segmentKey);

   ZoneIndexType nbrZones = GetPrimaryZoneCount(segmentKey);

   // First get data from primary zones - use min bar size and max spacing from zones in required region
   Float64 primSpc(-1), primZonL(-1);
   matRebar::Size primSize(matRebar::bsNone);

   Float64 ezloc;

   // walk from left to right on girder
   for ( ZoneIndexType zone = 0; zone < nbrZones; zone++ )
   {
      Float64 zoneStart;
      GetPrimaryZoneBounds(segmentKey, zone, &zoneStart, &ezloc);

      ZoneIndexType idx = GetPrimaryZoneIndex(segmentKey, pShearData, zone);

      const CShearZoneData2& shearZoneData = pShearData->ShearZones[idx];

      if (shearZoneData.ConfinementBarSize != matRebar::bsNone)
      {
         primSize = Min(primSize, shearZoneData.ConfinementBarSize);
         primSpc  = Max(primSpc,  shearZoneData.BarSpacing);

         primZonL = ezloc;
      }

      if (requiredZoneLength < ezloc + TOL)
      {
         break; // actual zone length exceeds required - we are done
      }
   }

   // Next get additional confinement bar info
   Float64 addlSpc, addlZonL;
   matRebar::Size addlSize;
   GetAddConfinementBarInfo(segmentKey, &addlSize, &addlZonL, &addlSpc);

   // Use either primary bars or additional bars. Choose by which has addequate zone length, smallest spacing, largest bars
   ChooseConfinementBars(requiredZoneLength, primSpc, primZonL, primSize, addlSpc, addlZonL, addlSize,
                         pSize, pProvidedZoneLength, pSpacing);
}


void CBridgeAgentImp::GetEndConfinementBarInfo( const CSegmentKey& segmentKey, Float64 requiredZoneLength, matRebar::Size* pSize, Float64* pProvidedZoneLength, Float64* pSpacing)
{
   const CShearData2* pShearData = GetShearData(segmentKey);

   Float64 segment_length = GetSegmentLength(segmentKey);

   ZoneIndexType nbrZones = GetPrimaryZoneCount(segmentKey);

   // First get data from primary zones - use min bar size and max spacing from zones in required region
   Float64 primSpc(-1), primZonL(-1);
   matRebar::Size primSize(matRebar::bsNone);

   Float64 ezloc;
   // walk from right to left on girder
   for ( ZoneIndexType zone = nbrZones-1; zone>=0; zone-- )
   {
      Float64 zoneStart, zoneEnd;
      GetPrimaryZoneBounds(segmentKey, zone, &zoneStart, &zoneEnd);

      ezloc = segment_length - zoneStart;

      ZoneIndexType idx = GetPrimaryZoneIndex(segmentKey, pShearData, zone);

      const CShearZoneData2* pShearZoneData = &pShearData->ShearZones[idx];

      if (pShearZoneData->ConfinementBarSize != matRebar::bsNone)
      {
         primSize = Min(primSize, pShearZoneData->ConfinementBarSize);
         primSpc  = Max(primSpc, pShearZoneData->BarSpacing);

         primZonL = ezloc;
      }

      if (requiredZoneLength < ezloc + TOL)
      {
         break; // actual zone length exceeds required - we are done
      }
   }

   // Next get additional confinement bar info
   Float64 addlSpc, addlZonL;
   matRebar::Size addlSize;
   GetAddConfinementBarInfo(segmentKey, &addlSize, &addlZonL, &addlSpc);

   // Use either primary bars or additional bars. Choose by which has addequate zone length, smallest spacing, largest bars
   ChooseConfinementBars(requiredZoneLength, primSpc, primZonL, primSize, addlSpc, addlZonL, addlSize,
                         pSize, pProvidedZoneLength, pSpacing);
}

bool CBridgeAgentImp::AreStirrupZoneLengthsCombatible(const CGirderKey& girderKey)
{
   SegmentIndexType nSegments = GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      const CShearData2* pShearData = GetShearData(segmentKey);

      ZoneIndexType squishyZoneIdx = INVALID_INDEX;
      if ( pShearData->bAreZonesSymmetrical )
      {
         // if zones are symmetrical, the last zone input is the "squishy" zone
         squishyZoneIdx = pShearData->ShearZones.size()-1;
      }


      ZoneIndexType nZones = GetPrimaryZoneCount(segmentKey);
      for (ZoneIndexType zoneIdx = 0; zoneIdx < nZones; zoneIdx++ )
      {
         Float64 zoneStart, zoneEnd;
         GetPrimaryZoneBounds(segmentKey, zoneIdx, &zoneStart, &zoneEnd);
         Float64 zoneLength = zoneEnd-zoneStart;

         matRebar::Size barSize;
         Float64 spacing;
         Float64 nStirrups;
         GetPrimaryVertStirrupBarInfo(segmentKey,zoneIdx,&barSize,&nStirrups,&spacing);

         if (barSize != matRebar::bsNone && TOLERANCE < spacing)
         {
            // If spacings fit within 1%, then pass. Otherwise fail
            Float64 nFSpaces = zoneLength / spacing;
            Int32 nSpaces = (Int32)nFSpaces;
            Float64 remainder = nFSpaces - nSpaces;

            if ( zoneIdx != squishyZoneIdx && 
                 !IsZero(remainder, 0.01) && 
                 !IsEqual(remainder, 1.0, 0.01) )
            {
               return false;
            }
         }
      }
   } // next segment

   return true;
}

// private:

void CBridgeAgentImp::InvalidateStirrupData()
{
   m_ShearData.clear();
}

const CShearData2* CBridgeAgentImp::GetShearData(const CSegmentKey& segmentKey)
{
   ShearDataIterator found;
   found = m_ShearData.find( segmentKey );
   if ( found == m_ShearData.end() )
   {
	   GET_IFACE2(m_pBroker,IShear,pShear);
	   const CShearData2* pShearData = pShear->GetSegmentShearData(segmentKey);
      std::pair<ShearDataIterator,bool> insit = m_ShearData.insert( std::make_pair(segmentKey, *pShearData ) );
      ATLASSERT( insit.second );
      return &(*insit.first).second;
   }
   else
   {
      ATLASSERT( found != m_ShearData.end() );
      return &(*found).second;
   }
}

ZoneIndexType CBridgeAgentImp::GetPrimaryZoneIndex(const CSegmentKey& segmentKey, const CShearData2* pShearData, ZoneIndexType zone)
{
   // mapping so that we only need to store half of the zones
   ZoneIndexType nZones = GetPrimaryZoneCount(segmentKey); 
   ATLASSERT(zone < nZones);
   if (pShearData->bAreZonesSymmetrical)
   {
      ZoneIndexType nz2 = (nZones+1)/2;
      if (zone < nz2)
      {
         return zone;
      }
      else
      {
         return nZones-zone-1;
      }
   }
   else
   {
      // mapping is 1:1 for non-sym
      return zone;
   }
}

ZoneIndexType CBridgeAgentImp::GetHorizInterfaceZoneIndex(const CSegmentKey& segmentKey, const CShearData2* pShearData, ZoneIndexType zone)
{
   // mapping so that we only need to store half of the zones
   ZoneIndexType nZones = GetHorizInterfaceZoneCount(segmentKey); 
   ATLASSERT(zone < nZones);
   if (pShearData->bAreZonesSymmetrical)
   {
      ZoneIndexType nz2 = (nZones+1)/2;
      if (zone < nz2)
      {
         return zone;
      }
      else
      {
         return nZones-zone-1;
      }
   }
   else
   {
      // mapping is 1:1 for non-sym
      return zone;
   }
}

ZoneIndexType CBridgeAgentImp::GetPrimaryShearZoneIndexAtPoi(const pgsPointOfInterest& poi, const CShearData2* pShearData)
{
   // NOTE: The logic here is identical to GetHorizInterfaceShearZoneIndexAtPoi
   //       If you fix a bug here, you need to fix it there also

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ZoneIndexType nz = GetPrimaryZoneCount(segmentKey);
   if (nz == 0)
   {
      return INVALID_INDEX;
   }

   Float64 location, length, left_bearing_location, right_bearing_location;
   CClosureKey closureKey;
   if ( IsInClosureJoint(poi,&closureKey) )
   {
      length = GetClosureJointLength(closureKey);
      if ( poi.GetDistFromStart() < 0 )
      {
         // poi is beyond the CL of the closure so it is measured in segment coordinates for the
         // segment that comes after the closure. location is the distance from the start
         // face of the closure

         // CL Closure -+
         //             |   Xpoi < 0
         //             |  |<--->|
         // ----+       |  |     +-----------
         //     |       |  *poi  |
         // ----+       |  |     +-----------
         //     | location |     |
         //     |<-------->|     |
         //     |    length      |
         //     |<-------------->|

         location = length + poi.GetDistFromStart();
      }
      else
      {
         // poi is before the CL Closure
         location = poi.GetDistFromStart() - GetSegmentLength(closureKey);
      }
      left_bearing_location  = 0;
      right_bearing_location = 0;
   }
   else if ( poi.HasAttribute(POI_BOUNDARY_PIER) || poi.GetDistFromStart() < 0 )
   {
      length = 0;
      location = 0;
      left_bearing_location = 0;
      right_bearing_location = 0;
   }
   else
   {
      length = GetSegmentLength(segmentKey);
      location = poi.GetDistFromStart();
      left_bearing_location  = GetSegmentStartBearingOffset(segmentKey);
      right_bearing_location = length - GetSegmentEndBearingOffset(segmentKey);
   }

   // use template function to do heavy work
   ZoneIndexType zone =  GetZoneIndexAtLocation(location, length, left_bearing_location, right_bearing_location, pShearData->bAreZonesSymmetrical, 
                                                pShearData->ShearZones.begin(), pShearData->ShearZones.end(), 
                                                pShearData->ShearZones.size());

   return zone;
}

const CShearZoneData2* CBridgeAgentImp::GetPrimaryShearZoneDataAtPoi(const pgsPointOfInterest& poi, const CShearData2* pShearData)
{
   ZoneIndexType idx = GetPrimaryShearZoneIndexAtPoi(poi,pShearData);
   return &pShearData->ShearZones[idx];
}

ZoneIndexType CBridgeAgentImp::GetHorizInterfaceShearZoneIndexAtPoi(const pgsPointOfInterest& poi, const CShearData2* pShearData)
{
   // NOTE: The logic here is identical to GetPrimaryShearZoneAtPoi
   //       If you fix a bug here, you need to fix it there also

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ZoneIndexType nz = GetHorizInterfaceZoneCount(segmentKey);
   if (nz == 0)
   {
      return INVALID_INDEX;
   }

   Float64 location, length, left_bearing_location, right_bearing_location;
   CClosureKey closureKey;
   if ( IsInClosureJoint(poi,&closureKey) )
   {
      length = GetClosureJointLength(closureKey);
      location = poi.GetDistFromStart() - GetSegmentLength(closureKey);
      left_bearing_location  = 0;
      right_bearing_location = 0;
   }
   else if ( poi.HasAttribute(POI_BOUNDARY_PIER) || poi.GetDistFromStart() < 0 )
   {
      length = 0;
      location = 0;
      left_bearing_location = 0;
      right_bearing_location = 0;
   }
   else
   {
      length = GetSegmentLength(segmentKey);
      location = poi.GetDistFromStart();
      left_bearing_location  = GetSegmentStartBearingOffset(segmentKey);
      right_bearing_location = length - GetSegmentEndBearingOffset(segmentKey);
   }

   // use template function to do heavy work
   ZoneIndexType zone =  GetZoneIndexAtLocation(location, length, left_bearing_location, right_bearing_location, pShearData->bAreZonesSymmetrical, 
                                                pShearData->HorizontalInterfaceZones.begin(), pShearData->HorizontalInterfaceZones.end(), 
                                                pShearData->HorizontalInterfaceZones.size());


   return zone;
}

const CHorizontalInterfaceZoneData* CBridgeAgentImp::GetHorizInterfaceShearZoneDataAtPoi(const pgsPointOfInterest& poi, const CShearData2* pShearData)
{
   ZoneIndexType idx = GetHorizInterfaceShearZoneIndexAtPoi(poi,pShearData);
   if ( idx == INVALID_INDEX )
   {
      ATLASSERT(false);
      return NULL;
   }
   return &pShearData->HorizontalInterfaceZones[idx];
}

/////////////////////////////////////////////////////////////////////////
// IStrandGeometry
//
Float64 CBridgeAgentImp::GetEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bIncTemp, Float64* nEffectiveStrands)
{
   pgsTypes::SectionPropertyType spType = (GetSectionPropertiesMode() == pgsTypes::spmGross ? pgsTypes::sptGross : pgsTypes::sptTransformed);
   return GetEccentricity(spType,intervalIdx,poi,bIncTemp,nEffectiveStrands);
}

Float64 CBridgeAgentImp::GetEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,Float64* pnEffectiveStrands)
{
   pgsTypes::SectionPropertyType spType = (GetSectionPropertiesMode() == pgsTypes::spmGross ? pgsTypes::sptGross : pgsTypes::sptTransformed);
   return GetEccentricity(spType,intervalIdx,poi,strandType,pnEffectiveStrands);
}

Float64 CBridgeAgentImp::GetEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bIncTemp, Float64* nEffectiveStrands)
{
   if ( IsOffSegment(poi) )
   {
      return 0.0;
   }

   ATLASSERT( spType != pgsTypes::sptNetDeck );

   VALIDATE( GIRDER );

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CComPtr<IPrecastGirder> girder;
   GetGirder(poi,&girder);

   Float64 nStraight  = 0;
   Float64 nHarped    = 0;
   Float64 nTemporary = 0;

   Float64 ecc_straight  = 0;
   Float64 ecc_harped    = 0;
   Float64 ecc_temporary = 0;

   Float64 ecc = 0;

   ecc_straight = GetEccentricity(spType,intervalIdx,poi,pgsTypes::Straight,&nStraight);
   ecc_harped   = GetEccentricity(spType,intervalIdx,poi,pgsTypes::Harped,  &nHarped);

   Float64 asStraight  = GetStrandMaterial(segmentKey,pgsTypes::Straight)->GetNominalArea()*nStraight;
   Float64 asHarped    = GetStrandMaterial(segmentKey,pgsTypes::Harped)->GetNominalArea()*nHarped;
   Float64 asTemporary = 0;

   if ( bIncTemp )
   {
      ecc_temporary = GetEccentricity(spType,intervalIdx,poi,pgsTypes::Temporary,&nTemporary);
      asTemporary   = GetStrandMaterial(segmentKey,pgsTypes::Temporary)->GetNominalArea()*nTemporary;
   }

   *nEffectiveStrands = nStraight + nHarped + nTemporary;
   if ( 0.0 < *nEffectiveStrands )
   {
      ecc = (asStraight*ecc_straight + asHarped*ecc_harped + asTemporary*ecc_temporary) / (asStraight + asHarped + asTemporary);
   }
   else
   {
      ecc = 0.0;
   }

   return ecc;
}

Float64 CBridgeAgentImp::GetEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StrandType strandType, Float64* nEffectiveStrands)
{
   if ( IsOffSegment(poi) )
   {
      return 0.0;
   }


   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   Float64 e = -9999999999;
   switch( strandType )
   {
   case pgsTypes::Straight:
      e = GetSsEccentricity(spType,intervalIdx,poi,nEffectiveStrands);
      break;

   case pgsTypes::Harped:
      e = GetHsEccentricity(spType,intervalIdx,poi,nEffectiveStrands);
      break;
   
   case pgsTypes::Temporary:
      e = GetTempEccentricity(spType,intervalIdx,poi,nEffectiveStrands);
      break;

   case pgsTypes::Permanent:
      {
       StrandIndexType Ns = GetStrandCount(segmentKey,pgsTypes::Straight);
       StrandIndexType Nh = GetStrandCount(segmentKey,pgsTypes::Harped);
       Float64 NsEffective, NhEffective;
       Float64 es = GetSsEccentricity(spType,intervalIdx,poi,&NsEffective);
       Float64 eh = GetHsEccentricity(spType,intervalIdx,poi,&NhEffective);
       *nEffectiveStrands = NsEffective + NhEffective;
       e = (*nEffectiveStrands == 0 ? 0 : (es*NsEffective + eh*NhEffective)/(NsEffective+NhEffective));
      }
      break;

   default:
      ATLASSERT(false);
   }

   return e;
}

Float64 CBridgeAgentImp::GetStrandLocation(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx)
{
   Float64 Y = 0;
   switch( strandType )
   {
   case pgsTypes::Straight:
      Y = GetSsLocation(poi,intervalIdx);
      break;

   case pgsTypes::Harped:
      Y = GetHsLocation(poi,intervalIdx);
      break;

   case pgsTypes::Temporary:
      Y = GetTempLocation(poi,intervalIdx);
      break;

   default:
      ATLASSERT(false);
   }

   ATLASSERT( Y <= 0 ); // this is in Girder Section Coordinates. Since strand is below top of girder its position should be negative
   return Y;
}

Float64 CBridgeAgentImp::GetHsLocation(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx)
{
   VALIDATE( GIRDER );

   // if the strands aren't released yet, then there isn't an eccentricty with respect to the girder cross section
   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   if ( intervalIdx < GetPrestressReleaseInterval(segmentKey) || IsOffSegment(poi) )
   {
      return 0;
   }

   CComPtr<IPrecastGirder> girder;
   GetGirder(poi,&girder);

   Float64 Xs = poi.GetDistFromStart();

   CComPtr<IPoint2dCollection> points;
   girder->get_HarpedStrandPositions(Xs,&points);

   CollectionIndexType nStrands;
   points->get_Count(&nStrands);

   Float64 A = 0;
   Float64 Ay = 0;

   for (CollectionIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++)
   {
      CComPtr<IPoint2d> point;
      points->get_Item(strandIdx,&point);
      Float64 y;
      point->get_Y(&y);

      A += 1;
      Ay += y;
   }

   Float64 Y = (IsZero(A) ? 0 : Ay/A);
   return Y;
}

Float64 CBridgeAgentImp::GetSsLocation(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx)
{
   // Returns the distance from the top of the girder to the strand
   VALIDATE( GIRDER );

   // if the strands aren't released yet, then there isn't an eccentricty with respect to the girder cross section
   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   if ( intervalIdx < GetPrestressReleaseInterval(segmentKey) || IsOffSegment(poi) )
   {
      return 0;
   }

   CComPtr<IPrecastGirder> girder;
   GetGirder(poi,&girder);

   StrandIndexType nStrands;
   girder->GetStraightStrandCount(&nStrands);

   Float64 Xs = poi.GetDistFromStart();

   CComPtr<IIndexArray> debondPositions;
   girder->GetStraightStrandsDebondedByPositionIndex(etStart,Xs,&debondPositions);

   Float64 A = 0;
   Float64 Ay = 0;

   for (StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++)
   {
      Float64 y, left_bond, right_bond;
      girder->GetStraightStrandBondedLengthByPositionIndex(strandIdx, Xs, &y, &left_bond, &right_bond);

      IndexType foundIdx;
      HRESULT hr = debondPositions->Find(strandIdx,&foundIdx);
      if ( FAILED(hr) )
      {
         // Strand is NOT debonded at this location....
         A += 1;
         Ay += y;
      }
   }

   Float64 Y = IsZero(A) ? 0 : Ay/A;
   return Y;
}

Float64 CBridgeAgentImp::GetTempLocation(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx)
{
   VALIDATE( GIRDER );

   // if the strands aren't released yet, then there isn't an eccentricty with respect to the girder cross section
   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   if ( intervalIdx < GetPrestressReleaseInterval(segmentKey) || IsOffSegment(poi) )
   {
      return 0;
   }

   CComPtr<IPrecastGirder> girder;
   GetGirder(poi,&girder);

   Float64 Xs = poi.GetDistFromStart();

   CComPtr<IPoint2dCollection> points;
   girder->get_TemporaryStrandPositions(Xs, &points);

   CollectionIndexType nStrands;
   points->get_Count(&nStrands);

   Float64 A = 0;
   Float64 Ay = 0;

   for (CollectionIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++)
   {
      CComPtr<IPoint2d> point;
      points->get_Item(strandIdx,&point);
      Float64 y;
      point->get_Y(&y);

      A += 1;
      Ay += y;
   }

   Float64 Y = (IsZero(A) ? 0 : Ay/A);
   return Y;
}

Float64 CBridgeAgentImp::GetMaxStrandSlope(const pgsPointOfInterest& poi)
{
   // No harped strands in a closure joint
   if ( IsOffSegment(poi) )
   {
      return 0.0;
   }

   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(poi,&girder);

   Float64 slope;
   girder->ComputeMaxHarpedStrandSlope(poi.GetDistFromStart(),&slope);

   return slope;
}

Float64 CBridgeAgentImp::GetMaxStrandSlope(const CSegmentKey& segmentKey)
{
   std::vector<pgsPointOfInterest> vPoi = GetPointsOfInterest(segmentKey,POI_HARPINGPOINT);
   if ( vPoi.size() == 0 )
   {
      // No harping points
      ATLASSERT(GetStrandCount(segmentKey,pgsTypes::Harped) == 0);
      return 0;
   }

   Float64 maxSlope = 0; // magnitude of max slope
   std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      pgsPointOfInterest& poi(*iter);
      Float64 slope = GetMaxStrandSlope(poi);

      // compare magnitudes of slope
      if ( fabs(maxSlope) < fabs(slope) )
      {
         maxSlope = slope; // retain the sign
      }
   }

   return maxSlope;
}

Float64 CBridgeAgentImp::GetAvgStrandSlope(const pgsPointOfInterest& poi)
{
   // No harped strands in a closure joint
   if ( IsOffSegment(poi) )
   {
      return 0.0;
   }

   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(poi,&girder);

   Float64 slope;
   girder->ComputeAvgHarpedStrandSlope(poi.GetDistFromStart(),&slope);

   return slope;
}

Float64 CBridgeAgentImp::GetHsEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, Float64* nEffectiveStrands)
{
   if ( IsOffSegment(poi) )
   {
      return 0.0; // strands aren't located in closure joint
   }

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   StrandIndexType Nh = rconfig.PrestressConfig.GetStrandCount(pgsTypes::Harped);

   ATLASSERT(rconfig.PrestressConfig.Debond[pgsTypes::Harped].empty()); // we don't deal with debonding of harped strands for design. WBFL needs to be updated

   VALIDATE( GIRDER );

   if (Nh == 0)
   {
      *nEffectiveStrands = 0.0;
      return 0.0;
   }
   else
   {
      CComPtr<IPrecastGirder> girder;
      GetGirder(poi,&girder);

      // must account for partial bonding if poi is near end of girder
      // NOTE: bond factor is 1.0 if at girder ends
      Float64 Xpoi = poi.GetDistFromStart();
      Float64 bond_factor = 1.0;

      GET_IFACE(IPretensionForce,pPrestress);
      Float64 xfer_length = pPrestress->GetXferLength(segmentKey,pgsTypes::Harped);

      if (0.0 < Xpoi && Xpoi < xfer_length)
      {
         bond_factor = Xpoi/xfer_length;
      }
      else
      {
         Float64 girder_length = GetSegmentLength(segmentKey);

         if ( (girder_length-xfer_length) < Xpoi && Xpoi < girder_length)
         {
            bond_factor = (girder_length-Xpoi)/xfer_length;
         }
      }

      // use continuous interface to set strands
      CIndexArrayWrapper strand_fill(rconfig.PrestressConfig.GetStrandFill(pgsTypes::Harped));

      // Use CStrandMoverSwapper to swap out girder's strand mover and harping offset limits
      // temporarily for design
      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      CStrandMoverSwapper swapper(segmentKey, rconfig.PrestressConfig, this, girder, pIBridgeDesc);

      CComPtr<IPoint2dCollection> points;
      girder->get_HarpedStrandPositionsEx(poi.GetDistFromStart(), &strand_fill, &points);

      Float64 ecc=0.0;

      // compute cg
      CollectionIndexType num_strand_positions;
      points->get_Count(&num_strand_positions);

      *nEffectiveStrands = bond_factor*num_strand_positions;

      ATLASSERT( Nh == StrandIndexType(num_strand_positions));
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

         Float64 Yb = GetY(spType,intervalIdx,poi,pgsTypes::BottomGirder);
         Float64 Hg = GetHg(spType,intervalIdx,poi);
         ecc = Yb-(Hg+cg);
      }

      return ecc;
   }
}

Float64 CBridgeAgentImp::GetSsEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, Float64* nEffectiveStrands)
{
   if ( IsOffSegment(poi) )
   {
      return 0.0; // strands aren't located in closure joint
   }

   StrandIndexType Ns = rconfig.PrestressConfig.GetStrandCount(pgsTypes::Straight);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   VALIDATE( GIRDER );

   if (Ns == 0)
   {
      *nEffectiveStrands = 0;
      return 0.0;
   }
   else
   {
      CComPtr<IPrecastGirder> girder;
      GetGirder(poi,&girder);

      Float64 girder_length = GetSegmentLength(segmentKey);
      Float64 Xpoi = poi.GetDistFromStart();

      // transfer
      GET_IFACE(IPretensionForce,pPrestress);
      Float64 xfer_length = pPrestress->GetXferLength(segmentKey,pgsTypes::Straight);

      // debonding - Let's set up a mapping data structure to make dealing with debonding easier (eliminate searching)
      const DebondConfigCollection& rdebonds = rconfig.PrestressConfig.Debond[pgsTypes::Straight];

      std::vector<Int16> debond_map;
      debond_map.assign(Ns, -1); // strands not debonded have and index of -1
      Int16 dbinc=0;
      for (DebondConfigConstIterator idb=rdebonds.begin(); idb!=rdebonds.end(); idb++)
      {
         const DEBONDCONFIG& dbinfo = *idb;
         if (dbinfo.strandIdx<Ns)
         {
            debond_map[dbinfo.strandIdx] = dbinc; 
         }
         else
         {
            ATLASSERT(false); // shouldn't be debonding any strands that don't exist
         }

         dbinc++;
      }

      // get all current straight strand locations
      CIndexArrayWrapper strand_fill(rconfig.PrestressConfig.GetStrandFill(pgsTypes::Straight));

      CComPtr<IPoint2dCollection> points;
      girder->get_StraightStrandPositionsEx(Xpoi, &strand_fill, &points);

      CollectionIndexType num_strand_positions;
      points->get_Count(&num_strand_positions);
      ATLASSERT( Ns == num_strand_positions);

      // loop over all strands, compute bonded length, and weighted cg of strands
      Float64 cg = 0.0;
      Float64 num_eff_strands = 0.0;

      // special cases are where POI is at girder ends. For this, weight strands fully
      if (IsZero(Xpoi))
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
 
               if (dbinfo.DebondLength[pgsTypes::metStart]==0.0)
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
      else if (IsEqual(Xpoi, girder_length))
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
 
               if (dbinfo.DebondLength[pgsTypes::metEnd]==0.0)
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
               lft_bond = Xpoi - dbinfo.DebondLength[pgsTypes::metStart];
               rgt_bond = girder_length - dbinfo.DebondLength[pgsTypes::metEnd] - Xpoi;
            }
            else
            {
               lft_bond = Xpoi;
               rgt_bond = girder_length - Xpoi;
            }

            Float64 bond_len = Min(lft_bond, rgt_bond);
            if (bond_len <= 0.0)
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

      if (0.0 < num_eff_strands)
      {
         cg = cg/num_eff_strands;

         Float64 Yb = GetY(spType,intervalIdx,poi,pgsTypes::BottomGirder);
         Float64 Hg = GetHg(spType,intervalIdx,poi);

         Float64 ecc = Yb - (Hg+cg);
         return ecc;
      }
      else
      {
         // all strands debonded out
         return 0.0;
      }
   }
}

Float64 CBridgeAgentImp::GetTempEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, Float64* nEffectiveStrands)
{
   if ( IsOffSegment(poi) )
   {
      return 0.0;
   }

   StrandIndexType Nt = rconfig.PrestressConfig.GetStrandCount(pgsTypes::Temporary);

   ATLASSERT(rconfig.PrestressConfig.Debond[pgsTypes::Temporary].empty()); // we don't deal with debonding of temp strands for design. WBFL needs to be updated

   VALIDATE( GIRDER );

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   if (Nt == 0)
   {
      *nEffectiveStrands = 0.0;
      return 0.0;
   }
   else
   {
      CComPtr<IPrecastGirder> girder;
      GetGirder(poi,&girder);

      // must account for partial bonding if poi is near end of girder
      // NOTE: bond factor is 1.0 if at girder ends
      Float64 Xpoi = poi.GetDistFromStart();
      Float64 bond_factor = 1.0;

      GET_IFACE(IPretensionForce,pPrestress);
      Float64 xfer_length = pPrestress->GetXferLength(segmentKey,pgsTypes::Temporary);

      if (0.0 < Xpoi && Xpoi < xfer_length)
      {
         bond_factor = Xpoi/xfer_length;
      }
      else
      {
         Float64 girder_length = GetSegmentLength(segmentKey);

         if ( (girder_length-xfer_length) < Xpoi && Xpoi < girder_length)
         {
            bond_factor = (girder_length-Xpoi)/xfer_length;
         }
      }

      // use continuous interface to set strands
      CIndexArrayWrapper strand_fill(rconfig.PrestressConfig.GetStrandFill(pgsTypes::Temporary));

      CComPtr<IPoint2dCollection> points;
      girder->get_TemporaryStrandPositionsEx(Xpoi, &strand_fill, &points);

      Float64 ecc = 0.0;

      // compute cg
      CollectionIndexType num_strands_positions;
      points->get_Count(&num_strands_positions);

      *nEffectiveStrands = bond_factor * num_strands_positions;

      ATLASSERT(Nt == StrandIndexType(num_strands_positions));
      Float64 cg = 0.0;
      for (CollectionIndexType strandPositionIdx = 0; strandPositionIdx < num_strands_positions; strandPositionIdx++)
      {
         CComPtr<IPoint2d> point;
         points->get_Item(strandPositionIdx,&point);
         Float64 y;
         point->get_Y(&y);

         cg += y;
      }

      cg = cg / (Float64)num_strands_positions;

      Float64 Yb = GetY(spType,intervalIdx,poi,pgsTypes::BottomGirder);
      Float64 Hg = GetHg(spType,intervalIdx,poi);
      ecc = Yb-(Hg+cg);

      return ecc;
   }
}

Float64 CBridgeAgentImp::GetEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, bool bIncTemp, Float64* nEffectiveStrands)
{
   pgsTypes::SectionPropertyType spType = (GetSectionPropertiesMode() == pgsTypes::spmGross ? pgsTypes::sptGross : pgsTypes::sptTransformed);
   return GetEccentricity(spType,intervalIdx,poi,rconfig,bIncTemp,nEffectiveStrands);
}

Float64 CBridgeAgentImp::GetEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, pgsTypes::StrandType strandType, Float64* nEffectiveStrands)
{
   pgsTypes::SectionPropertyType spType = (GetSectionPropertiesMode() == pgsTypes::spmGross ? pgsTypes::sptGross : pgsTypes::sptTransformed);
   return GetEccentricity(spType,intervalIdx,poi,rconfig,strandType,nEffectiveStrands);
}
   
Float64 CBridgeAgentImp::GetEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, bool bIncTemp, Float64* nEffectiveStrands)
{
   if ( IsOffSegment(poi) )
   {
      return 0.0;
   }

   VALIDATE( GIRDER );

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CComPtr<IPrecastGirder> girder;
   GetGirder(poi,&girder);

   Float64 dns = 0.0;
   Float64 dnh = 0.0;
   Float64 dnt = 0.0;

   Float64 eccs = GetEccentricity(spType,intervalIdx, poi, rconfig, pgsTypes::Straight, &dns);
   Float64 ecch = GetEccentricity(spType,intervalIdx, poi, rconfig, pgsTypes::Harped,   &dnh);
   Float64 ecct = 0.0;

   Float64 aps[3];
   aps[pgsTypes::Straight]  = GetStrandMaterial(segmentKey,pgsTypes::Straight)->GetNominalArea() * dns;
   aps[pgsTypes::Harped]    = GetStrandMaterial(segmentKey,pgsTypes::Harped)->GetNominalArea()   * dnh;
   aps[pgsTypes::Temporary] = 0;

   if (bIncTemp)
   {
      ecct = GetEccentricity(spType,intervalIdx, poi, rconfig, pgsTypes::Temporary, &dnt);
      aps[pgsTypes::Temporary] = GetStrandMaterial(segmentKey,pgsTypes::Temporary)->GetNominalArea()* dnt;
   }

   Float64 ecc=0.0;
   *nEffectiveStrands = dns + dnh + dnt;
   if (0 < *nEffectiveStrands)
   {
      ecc = (aps[pgsTypes::Straight]*eccs + aps[pgsTypes::Harped]*ecch + aps[pgsTypes::Temporary]*ecct)/(aps[pgsTypes::Straight] + aps[pgsTypes::Harped] + aps[pgsTypes::Temporary]);
   }

   return ecc;
}

Float64 CBridgeAgentImp::GetEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, const GDRCONFIG& rconfig, pgsTypes::StrandType strandType,Float64* nEffectiveStrands)
{
   Float64 e;
   if ( IsOffSegment(poi) )
   {
      return 0.0;
   }

   switch( strandType )
   {
   case pgsTypes::Straight:
      e = GetSsEccentricity(spType,intervalIdx,poi,rconfig,nEffectiveStrands);
      break;

   case pgsTypes::Harped:
      e = GetHsEccentricity(spType,intervalIdx,poi,rconfig,nEffectiveStrands);
      break;

   case pgsTypes::Temporary:
      e = GetTempEccentricity(spType,intervalIdx,poi,rconfig,nEffectiveStrands);
      break;

   case pgsTypes::Permanent:
      {
       StrandIndexType Ns = rconfig.PrestressConfig.GetStrandCount(pgsTypes::Straight);
       StrandIndexType Nh = rconfig.PrestressConfig.GetStrandCount(pgsTypes::Harped);
       Float64 NsEffective, NhEffective;
       Float64 es = GetSsEccentricity(spType,intervalIdx,poi,rconfig,&NsEffective);
       Float64 eh = GetHsEccentricity(spType,intervalIdx,poi,rconfig,&NhEffective);
       e = (Ns+Nh == 0 ? 0 : (es*Ns + eh*Nh)/(Ns+Nh));
       *nEffectiveStrands = NsEffective + NhEffective;
      }
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return e;
}

Float64 CBridgeAgentImp::GetMaxStrandSlope(const pgsPointOfInterest& poi,StrandIndexType Nh,Float64 endShift,Float64 hpShift)
{
   if ( IsOffSegment(poi) )
   {
      return 0.0;
   }

   VALIDATE( GIRDER );

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CComPtr<IPrecastGirder> girder;
   GetGirder(poi,&girder);

   // use continuous interface to compute
   CComPtr<IIndexArray> fill;
   m_StrandFillers[segmentKey].ComputeHarpedStrandFill(girder, Nh, &fill);

   Float64 slope;
   girder->ComputeMaxHarpedStrandSlopeEx(poi.GetDistFromStart(),fill,endShift,hpShift,&slope);

   return slope;
}

Float64 CBridgeAgentImp::GetAvgStrandSlope(const pgsPointOfInterest& poi,StrandIndexType Nh,Float64 endShift,Float64 hpShift)
{
   if ( IsOffSegment(poi) )
   {
      return 0.0;
   }

   VALIDATE( GIRDER );

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CComPtr<IPrecastGirder> girder;
   GetGirder(poi,&girder);

   // use continuous interface to compute
   CComPtr<IIndexArray> fill;
   m_StrandFillers[segmentKey].ComputeHarpedStrandFill(girder, Nh, &fill);

   Float64 slope;
   girder->ComputeAvgHarpedStrandSlopeEx(poi.GetDistFromStart(),fill,endShift,hpShift,&slope);

   return slope;
}

Float64 CBridgeAgentImp::GetHsEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, Float64* nEffectiveStrands)
{
   ATLASSERT( spType != pgsTypes::sptNetDeck );

   VALIDATE( GIRDER );


   // if the strands aren't released yet, then there isn't an eccentricty with respect to the girder cross section
   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   IntervalIndexType releaseIntervalIdx = GetPrestressReleaseInterval(segmentKey);
   if ( intervalIdx < releaseIntervalIdx || IsOffSegment(poi) )
   {
      return 0;
   }

   CComPtr<IPrecastGirder> girder;
   GetGirder(poi,&girder);

   CComPtr<IPoint2dCollection> points;
   girder->get_HarpedStrandPositions(poi.GetDistFromStart(),&points);

   CollectionIndexType nStrandPoints;
   points->get_Count(&nStrandPoints);

   Float64 ecc = 0.0;

   if (nStrandPoints == 0)
   {
      *nEffectiveStrands = 0.0;
   }
   else
   {
      // must account for partial bonding if poi is near end of girder
      // NOTE: bond factor is 1.0 if at girder ends
      Float64 Xpoi = poi.GetDistFromStart();
      Float64 bond_factor = 1.0;

      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      GET_IFACE(IPretensionForce,pPrestress);
      Float64 xfer_length = pPrestress->GetXferLength(segmentKey,pgsTypes::Harped);

      if (0.0 < Xpoi && Xpoi < xfer_length)
      {
         bond_factor = Xpoi/xfer_length;
      }
      else
      {
         Float64 girder_length = GetSegmentLength(segmentKey);

         if ( (girder_length-xfer_length) < Xpoi && Xpoi < girder_length)
         {
            bond_factor = (girder_length-Xpoi)/xfer_length;
         }
      }

      *nEffectiveStrands = (Float64)nStrandPoints*bond_factor;

      if (0 < nStrandPoints)
      {
         Float64 cg = 0.0;
         for (CollectionIndexType strandPointIdx = 0; strandPointIdx < nStrandPoints; strandPointIdx++)
         {
            CComPtr<IPoint2d> point;
            points->get_Item(strandPointIdx,&point);
            Float64 y;
            point->get_Y(&y);

            cg += y;
         }

         cg = cg / (Float64)nStrandPoints;

         Float64 Yb = GetY(spType,intervalIdx,poi,pgsTypes::BottomGirder);
         Float64 Hg = GetHg(spType,intervalIdx,poi);
         ecc = Yb - (Hg+cg);
      }
   }
   
   return ecc;
}

Float64 CBridgeAgentImp::GetSsEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, Float64* nEffectiveStrands)
{
   ATLASSERT(spType != pgsTypes::sptNetDeck);

   VALIDATE( GIRDER );

   // if the strands aren't released yet, then there isn't an eccentricty with respect to the girder cross section
   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   IntervalIndexType releaseIntervalIdx = GetPrestressReleaseInterval(segmentKey);
   if ( intervalIdx < releaseIntervalIdx || IsOffSegment(poi) )
   {
      return 0;
   }

   CComPtr<IPrecastGirder> girder;
   GetGirder(poi,&girder);

   StrandIndexType num_strands;
   girder->GetStraightStrandCount(&num_strands);
   if (0 < num_strands)
   {
      StrandIndexType num_bonded_strands = 0;
      Float64 num_eff_strands = 0.0; // weighted number of effective strands

      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      Float64 segment_length = GetSegmentLength(segmentKey);

      Float64 Xpoi = poi.GetDistFromStart();

      // treat comp at ends different than in interior
      Float64 cg = 0.0;
      if (IsZero(Xpoi))
      {
         // we are at the girder left end. Treat all bonded strands as full strength
         for (StrandIndexType strandIndex = 0; strandIndex < num_strands; strandIndex++)
         {
            Float64 yloc, left_debond, right_debond;
            girder->GetStraightStrandDebondLengthByPositionIndex(etStart,strandIndex, &yloc, &left_debond,&right_debond);

            if (left_debond==0.0)
            {
               // strand is not debonded, we don't care by how much
               cg += yloc;
               num_bonded_strands++;
            }
         }

         num_eff_strands = (Float64)num_bonded_strands;
      }
      else if (IsEqual(Xpoi,segment_length))
      {
         // we are at the girder right end. Treat all bonded strands as full strength
         for (StrandIndexType strandIndex = 0; strandIndex < num_strands; strandIndex++)
         {
            Float64 yloc, left_debond, right_debond;
            girder->GetStraightStrandDebondLengthByPositionIndex(etEnd,strandIndex, &yloc, &left_debond,&right_debond);

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
         GET_IFACE(IPretensionForce,pPrestress);
         Float64 xfer_length = pPrestress->GetXferLength(segmentKey,pgsTypes::Straight);

         for (StrandIndexType strandIndex = 0; strandIndex < num_strands; strandIndex++)
         {
            Float64 yloc, left_bond, right_bond;
            girder->GetStraightStrandBondedLengthByPositionIndex(strandIndex, Xpoi, &yloc,&left_bond,&right_bond);

            Float64 bond_length = Min(left_bond, right_bond);
            
            if (bond_length <= 0.0) 
            {
               ;// do nothing if bond length is zero
            }
            else if (bond_length < xfer_length)
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

      if (0.0 < num_eff_strands)
      {
         cg = cg/num_eff_strands;

         Float64 Yb = GetY(spType,intervalIdx,poi,pgsTypes::BottomGirder);
         Float64 Hg = GetHg(spType,intervalIdx,poi);

         Float64 ecc = Yb - (Hg+cg);
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

Float64 CBridgeAgentImp::GetTempEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi, Float64* nEffectiveStrands)
{
   ATLASSERT(spType != pgsTypes::sptNetDeck);

   VALIDATE(BRIDGE);

   // if the strands aren't released yet, then there isn't an eccentricty with respect to the girder cross section
   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   IntervalIndexType releaseIntervalIdx = GetPrestressReleaseInterval(segmentKey);
   if ( intervalIdx < releaseIntervalIdx || IsOffSegment(poi) )
   {
      return 0;
   }

   CComPtr<IPrecastGirder> girder;
   GetGirder(poi,&girder);

   Float64 Xpoi = poi.GetDistFromStart();

   CComPtr<IPoint2dCollection> points;
   girder->get_TemporaryStrandPositions(Xpoi, &points);

   CollectionIndexType num_strand_positions;
   points->get_Count(&num_strand_positions);

   if (0 < num_strand_positions)
   {
      // must account for partial bonding if poi is near end of girder
      // NOTE: bond factor is 1.0 if at girder ends
      Float64 bond_factor = 1.0;

      GET_IFACE(IPretensionForce,pPrestress);
      Float64 xfer_length = pPrestress->GetXferLength(segmentKey,pgsTypes::Temporary);

      if (0.0 < Xpoi && Xpoi < xfer_length)
      {
         bond_factor = Xpoi/xfer_length;
      }
      else
      {
         Float64 girder_length = GetSegmentLength(segmentKey);

         if ( (girder_length-xfer_length) < Xpoi && Xpoi < girder_length)
         {
            bond_factor = (girder_length-Xpoi)/xfer_length;
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

      Float64 Yb = GetY(spType,intervalIdx,poi,pgsTypes::BottomGirder);
      Float64 Hg = GetHg(spType,intervalIdx,poi);

      Float64 ecc = Yb - (Hg+cg);
      return ecc;
   }
   else
   {
      *nEffectiveStrands = 0.0;
      return 0.0;
   }
}

Float64 CBridgeAgentImp::GetSuperstructureDepth(PierIndexType pierIdx)
{
   // The overall superstructure depth is taken to be the depth of the deepest
   // girder in the cross section plus the "A" dimension
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);
   Float64 Dmax = 0;
   if ( pPier->IsBoundaryPier() )
   {
      // There are girders on each side of the pier
      const CGirderGroupData* pBackGroup  = pPier->GetGirderGroup(pgsTypes::Back);
      GirderIndexType nGirders = pBackGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CSpanKey spanKey(pierIdx,gdrIdx);
         Float64 spanLength = GetSpanLength(spanKey);
         pgsPointOfInterest poi = ConvertSpanPointToPoi(spanKey,spanLength);
         Float64 Hg = GetHeight(poi);
         Float64 slabOffset = GetSlabOffset(poi);

         Dmax = Max(Dmax,Hg+slabOffset);
      }

      const CGirderGroupData* pAheadGroup = pPier->GetGirderGroup(pgsTypes::Ahead);
      nGirders = pAheadGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CSpanKey spanKey(pierIdx+1,gdrIdx);
         pgsPointOfInterest poi = ConvertSpanPointToPoi(spanKey,0.0);
         Float64 Hg = GetHeight(poi);
         Float64 slabOffset = GetSlabOffset(poi);

         Dmax = Max(Dmax,Hg+slabOffset);
      }
   }
   else
   {
      // Girders span over the pier
      GirderIndexType nGirders = GetGirderCountBySpan(pierIdx);
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CSpanKey spanKey(pierIdx,gdrIdx);
         Float64 spanLength = GetSpanLength(spanKey);
         pgsPointOfInterest poi = ConvertSpanPointToPoi(spanKey,spanLength);
         Float64 Hg = GetHeight(poi);
         Float64 slabOffset = GetSlabOffset(poi);

         Dmax = Max(Dmax,Hg+slabOffset);
      }
   }
   return Dmax;
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

ConfigStrandFillVector CBridgeAgentImp::ComputeStrandFill(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType Ns)
{
   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   CContinuousStrandFiller* pFiller = GetContinuousStrandFiller(segmentKey);

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

   // The position of the strands in the strand grid doesn't matter
   // for this method. Just use dummy values.
   Float64 Hg = 0;

   // Get fill in COM format
   CComPtr<IIndexArray> indexarr;
   switch( type )
   {
   case pgsTypes::Straight:
      {
         CComPtr<IStrandGrid> startGrid, endGrid;
         startGrid.CoCreateInstance(CLSID_StrandGrid);
         endGrid.CoCreateInstance(CLSID_StrandGrid);
         pGdrEntry->ConfigureStraightStrandGrid(Hg,Hg,startGrid,endGrid);

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
         pGdrEntry->ConfigureHarpedStrandGrids(Hg,Hg,Hg,Hg,startGrid,startHPGrid,endHPGrid,endGrid);

         CComQIPtr<IStrandGridFiller> endGridFiller(startGrid), hpGridFiller(startHPGrid);
         filler.ComputeHarpedStrandFill(pGdrEntry->OddNumberOfHarpedStrands(),endGridFiller, hpGridFiller, Ns, &indexarr);
      }
      break;

   case pgsTypes::Temporary:
      {
         CComPtr<IStrandGrid> startGrid, endGrid;
         startGrid.CoCreateInstance(CLSID_StrandGrid);
         endGrid.CoCreateInstance(CLSID_StrandGrid);
         pGdrEntry->ConfigureTemporaryStrandGrid(Hg,Hg,startGrid,endGrid);

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

   // The position of the strands in the strand grid doesn't matter
   // for this method. Just use dummy values.
   Float64 Hg = 0;

   // Get fill in COM format
   StrandIndexType gridIdx(INVALID_INDEX);
   switch( type )
   {
   case pgsTypes::Straight:
      {
         CComPtr<IStrandGrid> startGrid, endGrid;
         startGrid.CoCreateInstance(CLSID_StrandGrid);
         endGrid.CoCreateInstance(CLSID_StrandGrid);
         pGdrEntry->ConfigureStraightStrandGrid(Hg,Hg,startGrid,endGrid);

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
         pGdrEntry->ConfigureHarpedStrandGrids(Hg,Hg,Hg,Hg,startGrid,startHPGrid,endHPGrid,endGrid);

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
         pGdrEntry->ConfigureTemporaryStrandGrid(Hg,Hg,startGrid,endGrid);

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

   // The position of the strands in the strand grid doesn't matter
   // for this method. Just use dummy values.
   Float64 Hg = 0;

   // Get fill in COM format
   HRESULT hr;
   switch( type )
   {
   case pgsTypes::Straight:
      {
         CComPtr<IStrandGrid> startGrid, endGrid;
         startGrid.CoCreateInstance(CLSID_StrandGrid);
         endGrid.CoCreateInstance(CLSID_StrandGrid);
         pGdrEntry->ConfigureStraightStrandGrid(Hg,Hg,startGrid,endGrid);

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
         pGdrEntry->ConfigureHarpedStrandGrids(Hg,Hg,Hg,Hg,startGrid,startHPGrid,endHPGrid,endGrid);

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
         pGdrEntry->ConfigureTemporaryStrandGrid(Hg,Hg,startGrid,endGrid);

         CComQIPtr<IStrandGridFiller> gridFiller(startGrid);
         hr = gridFiller->GridIndexToStrandIndex(gridIdx, pStrandNo1, pStrandNo2);
         ATLASSERT(SUCCEEDED(hr));
      }
      break;

   default:
      ATLASSERT(false); // should never get here
   }
}


StrandIndexType CBridgeAgentImp::GetStrandCount(const CSegmentKey& segmentKey,pgsTypes::StrandType type)
{
   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

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
   {
      ATLASSERT(false);
   }

   return nStrands;
}

StrandIndexType CBridgeAgentImp::GetMaxStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type)
{
   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

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
   {
      return 0;
   }

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

Float64 CBridgeAgentImp::GetStrandArea(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::StrandType strandType)
{
   if ( intervalIdx < GetStressStrandInterval(segmentKey) )
   {
      return 0; // strands aren't stressed so they aren't even in play yet
   }

   StrandIndexType Ns = GetStrandCount(segmentKey,strandType);
   const matPsStrand* pStrand = GetStrandMaterial(segmentKey,strandType);

   Float64 aps = pStrand->GetNominalArea();
   Float64 Aps = aps * Ns;
   return Aps;
}

Float64 CBridgeAgentImp::GetStrandArea(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::StrandType strandType)
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   if ( intervalIdx < GetStressStrandInterval(segmentKey) )
   {
      return 0; // strands aren't stressed so they aren't even in play yet
   }

   const matPsStrand* pStrand = GetStrandMaterial(segmentKey,strandType);
   Float64 aps = pStrand->GetNominalArea();

   // count the number of strands that are debonded
   StrandIndexType nDebonded = 0;
   StrandIndexType nStrands = GetStrandCount(segmentKey,strandType);
   for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
   {
      if ( IsStrandDebonded(poi,strandIdx,strandType) )
      {
         nDebonded++;
      }
   }

   Float64 Aps = aps * (nStrands - nDebonded);
   return Aps;
}

Float64 CBridgeAgentImp::GetAreaPrestressStrands(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,bool bIncTemp)
{
   Float64 Aps = GetStrandArea(segmentKey,intervalIdx,pgsTypes::Straight) + GetStrandArea(segmentKey,intervalIdx,pgsTypes::Harped);
   if ( bIncTemp )
   {
      Aps += GetStrandArea(segmentKey,intervalIdx,pgsTypes::Temporary);
   }

   return Aps;
}

Float64 CBridgeAgentImp::GetPjack(const CSegmentKey& segmentKey,pgsTypes::StrandType type)
{
   GET_IFACE(IBridgeDescription,pBridgeDesc);
   const CPrecastSegmentData* pSegment = pBridgeDesc->GetPrecastSegmentData(segmentKey);
   if ( type == pgsTypes::Permanent )
   {
      return pSegment->Strands.GetPjack(pgsTypes::Straight) + pSegment->Strands.GetPjack(pgsTypes::Harped);
   }
   else
   {
      return pSegment->Strands.GetPjack(type);
   }
}

Float64 CBridgeAgentImp::GetPjack(const CSegmentKey& segmentKey,bool bIncTemp)
{
   Float64 Pj = GetPjack(segmentKey,pgsTypes::Straight) + GetPjack(segmentKey,pgsTypes::Harped);
   if ( bIncTemp )
   {
      Pj += GetPjack(segmentKey,pgsTypes::Temporary);
   }

   return Pj;
}

bool CBridgeAgentImp::GetAreHarpedStrandsForcedStraight(const CSegmentKey& segmentKey)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);
   return pSegment->Strands.GetAdjustableStrandType() == pgsTypes::asStraight ? true : false;
}

void CBridgeAgentImp::GetHarpedStrandControlHeights(const CSegmentKey& segmentKey,Float64* pHgStart,Float64* pHgHp1,Float64* pHgHp2,Float64* pHgEnd)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);

   std::vector<pgsPointOfInterest> vPoi;
   vPoi = GetPointsOfInterest(segmentKey,POI_RELEASED_SEGMENT | POI_0L);
   ATLASSERT(vPoi.size() == 1);
   pgsPointOfInterest poiStart(vPoi.front());

   vPoi = GetPointsOfInterest(segmentKey,POI_RELEASED_SEGMENT | POI_10L);
   ATLASSERT(vPoi.size() == 1);
   pgsPointOfInterest poiEnd(vPoi.front());

   vPoi = GetPointsOfInterest(segmentKey,POI_HARPINGPOINT);
   pgsPointOfInterest poiHp1, poiHp2;
   if ( 0 < vPoi.size() )
   {
      ATLASSERT( vPoi.size() < 3 );
      poiHp1 = vPoi.front();
      poiHp2 = vPoi.back();
   }
   else
   {
      // no harp points
      poiHp1 = poiStart;
      poiHp2 = poiEnd;
   }

   *pHgStart = GetHeight(poiStart);
   *pHgHp1   = GetHeight(poiHp1);
   *pHgHp2   = GetHeight(poiHp2);
   *pHgEnd   = GetHeight(poiEnd);
}

Float64 CBridgeAgentImp::GetGirderTopElevation(const CSegmentKey& segmentKey)
{
   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   Float64 top;
   girder->get_TopElevation(&top);

   return top;
}

Float64 CBridgeAgentImp::GetSplittingZoneHeight(const pgsPointOfInterest& poi)
{
   CComPtr<IGirderSection> girder_section;
   GetGirderSection(poi,pgsTypes::scBridge,&girder_section);

   CComQIPtr<IPrestressedGirderSection> psg_section(girder_section);

   Float64 splitting_zone_height;
   psg_section->get_SplittingZoneDimension(&splitting_zone_height);

   return splitting_zone_height;
}

pgsTypes::SplittingDirection CBridgeAgentImp::GetSplittingDirection(const CGirderKey& girderKey)
{
   CSegmentKey segmentKey(girderKey,0);

   CComPtr<IGirderSection> girder_section;
   pgsPointOfInterest poi(segmentKey,0.00);
   GetGirderSection(poi,pgsTypes::scBridge,&girder_section);

   CComQIPtr<IPrestressedGirderSection> psg_section(girder_section);
   SplittingDirection splitDir;
   psg_section->get_SplittingDirection(&splitDir);

   return (splitDir == sdVertical ? pgsTypes::sdVertical : pgsTypes::sdHorizontal);
}

void CBridgeAgentImp::GetHarpStrandOffsets(const CSegmentKey& segmentKey,Float64* pOffsetEnd,Float64* pOffsetHp)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);
   girder->get_HarpedStrandAdjustmentEnd(pOffsetEnd);
   girder->get_HarpedStrandAdjustmentHP(pOffsetHp);
}

void CBridgeAgentImp::GetHarpedEndOffsetBounds(const CSegmentKey& segmentKey,Float64* DownwardOffset, Float64* UpwardOffset)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   HRESULT hr = girder->GetHarpedEndAdjustmentBounds(DownwardOffset, UpwardOffset);
   ATLASSERT(SUCCEEDED(hr));
}

void CBridgeAgentImp::GetHarpedEndOffsetBoundsEx(const CSegmentKey& segmentKey,StrandIndexType Nh, Float64* DownwardOffset, Float64* UpwardOffset)
{
   VALIDATE( GIRDER );

   if (Nh == 0)
   {
      *DownwardOffset = 0.0;
      *UpwardOffset = 0.0;
   }
   else
   {
      CComPtr<IPrecastGirder> girder;
      GetGirder(segmentKey,&girder);

      // use continuous interface to compute
      CComPtr<IIndexArray> fill;
      m_StrandFillers[segmentKey].ComputeHarpedStrandFill(girder, Nh, &fill);

      HRESULT hr = girder->GetHarpedEndAdjustmentBoundsEx(fill,DownwardOffset, UpwardOffset);
      ATLASSERT(SUCCEEDED(hr));
   }
}
void CBridgeAgentImp::GetHarpedEndOffsetBoundsEx(LPCTSTR strGirderName, pgsTypes::AdjustableStrandType adjType, Float64 HgStart, Float64 HgHp1, Float64 HgHp2, Float64 HgEnd, const ConfigStrandFillVector& rHarpedFillArray, Float64* DownwardOffset, Float64* UpwardOffset)
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
   pGdrEntry->ConfigureHarpedStrandGrids(HgStart,HgHp1,HgHp2,HgEnd,startGrid,startHPGrid,endHPGrid,endGrid);

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
      CreateStrandMover(strGirderName,adjType,&strandMover);
      hr = strandMover->get_EndStrandElevationBoundaries(&bottom_min, &top_max);
      ATLASSERT(SUCCEEDED(hr));

      *DownwardOffset = bottom_min - bottom;
      *DownwardOffset = IsZero(*DownwardOffset) ? 0.0 : *DownwardOffset;

      *UpwardOffset   = top_max - top;
      *UpwardOffset   = IsZero(*UpwardOffset)   ? 0.0 : *UpwardOffset;

      // if these fire, strands cannot be adjusted within section bounds. this should be caught at library entry time.
      ATLASSERT(*DownwardOffset < 1.0e-06);
      ATLASSERT(*UpwardOffset > -1.0e-06);
   }
}

void CBridgeAgentImp::GetHarpedHpOffsetBounds(const CSegmentKey& segmentKey,Float64* DownwardOffset, Float64* UpwardOffset)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   HRESULT hr = girder->GetHarpedHpAdjustmentBounds(DownwardOffset, UpwardOffset);
   ATLASSERT(SUCCEEDED(hr));
}

void CBridgeAgentImp::GetHarpedHpOffsetBoundsEx(const CSegmentKey& segmentKey,StrandIndexType Nh, Float64* DownwardOffset, Float64* UpwardOffset)
{
   VALIDATE( GIRDER );

   if (Nh == 0)
   {
      *DownwardOffset = 0.0;
      *UpwardOffset = 0.0;
   }
   else
   {
      CComPtr<IPrecastGirder> girder;
      GetGirder(segmentKey,&girder);

      // use continuous interface to compute
      CComPtr<IIndexArray> fill;
      m_StrandFillers[segmentKey].ComputeHarpedStrandFill(girder, Nh, &fill);

      HRESULT hr = girder->GetHarpedHpAdjustmentBoundsEx(fill,DownwardOffset, UpwardOffset);
      ATLASSERT(SUCCEEDED(hr));
   }
}

void CBridgeAgentImp::GetHarpedHpOffsetBoundsEx(LPCTSTR strGirderName, pgsTypes::AdjustableStrandType adjType, Float64 HgStart, Float64 HgHp1, Float64 HgHp2, Float64 HgEnd, const ConfigStrandFillVector& rHarpedFillArray, Float64* DownwardOffset, Float64* UpwardOffset)
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
   pGdrEntry->ConfigureHarpedStrandGrids(HgStart,HgHp1,HgHp2,HgEnd,startGrid,startHPGrid,endHPGrid,endGrid);

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
      CreateStrandMover(strGirderName,adjType,&strandMover);
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

Float64 CBridgeAgentImp::GetHarpedEndOffsetIncrement(const CSegmentKey& segmentKey)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   Float64 increment;

   HRESULT hr = girder->get_HarpedEndAdjustmentIncrement(&increment);
   ATLASSERT(SUCCEEDED(hr));

   return increment;
}

Float64 CBridgeAgentImp::GetHarpedEndOffsetIncrement(LPCTSTR strGirderName, pgsTypes::AdjustableStrandType adjType )
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGirderEntry = pLib->GetGirderEntry(strGirderName);
   Float64 end_increment;
   if (adjType == pgsTypes::asStraight)
   {
      end_increment  = pGirderEntry->IsVerticalAdjustmentAllowedStraight()  ?  pGirderEntry->GetStraightStrandIncrement()  : -1.0;
   }
   else
   {
      end_increment  =pGirderEntry->IsVerticalAdjustmentAllowedEnd() ?  pGirderEntry->GetEndStrandIncrement() : -1.0;
   }

   return end_increment;
}

Float64 CBridgeAgentImp::GetHarpedHpOffsetIncrement(const CSegmentKey& segmentKey)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   Float64 increment;

   HRESULT hr = girder->get_HarpedHpAdjustmentIncrement(&increment);
   ATLASSERT(SUCCEEDED(hr));

   return increment;
}

Float64 CBridgeAgentImp::GetHarpedHpOffsetIncrement(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGirderEntry = pLib->GetGirderEntry(strGirderName);
   Float64 hp_increment;
   if (adjType == pgsTypes::asStraight)
   {
      hp_increment  = pGirderEntry->IsVerticalAdjustmentAllowedStraight()  ?  pGirderEntry->GetStraightStrandIncrement()  : -1.0;
   }
   else
   {
      hp_increment = pGirderEntry->IsVerticalAdjustmentAllowedHP()  ?  pGirderEntry->GetHPStrandIncrement()  : -1.0;
   }

   return hp_increment;
}

void CBridgeAgentImp::GetHarpingPointLocations(const CSegmentKey& segmentKey,Float64* lhp,Float64* rhp)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   girder->GetHarpingPointLocations(lhp,rhp);
}

void CBridgeAgentImp::GetHarpingPointLocations(const CSegmentKey& segmentKey,Float64* pX1,Float64* pX2,Float64* pX3,Float64* pX4)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   girder->GetHarpingPointLocations(pX2,pX3);
   girder->GetEndHarpingPointLocations(pX1,pX4);
}

void CBridgeAgentImp::GetHighestHarpedStrandLocationEnds(const CSegmentKey& segmentKey,Float64* pElevation)
{
   // determine distance from bottom of girder to highest harped strand at end of girder 
   // to compute the txdot ibns TO value
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

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

   *pElevation = Max(startTop,endTop);
}

void CBridgeAgentImp::GetHighestHarpedStrandLocationHPs(const CSegmentKey& segmentKey,Float64* pElevation)
{
   // determine distance from bottom of girder to highest harped strand at end of girder 
   // to compute the txdot ibns TO CL value
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   CComPtr<IRect2d> boxStart;
   HRESULT hr = girder->HarpedHpStrandBoundingBox(etStart,&boxStart);
   ATLASSERT(SUCCEEDED(hr));

   Float64 startTop;
   boxStart->get_Top(&startTop);

   CComPtr<IRect2d> boxEnd;
   hr = girder->HarpedHpStrandBoundingBox(etEnd,&boxEnd);
   ATLASSERT(SUCCEEDED(hr));

   Float64 endTop;
   boxEnd->get_Top(&endTop);

   *pElevation = Max(startTop,endTop);
}

IndexType CBridgeAgentImp::GetNumHarpPoints(const CSegmentKey& segmentKey)
{
   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   StrandIndexType maxHarped;
   girder->get_MaxHarpedStrands(&maxHarped);

   if(maxHarped == 0)
   {
      return 0;
   }

   Float64 lhp, rhp;
   GetHarpingPointLocations(segmentKey, &lhp, &rhp);

   if (IsEqual(lhp,rhp))
   {
      return 1;
   }
   else
   {
      return 2;
   }
}


StrandIndexType CBridgeAgentImp::GetNextNumStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType curNum)
{
   StrandIndexType ns;
   switch(type)
   {
   case pgsTypes::Straight:
      ns = GetNextNumStraightStrands(segmentKey,curNum);
      break;

   case pgsTypes::Harped:
      ns = GetNextNumHarpedStrands(segmentKey,curNum);
      break;

   case pgsTypes::Temporary:
      ns = GetNextNumTempStrands(segmentKey,curNum);
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


StrandIndexType CBridgeAgentImp::GetPrevNumStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType curNum)
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
         ns = GetPrevNumStraightStrands(segmentKey,curNum);
         break;

      case pgsTypes::Harped:
         ns = GetPrevNumHarpedStrands(segmentKey,curNum);
         break;

      case pgsTypes::Temporary:
         ns = GetPrevNumTempStrands(segmentKey,curNum);
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
   {
      return 0;
   }

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

StrandIndexType CBridgeAgentImp::GetNumExtendedStrands(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType)
{
   GDRCONFIG config = GetSegmentConfiguration(segmentKey);
   return config.PrestressConfig.GetExtendedStrands(strandType,endType).size();
}

bool CBridgeAgentImp::IsExtendedStrand(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end,StrandIndexType strandIdx,pgsTypes::StrandType strandType)
{
   GDRCONFIG config = GetSegmentConfiguration(segmentKey);
   return IsExtendedStrand(segmentKey,end,strandIdx,strandType,config);
}

bool CBridgeAgentImp::IsExtendedStrand(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG& config)
{
   if ( config.PrestressConfig.GetExtendedStrands(strandType,endType).size() == 0 )
   {
      return false;
   }

   const std::vector<StrandIndexType>& extStrands = config.PrestressConfig.GetExtendedStrands(strandType,endType);
   std::vector<StrandIndexType>::const_iterator iter(extStrands.begin());
   std::vector<StrandIndexType>::const_iterator endIter(extStrands.end());
   for ( ; iter != endIter; iter++ )
   {
      if (*iter == strandIdx)
      {
         return true;
      }
   }

   return false;
}

bool CBridgeAgentImp::IsExtendedStrand(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType)
{
   Float64 Lg = GetSegmentLength(poi.GetSegmentKey());
   pgsTypes::MemberEndType end = (poi.GetDistFromStart() < Lg/2 ? pgsTypes::metStart : pgsTypes::metEnd);
   return IsExtendedStrand(poi.GetSegmentKey(),end,strandIdx,strandType);
}

bool CBridgeAgentImp::IsExtendedStrand(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG& config)
{
   Float64 Lg = GetSegmentLength(poi.GetSegmentKey());
   pgsTypes::MemberEndType end = (poi.GetDistFromStart() < Lg/2 ? pgsTypes::metStart : pgsTypes::metEnd);
   return IsExtendedStrand(poi.GetSegmentKey(),end,strandIdx,strandType,config);
}

bool CBridgeAgentImp::IsStrandDebonded(const CSegmentKey& segmentKey,StrandIndexType strandIdx,pgsTypes::StrandType strandType,Float64* pStart,Float64* pEnd)
{
   GDRCONFIG config = GetSegmentConfiguration(segmentKey);
   return IsStrandDebonded(segmentKey,strandIdx,strandType,config,pStart,pEnd);
}

bool CBridgeAgentImp::IsStrandDebonded(const CSegmentKey& segmentKey,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG& config,Float64* pStart,Float64* pEnd)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   Float64 length = GetSegmentLength(segmentKey);

   bool bDebonded = false;
   *pStart = 0.0;
   *pEnd   = length;

   DebondConfigConstIterator iter;
   for ( iter = config.PrestressConfig.Debond[strandType].begin(); iter != config.PrestressConfig.Debond[strandType].end(); iter++ )
   {
      const DEBONDCONFIG& di = *iter;
      if ( strandIdx == di.strandIdx )
      {
         *pStart = di.DebondLength[pgsTypes::metStart]; 
         *pEnd   = length - di.DebondLength[pgsTypes::metEnd];
         bDebonded = true;
         break;
      }
   }

   if ( bDebonded && (length < *pStart) )
   {
      *pStart = length;
   }

   if ( bDebonded && (length < *pEnd) )
   {
      *pEnd = length;
   }

   // if not debonded, bond starts at 0 and ends at the other end of the girder
   if ( !bDebonded )
   {
      *pStart = 0;
      *pEnd = length;
   }

   return bDebonded;
}

//-----------------------------------------------------------------------------
bool CBridgeAgentImp::IsStrandDebonded(const pgsPointOfInterest& poi,
                                       StrandIndexType strandIdx,
                                       pgsTypes::StrandType strandType)
{
   VALIDATE( GIRDER );

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   Float64 debond_left, debond_right;
   if ( !IsStrandDebonded(segmentKey,strandIdx,strandType,&debond_left,&debond_right) )
   {
      return false;
   }

   Float64 location = poi.GetDistFromStart();

   if ( location <= debond_left || debond_right <= location )
   {
      return true;
   }

   return false;
}

//-----------------------------------------------------------------------------
StrandIndexType CBridgeAgentImp::GetNumDebondedStrands(const CSegmentKey& segmentKey,
                                                       pgsTypes::StrandType strandType)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

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
RowIndexType CBridgeAgentImp::GetNumRowsWithStrand(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType )
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   StrandIndexType nStrands = GetStrandCount(segmentKey,strandType);
   return GetNumRowsWithStrand(poi,nStrands,strandType);
}

//-----------------------------------------------------------------------------
StrandIndexType CBridgeAgentImp::GetNumStrandInRow(const pgsPointOfInterest& poi,
                                                   RowIndexType rowIdx,
                                                   pgsTypes::StrandType strandType )
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   StrandIndexType nStrands = GetStrandCount(segmentKey,strandType);
   return GetNumStrandInRow(poi,nStrands,rowIdx,strandType);
}

std::vector<StrandIndexType> CBridgeAgentImp::GetStrandsInRow(const pgsPointOfInterest& poi, RowIndexType rowIdx, pgsTypes::StrandType strandType )
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   StrandIndexType nStrands = GetStrandCount(segmentKey,strandType);
   return GetStrandsInRow(poi,nStrands,rowIdx,strandType);
}

//-----------------------------------------------------------------------------
StrandIndexType CBridgeAgentImp::GetNumDebondedStrandsInRow(const CSegmentKey& segmentKey,
                                                            RowIndexType rowIdx,
                                                            pgsTypes::StrandType strandType )
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

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
bool CBridgeAgentImp::IsExteriorStrandDebondedInRow(const CSegmentKey& segmentKey,
                                                    RowIndexType rowIdx,
                                                    pgsTypes::StrandType strandType)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

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
Float64 CBridgeAgentImp::GetDebondSection(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,SectionIndexType sectionIdx,pgsTypes::StrandType strandType)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

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
   if ( endType == pgsTypes::metStart )
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
      Float64 gdr_length = GetSegmentLength(segmentKey);
      location = gdr_length - location;
   }

   return location;
}

//-----------------------------------------------------------------------------
SectionIndexType CBridgeAgentImp::GetNumDebondSections(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

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
   if ( endType == pgsTypes::metStart )
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
StrandIndexType CBridgeAgentImp::GetNumDebondedStrandsAtSection(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,SectionIndexType sectionIdx,pgsTypes::StrandType strandType)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

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
   if (endType == pgsTypes::metStart )
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
StrandIndexType CBridgeAgentImp::GetNumBondedStrandsAtSection(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,SectionIndexType sectionIdx,pgsTypes::StrandType strandType)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   StrandIndexType nStrands = GetStrandCount(segmentKey,strandType);

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
   if (endType == pgsTypes::metStart )
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

std::vector<StrandIndexType> CBridgeAgentImp::GetDebondedStrandsAtSection(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,SectionIndexType sectionIdx,pgsTypes::StrandType strandType)
{
   VALIDATE( GIRDER );

   std::vector<StrandIndexType> debondedStrands;

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   switch( strandType )
   {
   case pgsTypes::Harped:
   case pgsTypes::Temporary:
                    // Assumed only bonded for the end 10'... PS force is constant through the debonded section
                    // this is different than strands debonded at the ends and bonded in the middle
                    // Treat this strand as bonded
      return debondedStrands;
      break;

   case pgsTypes::Straight:
      break; // fall through to below


   default:
      ATLASSERT(false); // should never get here
      return debondedStrands;
   }

   HRESULT hr;
   CollectionIndexType nStrands;
   CComPtr<IIndexArray> strands;
   if (endType == pgsTypes::metStart)
   {
      // left
      hr = girder->GetStraightStrandDebondAtLeftSection(sectionIdx,&strands);
   }
   else
   {
      hr = girder->GetStraightStrandDebondAtRightSection(sectionIdx,&strands);
   }
   ATLASSERT(SUCCEEDED(hr));
   strands->get_Count(&nStrands);

   for ( IndexType idx = 0; idx < nStrands; idx++ )
   {
      StrandIndexType strandIdx;
      strands->get_Item(idx,&strandIdx);
      debondedStrands.push_back(strandIdx);
   }

   return debondedStrands;
}

//-----------------------------------------------------------------------------
bool CBridgeAgentImp::CanDebondStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType)
{
   VALIDATE( BRIDGE );

   if ( strandType == pgsTypes::Harped || strandType == pgsTypes::Temporary )
   {
      return false;
   }

   const GirderLibraryEntry* pGirderEntry = GetGirderLibraryEntry(segmentKey);

   return pGirderEntry->CanDebondStraightStrands();
}

bool CBridgeAgentImp::CanDebondStrands(LPCTSTR strGirderName,pgsTypes::StrandType strandType)
{
   if ( strandType == pgsTypes::Harped || strandType == pgsTypes::Temporary )
   {
      return false;
   }

   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGirderEntry = pLib->GetGirderEntry( strGirderName );

   return pGirderEntry->CanDebondStraightStrands();
}

//-----------------------------------------------------------------------------
void CBridgeAgentImp::ListDebondableStrands(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rFillArray,pgsTypes::StrandType strandType, IIndexArray** list)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData*    pGroup      = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData*  pGirder     = pGroup->GetGirder(segmentKey.girderIndex);

   ListDebondableStrands(pGirder->GetGirderName(),rFillArray,strandType,list);
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
Float64 CBridgeAgentImp::GetDefaultDebondLength(const CSegmentKey& segmentKey)
{
   const GirderLibraryEntry* pGirderEntry = GetGirderLibraryEntry(segmentKey);
   return pGirderEntry->GetDefaultDebondSectionLength();
}

//-----------------------------------------------------------------------------
bool CBridgeAgentImp::IsDebondingSymmetric(const CSegmentKey& segmentKey)
{
   StrandIndexType Ns = GetNumDebondedStrands(segmentKey,pgsTypes::Straight);
   StrandIndexType Nh = GetNumDebondedStrands(segmentKey,pgsTypes::Harped);
   StrandIndexType Nt = GetNumDebondedStrands(segmentKey,pgsTypes::Temporary);

   // if there are no debonded strands then get the heck outta here
   if ( Ns == 0 && Nh == 0 && Nt == 0)
   {
      return true;
   }

   Float64 length = GetSegmentLength(segmentKey);

   // check the debonding to see if it is symmetric
   for ( int i = 0; i < 3; i++ )
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)(i);
      StrandIndexType nDebonded = GetNumDebondedStrands(segmentKey,strandType);
      if ( nDebonded == 0 )
      {
         continue;
      }

      StrandIndexType n = GetStrandCount(segmentKey,strandType);
      for ( StrandIndexType strandIdx = 0; strandIdx < n; strandIdx++ )
      {
         Float64 start, end;

         bool bIsDebonded = IsStrandDebonded(segmentKey,strandIdx,strandType,&start,&end);
         if ( bIsDebonded && !IsEqual(start,length-end) )
         {
            return false;
         }
      }
   }

   return true;
}

RowIndexType CBridgeAgentImp::GetNumRowsWithStrand(const pgsPointOfInterest& poi,StrandIndexType nStrands,pgsTypes::StrandType strandType )
{
   VALIDATE( GIRDER );

   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   Float64 Xpoi = poi.GetDistFromStart();

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   CComPtr<IIndexArray> oldFill;
   CComPtr<IIndexArray> fill;

   RowIndexType nRows = 0;
   HRESULT hr;
   switch( strandType )
   {
   case pgsTypes::Straight:
      m_StrandFillers[segmentKey].ComputeStraightStrandFill(girder, nStrands, &fill);
      girder->get_StraightStrandFill(&oldFill);
      girder->putref_StraightStrandFill(fill);
      hr = girder->get_StraightStrandRowsWithStrand(&nRows);
      girder->putref_StraightStrandFill(oldFill);
      break;

   case pgsTypes::Harped:
      m_StrandFillers[segmentKey].ComputeHarpedStrandFill(girder, nStrands, &fill);
      girder->get_HarpedStrandFill(&oldFill);
      girder->putref_HarpedStrandFill(fill);
      hr = girder->get_HarpedStrandRowsWithStrand(Xpoi,&nRows);
      girder->putref_HarpedStrandFill(oldFill);
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

StrandIndexType CBridgeAgentImp::GetNumStrandInRow(const pgsPointOfInterest& poi,StrandIndexType nStrands,RowIndexType rowIdx,pgsTypes::StrandType strandType )
{
   VALIDATE( GIRDER );

   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   Float64 Xpoi = poi.GetDistFromStart();

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   CComPtr<IIndexArray> oldFill;
   CComPtr<IIndexArray> fill;

   StrandIndexType cStrands = 0;
   HRESULT hr;
   switch( strandType )
   {
   case pgsTypes::Straight:
      m_StrandFillers[segmentKey].ComputeStraightStrandFill(girder, nStrands, &fill);
      girder->get_StraightStrandFill(&oldFill);
      girder->putref_StraightStrandFill(fill);
      hr = girder->get_NumStraightStrandsInRow(rowIdx,&cStrands);
      girder->putref_StraightStrandFill(oldFill);
      break;

   case pgsTypes::Harped:
      m_StrandFillers[segmentKey].ComputeHarpedStrandFill(girder, nStrands, &fill);
      girder->get_HarpedStrandFill(&oldFill);
      girder->putref_HarpedStrandFill(fill);
      hr = girder->get_NumHarpedStrandsInRow(Xpoi,rowIdx,&cStrands);
      girder->putref_HarpedStrandFill(oldFill);
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

std::vector<StrandIndexType> CBridgeAgentImp::GetStrandsInRow(const pgsPointOfInterest& poi,StrandIndexType nStrands,RowIndexType rowIdx, pgsTypes::StrandType strandType )
{
   std::vector<StrandIndexType> strandIdxs;
   if ( strandType == pgsTypes::Temporary )
   {
      ATLASSERT(false); // shouldn't get here
      return strandIdxs;
   }

   VALIDATE( GIRDER );

   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   Float64 Xpoi = poi.GetDistFromStart();

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   CComPtr<IIndexArray> oldFill;
   CComPtr<IIndexArray> fill;
   CComPtr<IIndexArray> array;

   if ( strandType == pgsTypes::Straight )
   {
      m_StrandFillers[segmentKey].ComputeStraightStrandFill(girder, nStrands, &fill);
      girder->get_StraightStrandFill(&oldFill);
      girder->putref_StraightStrandFill(fill);
      girder->get_StraightStrandsInRow(rowIdx,&array);
      girder->putref_StraightStrandFill(oldFill);
   }
   else
   {
      m_StrandFillers[segmentKey].ComputeHarpedStrandFill(girder, nStrands, &fill);
      girder->get_HarpedStrandFill(&oldFill);
      girder->putref_HarpedStrandFill(fill);
      girder->get_HarpedStrandsInRow(Xpoi,rowIdx,&array);
      girder->putref_HarpedStrandFill(oldFill);
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
void CBridgeAgentImp::GetStrandPosition(const pgsPointOfInterest& poi, StrandIndexType strandIdx,pgsTypes::StrandType type, IPoint2d** ppPoint)
{
   CComPtr<IPoint2dCollection> points;
   GetStrandPositions(poi,type,&points);
   points->get_Item(strandIdx,ppPoint);
}

void CBridgeAgentImp::GetStrandPositions(const pgsPointOfInterest& poi, pgsTypes::StrandType type, IPoint2dCollection** ppPoints)
{
   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(poi,&girder);

   Float64 Xpoi = poi.GetDistFromStart();
   Float64 Ls = GetSegmentLength(poi.GetSegmentKey());
   Float64 Xg = ::ForceIntoRange(0.0,Xpoi,Ls);

   HRESULT hr = S_OK;
   switch( type )
   {
   case pgsTypes::Straight:
      hr = girder->get_StraightStrandPositions(Xg,ppPoints);
      break;

   case pgsTypes::Harped:
      hr = girder->get_HarpedStrandPositions(Xg,ppPoints);
      break;

   case pgsTypes::Temporary:
      hr = girder->get_TemporaryStrandPositions(Xg,ppPoints);
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
   GetGirder(poi,&girder);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   Float64 Xpoi = poi.GetDistFromStart();
   Float64 Ls = GetSegmentLength(segmentKey);
   Float64 Xg = ::ForceIntoRange(0.0,Xpoi,Ls);

   CIndexArrayWrapper fill(rconfig.GetStrandFill(type));

   HRESULT hr = S_OK;
   switch( type )
   {
   case pgsTypes::Straight:
      hr = girder->get_StraightStrandPositionsEx(Xg,&fill,ppPoints);
      break;

   case pgsTypes::Harped:
      // Use CStrandMoverSwapper to temporarily swap out girder's strand mover and harping offset limits
      //  for design
      {
         GET_IFACE(IBridgeDescription,pIBridgeDesc);
         CStrandMoverSwapper swapper(segmentKey, rconfig, this, girder, pIBridgeDesc);

         hr = girder->get_HarpedStrandPositionsEx(Xg,&fill,ppPoints);
      }
      break;

   case pgsTypes::Temporary:
      hr = girder->get_TemporaryStrandPositionsEx(Xg,&fill,ppPoints);
      break;

   default:
      ATLASSERT(false); // shouldn't get here
   }

#ifdef _DEBUG
   CollectionIndexType np;
   (*ppPoints)->get_Count(&np);
   ATLASSERT(np==rconfig.GetStrandCount(type));
#endif

   ATLASSERT(SUCCEEDED(hr));
}

void CBridgeAgentImp::GetStrandPositionsEx(LPCTSTR strGirderName,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const PRESTRESSCONFIG& rconfig,
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
      pGdrEntry->ConfigureStraightStrandGrid(HgStart,HgEnd,startGrid,endGrid);
      gridFiller->GetStrandPositionsEx(&fill,ppPoints);
      break;

   case pgsTypes::Harped:
      {
      CComPtr<IStrandGrid> startHPGrid, endHPGrid;
      startHPGrid.CoCreateInstance(CLSID_StrandGrid);
      endHPGrid.CoCreateInstance(CLSID_StrandGrid);
      pGdrEntry->ConfigureHarpedStrandGrids(HgStart,HgHp1,HgHp2,HgEnd,startGrid,startHPGrid,endHPGrid,endGrid);
      CComQIPtr<IStrandGridFiller> hpGridFiller(startHPGrid);
      gridFiller->GetStrandPositionsEx(&fill,ppPoints);
      }
      break;

   case pgsTypes::Temporary:
      pGdrEntry->ConfigureTemporaryStrandGrid(HgStart,HgEnd,startGrid,endGrid);
      gridFiller->GetStrandPositionsEx(&fill,ppPoints);
      break;

   default:
      ATLASSERT(false); // is there a new strand type?
   }

   ATLASSERT(SUCCEEDED(hr));
}

Float64 CBridgeAgentImp::ComputeAbsoluteHarpedOffsetEnd(const CSegmentKey& segmentKey, const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset)
{
   // returns the offset value measured from the original strand locations defined in the girder library
   // Up is positive
   Float64 HgStart, HgHp1, HgHp2, HgEnd;
   GetHarpedStrandControlHeights(segmentKey,&HgStart,&HgHp1,&HgHp2,&HgEnd);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CSplicedGirderData* pGirder = pIBridgeDesc->GetGirder(segmentKey);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);

   pgsTypes::AdjustableStrandType adjType = pSegment->Strands.GetAdjustableStrandType(); 
   Float64 absOffset = ComputeAbsoluteHarpedOffsetEnd(pGirder->GetGirderName(),adjType,HgStart,HgHp1,HgHp2,HgEnd,rHarpedFillArray,measurementType,offset);

#if defined _DEBUG
   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey, &girder);

   Float64 increment; // if less than zero, strands cannot be adjusted
   girder->get_HarpedEndAdjustmentIncrement(&increment);

   Float64 result = 0;
   if (0.0 <= increment && AreStrandsInConfigFillVec(rHarpedFillArray))
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
            Float64 dist = -cg;
            result  = dist - offset;

         }
         else if ( measurementType==hsoCGFROMBOTTOM)
         {
            // top is a Y=0.0
            result =  offset - (HgHp1+cg);
         }
         else if (measurementType==hsoECCENTRICITY)
         {
            IntervalIndexType releaseIntervalIdx = GetPrestressReleaseInterval(segmentKey);
            Float64 Yb = GetY(releaseIntervalIdx,pgsPointOfInterest(segmentKey,0.0),pgsTypes::BottomGirder);
            Float64 ecc = Yb - (HgHp1+cg);

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
            result =  offset - (HgHp1+toploc);
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

         result =  offset - (HgHp1+botloc);
      }
      else
      {
         ATLASSERT(false);
      }
    }

   ATLASSERT(IsEqual(result,absOffset));
#endif // _DEBUG
   return absOffset;
}

Float64 CBridgeAgentImp::ComputeAbsoluteHarpedOffsetEnd(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset)
{
   // returns the offset value measured from the original strand locations defined in the girder library
   // Up is positive
   if ( !AreStrandsInConfigFillVec(rHarpedFillArray) )
   {
      return 0.0;
   }

   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   if ( adjType == pgsTypes::asHarped )
   {
      if (!pGdrEntry->IsVerticalAdjustmentAllowedEnd())
      {
         return 0.0; // No offset allowed
      }
   }
   else if (adjType==pgsTypes::asStraight && !pGdrEntry->IsVerticalAdjustmentAllowedStraight())
   {
      return 0.0; // No offset allowed
   }

   CComPtr<IStrandMover> pStrandMover;
   CreateStrandMover(strGirderName,adjType,&pStrandMover);

   CComPtr<IStrandGrid> startGrid, startHPGrid, endHPGrid, endGrid;
   startGrid.CoCreateInstance(CLSID_StrandGrid);
   startHPGrid.CoCreateInstance(CLSID_StrandGrid);
   endHPGrid.CoCreateInstance(CLSID_StrandGrid);
   endGrid.CoCreateInstance(CLSID_StrandGrid);
   pGdrEntry->ConfigureHarpedStrandGrids(HgStart,HgHp1,HgHp2,HgEnd,startGrid,startHPGrid,endHPGrid,endGrid);

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
         Float64 dist = -cg;
         absOffset  = dist - offset;
      }
      else if ( measurementType==hsoCGFROMBOTTOM)
      {
         // top is a Y=0.0
         absOffset =  offset - (HgHp1+cg);
      }
      else if (measurementType==hsoECCENTRICITY)
      {
         CComPtr<IBeamFactory> factory;
         pGdrEntry->GetBeamFactory(&factory);

         CComPtr<IGirderSection> gdrSection;
         factory->CreateGirderSection(m_pBroker,INVALID_ID,pGdrEntry->GetDimensions(),-1,-1,&gdrSection);

         CComQIPtr<IShape> shape(gdrSection);
         CComPtr<IShapeProperties> props;
         shape->get_ShapeProperties(&props);
         Float64 Yb;
         props->get_Ybottom(&Yb);

         Float64 ecc = Yb - (HgHp1+cg);

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
         absOffset =  offset - (HgHp1+toploc);
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

      absOffset =  offset - (HgHp1+botloc);
   }
   else
   {
      ATLASSERT(false);
   }

   return absOffset;
}

Float64 CBridgeAgentImp::ComputeAbsoluteHarpedOffsetHp(const CSegmentKey& segmentKey, const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset)
{
   // returns the offset value measured from the original strand locations defined in the girder library
   // Up is positive
   Float64 HgStart, HgHp1, HgHp2, HgEnd;
   GetHarpedStrandControlHeights(segmentKey,&HgStart,&HgHp1,&HgHp2,&HgEnd);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CSplicedGirderData* pGirder = pIBridgeDesc->GetGirder(segmentKey);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);

   pgsTypes::AdjustableStrandType adjType = pSegment->Strands.GetAdjustableStrandType();
   Float64 absOffset = ComputeAbsoluteHarpedOffsetHp(pGirder->GetGirderName(), adjType, HgStart, HgHp1, HgHp2, HgEnd, rHarpedFillArray,measurementType,offset);

#if defined _DEBUG

//   VALIDATE( GIRDER );
   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey, &girder);

   Float64 increment; // if less than zero, strands cannot be adjusted
   girder->get_HarpedHpAdjustmentIncrement(&increment);

   Float64 result = 0;
   if ( 0.0 <= increment && AreStrandsInConfigFillVec(rHarpedFillArray) )
   {
      if (measurementType==hsoLEGACY)
      {
         result = offset;
      }
      else if (measurementType==hsoCGFROMTOP || measurementType==hsoCGFROMBOTTOM || measurementType==hsoECCENTRICITY)
      {
         // compute adjusted cg location
         CIndexArrayWrapper fill(rHarpedFillArray);

         Float64 L = GetSegmentLength(segmentKey); 

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
            // top is a Y=0.0
            result =  offset - (HgHp1+cg);
         }
         else if (measurementType==hsoECCENTRICITY)
         {
            IntervalIndexType releaseIntervalIdx = GetPrestressReleaseInterval(segmentKey);
            Float64 Yb = GetY(releaseIntervalIdx,pgsPointOfInterest(segmentKey,0.0),pgsTypes::BottomGirder);
            Float64 ecc = Yb - (HgHp1+cg);

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
            result =  offset - (HgHp1+toploc);
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

         result =  offset - (HgHp1+botloc);
      }
      else
      {
         ATLASSERT(false);
      }
   }

   result = IsZero(result,0.005) ? 0.0 : result;
   ATLASSERT(IsEqual(result,absOffset));
#endif // __DEBUG

   return absOffset;
}

Float64 CBridgeAgentImp::ComputeAbsoluteHarpedOffsetHp(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset)
{
   // returns the offset value measured from the original strand locations defined in the girder library
   // Up is positive
   if ( !AreStrandsInConfigFillVec(rHarpedFillArray) )
   {
      return 0;
   }

   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   if (adjType==pgsTypes::asStraight)
   {
      if  (!pGdrEntry->IsVerticalAdjustmentAllowedStraight())
      {
         return 0.0; // No offset allowed
      }
   }
   else if ( !pGdrEntry->IsVerticalAdjustmentAllowedHP())
   {
      return 0.0; // No offset allowed
   }

   CComPtr<IStrandMover> pStrandMover;
   CreateStrandMover(strGirderName,adjType,&pStrandMover);

   CComPtr<IStrandGrid> startGrid, startHPGrid, endHPGrid, endGrid;
   startGrid.CoCreateInstance(CLSID_StrandGrid);
   startHPGrid.CoCreateInstance(CLSID_StrandGrid);
   endHPGrid.CoCreateInstance(CLSID_StrandGrid);
   endGrid.CoCreateInstance(CLSID_StrandGrid);
   pGdrEntry->ConfigureHarpedStrandGrids(HgStart,HgHp1,HgHp2,HgEnd,startGrid,startHPGrid,endHPGrid,endGrid);

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

      CComPtr<IIndexArray> hp_fill;
      CComQIPtr<IStrandGridFiller> startGridFiller(startGrid);
      ComputeHpFill(pGdrEntry, startGridFiller, &fill, &hp_fill);

      CComPtr<IPoint2dCollection> points;
      hpGridFiller->GetStrandPositionsEx(hp_fill,&points);

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
         // top is a Y=0.0
         absOffset =  offset - (HgHp1+cg);
      }
      else if (measurementType==hsoECCENTRICITY)
      {
         CComPtr<IBeamFactory> factory;
         pGdrEntry->GetBeamFactory(&factory);

         CComPtr<IGirderSection> gdrSection;
         factory->CreateGirderSection(m_pBroker,INVALID_ID,pGdrEntry->GetDimensions(),-1,-1,&gdrSection);

         CComQIPtr<IShape> shape(gdrSection);
         CComPtr<IShapeProperties> props;
         shape->get_ShapeProperties(&props);
         Float64 Yb;
         props->get_Ybottom(&Yb);

         Float64 ecc = Yb - (HgHp1+cg);

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
         absOffset =  offset - (HgHp1+toploc);
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

      absOffset =  offset - (HgHp1+botloc);
   }
   else
   {
      ATLASSERT(false);
   }

   absOffset = IsZero(absOffset) ? 0.0 : absOffset;
   return absOffset;
}

Float64 CBridgeAgentImp::ComputeHarpedOffsetFromAbsoluteEnd(const CSegmentKey& segmentKey, const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset)
{
   // all we really need to know is the distance and direction between the coords, compute absolute
   // from zero
   Float64 absol = ComputeAbsoluteHarpedOffsetEnd(segmentKey, rHarpedFillArray, measurementType, 0.0);

   Float64 offset = 0.0;
   // direction depends if meassured from bottom up or top down
   if (measurementType==hsoLEGACY || measurementType==hsoCGFROMTOP || measurementType==hsoTOP2TOP
      || measurementType==hsoECCENTRICITY)
   {
      offset = absol - absoluteOffset ;
   }
   else if (measurementType==hsoCGFROMBOTTOM || measurementType==hsoTOP2BOTTOM || 
            measurementType==hsoBOTTOM2BOTTOM)
   {
      offset = absoluteOffset - absol;
   }
   else
   {
      ATLASSERT(false); 
   }

   return offset;
}

Float64 CBridgeAgentImp::ComputeHarpedOffsetFromAbsoluteEnd(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset)
{
   // all we really need to know is the distance and direction between the coords, compute absolute
   // from zero
   Float64 absol = ComputeAbsoluteHarpedOffsetEnd(strGirderName, adjType, HgStart, HgHp1, HgHp2, HgEnd, rHarpedFillArray, measurementType, 0.0);

   Float64 offset = 0.0;
   // direction depends if meassured from bottom up or top down
   if (measurementType==hsoLEGACY || measurementType==hsoCGFROMTOP || measurementType==hsoTOP2TOP
      || measurementType==hsoECCENTRICITY)
   {
      offset = absol - absoluteOffset ;
   }
   else if (measurementType==hsoCGFROMBOTTOM || measurementType==hsoTOP2BOTTOM || 
            measurementType==hsoBOTTOM2BOTTOM)
   {
      offset = absoluteOffset - absol;
   }
   else
   {
      ATLASSERT(false); 
   }

   return offset;
}

Float64 CBridgeAgentImp::ComputeHarpedOffsetFromAbsoluteHp(const CSegmentKey& segmentKey, const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset)
{
   Float64 absol = ComputeAbsoluteHarpedOffsetHp(segmentKey, rHarpedFillArray, measurementType, 0.0);

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
   {
      ATLASSERT(false);
   }

   return off;
}

Float64 CBridgeAgentImp::ComputeHarpedOffsetFromAbsoluteHp(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset)
{
   Float64 absol = ComputeAbsoluteHarpedOffsetHp(strGirderName, adjType, HgStart, HgHp1, HgHp2, HgEnd, rHarpedFillArray, measurementType, 0.0);

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
   {
      ATLASSERT(false);
   }

   return off;
}

void CBridgeAgentImp::ComputeValidHarpedOffsetForMeasurementTypeEnd(const CSegmentKey& segmentKey, const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange)
{
   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey, &girder);

   CIndexArrayWrapper fill(rHarpedFillArray);

   Float64 absDown, absUp;
   HRESULT hr = girder->GetHarpedEndAdjustmentBoundsEx(&fill, &absDown, &absUp);
   ATLASSERT(SUCCEEDED(hr));

   Float64 offDown = ComputeHarpedOffsetFromAbsoluteEnd(segmentKey,  rHarpedFillArray, measurementType, absDown);
   Float64 offUp =   ComputeHarpedOffsetFromAbsoluteEnd(segmentKey,  rHarpedFillArray, measurementType, absUp);

   *lowRange = offDown;
   *highRange = offUp;
}

void CBridgeAgentImp::ComputeValidHarpedOffsetForMeasurementTypeEnd(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray,HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange)
{
   Float64 absDown, absUp;
   GetHarpedEndOffsetBoundsEx(strGirderName, adjType, HgStart, HgHp1, HgHp2, HgEnd,rHarpedFillArray, &absDown,&absUp);

   Float64 offDown = ComputeHarpedOffsetFromAbsoluteEnd(strGirderName, adjType, HgStart, HgHp1, HgHp2, HgEnd,  rHarpedFillArray, measurementType, absDown);
   Float64 offUp =   ComputeHarpedOffsetFromAbsoluteEnd(strGirderName, adjType, HgStart, HgHp1, HgHp2, HgEnd,  rHarpedFillArray, measurementType, absUp);

   *lowRange = offDown;
   *highRange = offUp;
}

void CBridgeAgentImp::ComputeValidHarpedOffsetForMeasurementTypeHp(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange)
{
   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey, &girder);

   CIndexArrayWrapper fill(rHarpedFillArray);

   Float64 absDown, absUp;
   HRESULT hr = girder->GetHarpedHpAdjustmentBoundsEx(&fill, &absDown, &absUp);
   ATLASSERT(SUCCEEDED(hr));

   Float64 offDown = ComputeHarpedOffsetFromAbsoluteHp(segmentKey,  rHarpedFillArray, measurementType, absDown);
   Float64 offUp   = ComputeHarpedOffsetFromAbsoluteHp(segmentKey,  rHarpedFillArray, measurementType, absUp);

   *lowRange = offDown;
   *highRange = offUp;
}

void CBridgeAgentImp::ComputeValidHarpedOffsetForMeasurementTypeHp(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange)
{
   Float64 absDown, absUp;
   GetHarpedHpOffsetBoundsEx(strGirderName, adjType, HgStart, HgHp1, HgHp2, HgEnd,rHarpedFillArray, &absDown,&absUp);

   Float64 offDown = ComputeHarpedOffsetFromAbsoluteHp(strGirderName, adjType, HgStart, HgHp1, HgHp2, HgEnd,  rHarpedFillArray, measurementType, absDown);
   Float64 offUp =   ComputeHarpedOffsetFromAbsoluteHp(strGirderName, adjType, HgStart, HgHp1, HgHp2, HgEnd,  rHarpedFillArray, measurementType, absUp);

   *lowRange = offDown;
   *highRange = offUp;
}

Float64 CBridgeAgentImp::ConvertHarpedOffsetEnd(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType)
{
   Float64 abs_offset = ComputeAbsoluteHarpedOffsetEnd(segmentKey,rHarpedFillArray,fromMeasurementType,offset);
   Float64 result = ComputeHarpedOffsetFromAbsoluteEnd(segmentKey,rHarpedFillArray,toMeasurementType,abs_offset);
   return result;
}

Float64 CBridgeAgentImp::ConvertHarpedOffsetEnd(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType)
{
   Float64 abs_offset = ComputeAbsoluteHarpedOffsetEnd(strGirderName, adjType, HgStart, HgHp1, HgHp2, HgEnd,rHarpedFillArray,fromMeasurementType,offset);
   Float64 result = ComputeHarpedOffsetFromAbsoluteEnd(strGirderName, adjType, HgStart, HgHp1, HgHp2, HgEnd,rHarpedFillArray,toMeasurementType,abs_offset);
   return result;
}

Float64 CBridgeAgentImp::ConvertHarpedOffsetHp(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType)
{
   Float64 abs_offset = ComputeAbsoluteHarpedOffsetHp(segmentKey,rHarpedFillArray,fromMeasurementType,offset);
   Float64 result = ComputeHarpedOffsetFromAbsoluteHp(segmentKey,rHarpedFillArray,toMeasurementType,abs_offset);
   return result;
}

Float64 CBridgeAgentImp::ConvertHarpedOffsetHp(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType,Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType)
{
   Float64 abs_offset = ComputeAbsoluteHarpedOffsetHp(strGirderName, adjType, HgStart, HgHp1, HgHp2, HgEnd,rHarpedFillArray,fromMeasurementType,offset);
   Float64 result = ComputeHarpedOffsetFromAbsoluteHp(strGirderName, adjType, HgStart, HgHp1, HgHp2, HgEnd,rHarpedFillArray,toMeasurementType,abs_offset);
   return result;
}

/////////////////////////////////////////////////////////////////////////
// IPointOfInterest
std::vector<pgsPointOfInterest> CBridgeAgentImp::GetPointsOfInterest(const CSegmentKey& segmentKey)
{
   ValidatePointsOfInterest(segmentKey);
   return m_pPoiMgr->GetPointsOfInterest(segmentKey);
}

std::vector<pgsPointOfInterest> CBridgeAgentImp::GetPointOfInterests(Float64 station,IDirection* pDirection)
{
#pragma Reminder("UPDATE - this can be made faster")
   // this method is using a brute force search. There are faster ways to do this.
   std::vector<pgsPointOfInterest> vPoi;
   GroupIndexType nGroups = GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType nGirders = GetGirderCount(grpIdx);
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         SegmentIndexType nSegments = GetSegmentCount(grpIdx,gdrIdx);
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);
            pgsPointOfInterest poi;
            if ( GetPointOfInterest(segmentKey,station,pDirection,&poi) )
            {
               vPoi.push_back(poi);
            }
         }
      }
   }
   return vPoi;
}

std::vector<pgsPointOfInterest> CBridgeAgentImp::GetPointsOfInterest(const CSegmentKey& segmentKey,PoiAttributeType attrib,Uint32 mode)
{
   ValidatePointsOfInterest(segmentKey);
   std::vector<pgsPointOfInterest> vPoi;
   m_pPoiMgr->GetPointsOfInterest(segmentKey,attrib,mode,&vPoi);
   return vPoi;
}

std::vector<pgsPointOfInterest> CBridgeAgentImp::GetPointsOfInterest(const CSpanKey& spanKey)
{
   return GetPointsOfInterest(spanKey,POI_SPAN);
}

std::vector<pgsPointOfInterest> CBridgeAgentImp::GetPointsOfInterest(const CSpanKey& spanKey,PoiAttributeType attrib,Uint32 mode)
{
   ValidatePointsOfInterest(CGirderKey(ALL_GROUPS,spanKey.girderIndex));
   std::vector<pgsPointOfInterest> vPoi;
   GroupIndexType grpIdx = (spanKey.spanIndex == ALL_SPANS ? ALL_GROUPS : GetGirderGroupIndex(spanKey.spanIndex));
   m_pPoiMgr->GetPointsOfInterest(CSegmentKey(grpIdx,spanKey.girderIndex,ALL_SEGMENTS),attrib,mode,&vPoi);
   vPoi.erase(std::remove_if(vPoi.begin(),vPoi.end(),PoiNotInSpan(this,spanKey)),vPoi.end());
   return vPoi;
}

pgsPointOfInterest CBridgeAgentImp::GetPointOfInterest(PoiIDType poiID)
{
   ValidatePointsOfInterest(CGirderKey(ALL_GROUPS,ALL_GIRDERS));
   return m_pPoiMgr->GetPointOfInterest(poiID);
}

pgsPointOfInterest CBridgeAgentImp::GetPointOfInterest(const CSegmentKey& segmentKey,Float64 Xpoi,Float64 tolerance)
{
   // This validate is commented out because it causes problems with validating the bridge model
   // The main validate calls this method which calls ValidatePointsOfInterest which calls the main validate
   // ... recursion ==> CRASH
   //ValidatePointsOfInterest(segmentKey);

   Float64 oldTolerance = m_pPoiMgr->SetTolerance(tolerance);
   pgsPointOfInterest poi = m_pPoiMgr->GetPointOfInterest(segmentKey,Xpoi);
   m_pPoiMgr->SetTolerance(oldTolerance);
   return poi;
}

bool CBridgeAgentImp::GetPointOfInterest(const CSegmentKey& segmentKey,Float64 station,IDirection* pDirection,pgsPointOfInterest* pPoi)
{
   VALIDATE( GIRDER );
   CComPtr<IPoint2d> pntOnSegment;
   HRESULT hr = m_BridgeGeometryTool->GirderPathPoint(m_Bridge,::GetSuperstructureMemberID(segmentKey.groupIndex,segmentKey.girderIndex),segmentKey.segmentIndex,CComVariant(station),CComVariant(pDirection),&pntOnSegment);
   if ( FAILED(hr) )
   {
      // Point wasn't found
      return false;
   }
   
   CComPtr<IPoint2d> pntPier1, pntEnd1, pntBrg1, pntBrg2, pntEnd2, pntPier2;
   GetSegmentEndPoints(segmentKey,&pntPier1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntPier2);

   Float64 Xpoi;
   pntEnd1->DistanceEx(pntOnSegment,&Xpoi);

   *pPoi = GetPointOfInterest(segmentKey,Xpoi);
   return true;
}

pgsPointOfInterest CBridgeAgentImp::GetNearestPointOfInterest(const CSegmentKey& segmentKey,Float64 Xpoi)
{
   ValidatePointsOfInterest(segmentKey);
   return m_pPoiMgr->GetNearestPointOfInterest(segmentKey,Xpoi);
}

pgsPointOfInterest CBridgeAgentImp::GetPrevPointOfInterest(PoiIDType poiID,PoiAttributeType attrib,Uint32 mode)
{
   ATLASSERT(poiID != INVALID_ID);
   // assumes the POIs are already validated.... if they weren't we would have a valid poiID
   return m_pPoiMgr->GetPrevPointOfInterest(poiID,attrib,mode == POIFIND_OR ? POIMGR_OR : POIMGR_AND);
}

pgsPointOfInterest CBridgeAgentImp::GetNextPointOfInterest(PoiIDType poiID,PoiAttributeType attrib,Uint32 mode)
{
   ATLASSERT(poiID != INVALID_ID);
   // assumes the POIs are already validated.... if they weren't we would have a valid poiID
   return m_pPoiMgr->GetNextPointOfInterest(poiID,attrib,mode == POIFIND_OR ? POIMGR_OR : POIMGR_AND);
}

std::vector<pgsPointOfInterest> CBridgeAgentImp::GetCriticalSections(pgsTypes::LimitState limitState,const CGirderKey& girderKey)
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   // LRFD 2004 and later, critical section is only a function of dv, which comes from the calculation of Mu,
   // so critical section is not a function of the limit state. We will work with the Strength I limit state
   if ( lrfdVersionMgr::ThirdEdition2004 <= pSpecEntry->GetSpecificationType() )
   {
      limitState = pgsTypes::StrengthI;
   }

   int idx = limitState == pgsTypes::StrengthI ? 0 : 1;
   std::set<CGirderKey>::iterator found;
   found = m_CriticalSectionState[idx].find( girderKey );
   if ( found == m_CriticalSectionState[idx].end() )
   {
      // Critical sections not previously computed.
      // Do it now.
      GET_IFACE(IShearCapacity,pShearCap);
      const std::vector<CRITSECTDETAILS>& vCSDetails(pShearCap->GetCriticalSectionDetails(limitState,girderKey));

      std::vector<CRITSECTDETAILS>::const_iterator iter(vCSDetails.begin());
      std::vector<CRITSECTDETAILS>::const_iterator end(vCSDetails.end());
      for ( ; iter != end; iter++ )
      {
         const CRITSECTDETAILS& csDetails(*iter);
         if ( csDetails.bAtFaceOfSupport )
         {
            m_pPoiMgr->AddPointOfInterest(csDetails.poiFaceOfSupport);
         }
         else
         {
            m_pPoiMgr->AddPointOfInterest(csDetails.pCriticalSection->Poi);
         }
      }

      m_CriticalSectionState[idx].insert( girderKey );
   }

   std::vector<pgsPointOfInterest> vPoi;
   m_pPoiMgr->GetPointsOfInterest(CSegmentKey(girderKey,ALL_SEGMENTS),(limitState == pgsTypes::StrengthI ? POI_CRITSECTSHEAR1 : POI_CRITSECTSHEAR2),POIMGR_OR,&vPoi);
   return vPoi;
}

std::vector<pgsPointOfInterest> CBridgeAgentImp::GetCriticalSections(pgsTypes::LimitState limitState,const CGirderKey& girderKey,const GDRCONFIG& config)
{
   std::vector<pgsPointOfInterest> vPoi;

   PoiAttributeType attrib = (limitState == pgsTypes::StrengthI ? POI_CRITSECTSHEAR1 : POI_CRITSECTSHEAR2);

   GET_IFACE(IShearCapacity,pShearCap);
   std::vector<Float64> vcsLoc(pShearCap->GetCriticalSections(limitState,girderKey,config));
   std::vector<Float64>::iterator iter(vcsLoc.begin());
   std::vector<Float64>::iterator end(vcsLoc.end());
   for ( ; iter != end; iter++ )
   {
      Float64 Xg = *iter;
      pgsPointOfInterest poi = ConvertGirderCoordinateToPoi(girderKey,Xg,0.0/*we have to be exact, so set tolerance to 0.0*/);
      poi.SetNonReferencedAttributes(attrib);
      vPoi.push_back(poi);
   }

   return vPoi;
}

pgsPointOfInterest CBridgeAgentImp::GetPierPointOfInterest(const CGirderKey& girderKey,PierIndexType pierIdx)
{
   ASSERT_GIRDER_KEY(girderKey);

   // Gets the POI at a pier. If this is the first or last pier, get the CL Bearing POI.

   Float64 Xgp;
   VERIFY( GetPierLocation(girderKey,pierIdx,&Xgp) );
   return ConvertGirderPathCoordinateToPoi(girderKey,Xgp);
}

pgsPointOfInterest CBridgeAgentImp::GetTemporarySupportPointOfInterest(const CGirderKey& girderKey,SupportIndexType tsIdx)
{
   ASSERT_GIRDER_KEY(girderKey);

   Float64 Xgp = GetTemporarySupportLocation(tsIdx,girderKey.girderIndex);
   return ConvertGirderPathCoordinateToPoi(girderKey,Xgp);
}

pgsPointOfInterest CBridgeAgentImp::ConvertSpanPointToPoi(const CSpanKey& spanKey,Float64 Xspan,Float64 tolerance)
{
   // convert spanIdx and Xspan to girder coordinates, Xg
   GroupIndexType grpIdx = GetGirderGroupIndex(spanKey.spanIndex);
   GirderIndexType gdrIdx = spanKey.girderIndex;
   Float64 brgOffset = GetSegmentStartBearingOffset(CSegmentKey(grpIdx,gdrIdx,0));
   Float64 endDist   = GetSegmentStartEndDistance(CSegmentKey(grpIdx,gdrIdx,0));
   Float64 endOffset = brgOffset - endDist;
   endOffset = IsZero(endOffset) ? 0 : endOffset;
   SpanIndexType startSpanIdx = GetGirderGroupStartSpan(grpIdx);

   PierIndexType pierIdx = (PierIndexType)spanKey.spanIndex; // index of pier at start of span
   bool bIsContinuous;
   if ( IsInteriorPier(pierIdx) )
   {
      bIsContinuous = true;
   }
   else
   {
      pgsTypes::BoundaryConditionType connectionType = GetBoundaryConditionType(pierIdx);
      bIsContinuous = !(connectionType == pgsTypes::bctHinge || connectionType == pgsTypes::bctRoller);
   }

   // For the first span, the span coordinate system begins at the CL Bearing which is
   // at girder coordinate Xg = endDist
   //
   // .---- Back of Pavement Seat (Abutment Reference Line)
   // |
   // |<---------->|--- Bearing Offset
   // |        |<->|--- End Distance
   // |        +-----------------------/
   // |        |   * Xg = End Distance \
   // |        +-----------------------/
   // |            ^
   //              |
   //              +----- CL Bearing (Xspan = 0, Xg = End Distance)

   // For other all spans, the start of the span coordinate is at the CL Pier if the 
   // pier connection is continuous otherwise it is at the CL Bearing.
   // For continuous pier connections, the girder coordinate for this location is Xg = -endOffset.
   // For all others, Xg = endDist;
   //
   //                       |<---------->|--- Bearing Offset
   //         End Offset ---|<------>|<->|--- End Distance
   // /------------+        |        +-----------------------/
   // \  Span i-1  |        |        |    Span i             \
   // /------------+        |        +-----------------------/
   //                       ^            ^
   //                       |            |
   //                       |            +----- CL Bearing (Xspan = 0, Xg = End Distance) (for non-continuous piers)
   //                       |
   //                       +----- CL Bearing (Xspan = 0, Xg = -(End Offset)) (for continuous piers)

   Float64 Xg = (startSpanIdx == 0 || !bIsContinuous) ? endDist : -endOffset;

   for ( SpanIndexType idx = startSpanIdx; idx < spanKey.spanIndex; idx++ )
   {
      Float64 Ls = GetSpanLength(idx,gdrIdx);
      Xg += Ls;
   }
   Xg += Xspan;

   // now convert the girder coordinate to a POI
   pgsPointOfInterest poi = ConvertGirderCoordinateToPoi(CGirderKey(grpIdx,gdrIdx),Xg,tolerance);

#if defined CHECK_POI_CONVERSIONS
   static bool bTesting = false;
   if ( !bTesting )
   {
      bTesting = true;
      CSpanKey testSpanKey;
      Float64 testXspan;
      ConvertPoiToSpanPoint(poi,&testSpanKey,&testXspan);
      SpanIndexType testSpanIdx = testSpanKey.spanIndex;
      if ( testSpanIdx == spanKey.spanIndex-1 )
      {
         // input span location is at the start of a span... poi is at the end of the previous span
         // this puts the test results at the end of the previous span and it is the same location
         // as the input location
         ATLASSERT(IsZero(Xspan)); // input should be at the start of the span
         ATLASSERT(IsEqual(testXspan,GetSpanLength(testSpanIdx,gdrIdx))); // should be at the end of the test span
      }
      else if ( spanKey.spanIndex == testSpanIdx-1 )
      {
         // input span location is at the end of a span... poi is at the start of the next span
         // this points the test results at the start of the next span and it is the same location
         // as the input location
         ATLASSERT(IsEqual(Xspan,GetSpanLength(spanKey))); // should be at the end of the span
         ATLASSERT(IsZero(testXspan)); // should be at the start of the next span
      }
      else
      {
         ATLASSERT(testSpanIdx == spanKey.spanIndex && IsEqual(testXspan,Xspan));
      }
      bTesting = false;
   }
#endif
   return poi;
}

void CBridgeAgentImp::ConvertPoiToSpanPoint(const pgsPointOfInterest& poi,CSpanKey* pSpanKey,Float64* pXspan)
{
   Float64 XgPoi = ConvertPoiToGirderCoordinate(poi);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   CGirderKey girderKey(segmentKey);
   pSpanKey->girderIndex = segmentKey.girderIndex;

   SpanIndexType startSpanIdx, endSpanIdx;
   GetGirderGroupSpans(segmentKey.groupIndex,&startSpanIdx,&endSpanIdx);

   Float64 brgOffset = GetSegmentStartBearingOffset(CSegmentKey(girderKey,0));
   Float64 endDist   = GetSegmentStartEndDistance(CSegmentKey(girderKey,0));
   Float64 endOffset = brgOffset - endDist;
   endOffset = IsZero(endOffset) ? 0 : endOffset;

   PierIndexType startPierIdx = (PierIndexType)(startSpanIdx); // index of pier at start of span
   pgsTypes::BoundaryConditionType connectionType = GetBoundaryConditionType(startPierIdx);
   bool bIsContinuous = !(connectionType == pgsTypes::bctHinge || connectionType == pgsTypes::bctRoller);

   pSpanKey->spanIndex = INVALID_INDEX;
   Float64 XgStartSpan = (startSpanIdx == 0 || !bIsContinuous ? endDist : -endOffset);
   if ( XgPoi < XgStartSpan )
   {
      // poi is before the start of the span
      pSpanKey->spanIndex = startSpanIdx;
      *pXspan = XgPoi - XgStartSpan;
   }
   else
   {
      for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
      {
         Float64 Lspan = GetSpanLength(spanIdx,segmentKey.girderIndex);
         Float64 XgEndSpan = XgStartSpan + Lspan;
         if ( ::InRange(XgStartSpan,XgPoi,XgEndSpan) )
         {
            // we found the span that contains the poi
            pSpanKey->spanIndex = spanIdx;
            *pXspan = XgPoi - XgStartSpan;
            break;
         }
         XgStartSpan = XgEndSpan; // start of next span is end of this span
      }

      if ( pSpanKey->spanIndex == INVALID_INDEX )
      {
         // XgPoi is beyond the end of the last span
         // XgStartSpan is the start of the next span which is the end of the last span... which is what we want
         pSpanKey->spanIndex = endSpanIdx;

         // Xspan is (XgPoi - XgStartSpan) which is the distance from the end of the last span to the poi
         // plus the length of the span which makes the value be the distance from the start of the last span
         *pXspan = XgPoi - XgStartSpan + GetSpanLength(endSpanIdx,segmentKey.girderIndex);
      }
   }

#if defined CHECK_POI_CONVERSIONS
   pgsPointOfInterest testPoi = ConvertSpanPointToPoi(*pSpanKey,*pXspan);
   ATLASSERT(poi.AtSamePlace(testPoi));
#endif
}

void CBridgeAgentImp::ConvertSpanPointToSegmentCoordiante(const CSpanKey& spanKey,Float64 Xspan,CSegmentKey* pSegmentKey,Float64* pXs)
{
   pgsPointOfInterest poi = ConvertSpanPointToPoi(spanKey,Xspan);
   *pSegmentKey = poi.GetSegmentKey();
   *pXs = poi.GetDistFromStart();
}

void CBridgeAgentImp::ConvertSegmentCoordinateToSpanPoint(const CSegmentKey& segmentKey,Float64 Xs,CSpanKey* pSpanKey,Float64* pXspan)
{
   ConvertPoiToSpanPoint(pgsPointOfInterest(segmentKey,Xs),pSpanKey,pXspan);
}

void CBridgeAgentImp::ConvertSpanPointToSegmentPathCoordiante(const CSpanKey& spanKey,Float64 Xspan,CSegmentKey* pSegmentKey,Float64* pXsp)
{
   Float64 Xs;
   ConvertSpanPointToSegmentCoordiante(spanKey,Xspan,pSegmentKey,&Xs);
   Float64 brgOffset = GetSegmentStartBearingOffset(*pSegmentKey);
   Float64 endDist   = GetSegmentStartEndDistance(*pSegmentKey);
   Float64 offset_dist = brgOffset - endDist;
   offset_dist = IsZero(offset_dist) ? 0 : offset_dist;
   Float64 Xsp = Xs - offset_dist;
   *pXsp = Xsp;
}

void CBridgeAgentImp::ConvertSegmentPathCoordinateToSpanPoint(const CSegmentKey& segmentKey,Float64 Xsp,CSpanKey* pSpanKey,Float64* pXspan)
{
   Float64 Xs = ConvertSegmentPathCoordinateToSegmentCoordinate(segmentKey,Xsp);
   ConvertSegmentCoordinateToSpanPoint(segmentKey,Xs,pSpanKey,pXspan);
}

std::vector<pgsPointOfInterest> CBridgeAgentImp::GetPointsOfInterestInRange(Float64 xLeft,const pgsPointOfInterest& poi,Float64 xRight)
{
   ValidatePointsOfInterest(poi.GetSegmentKey());

   Float64 xMin = poi.GetDistFromStart() - xLeft;
   Float64 xMax = poi.GetDistFromStart() + xRight;

   std::vector<pgsPointOfInterest> vPoi;
   m_pPoiMgr->GetPointsOfInterestInRange(poi.GetSegmentKey(),xMin,xMax,&vPoi);
   return vPoi;
}

std::list<std::vector<pgsPointOfInterest>> CBridgeAgentImp::GroupBySegment(const std::vector<pgsPointOfInterest>& vPoi)
{
   std::list<std::vector<pgsPointOfInterest>> lPoi;
   if ( vPoi.size() == 0 )
   {
      return lPoi;
   }

   CSegmentKey currentSegmentKey = vPoi.front().GetSegmentKey();
   std::vector<pgsPointOfInterest> vPoiThisSegment;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi(*iter);
      const CSegmentKey& thisSegmentKey(poi.GetSegmentKey());
      if ( !thisSegmentKey.IsEqual(currentSegmentKey) )
      {
         // a new segment was encountered

         // save the vector of POI into the list
         lPoi.push_back(vPoiThisSegment);

         // clear it so we start fresh for the next segment
         vPoiThisSegment.clear();

         // next segment
         currentSegmentKey = thisSegmentKey;
      }
      vPoiThisSegment.push_back(poi);
   }

   lPoi.push_back(vPoiThisSegment);

   return lPoi;
}

std::list<std::vector<pgsPointOfInterest>> CBridgeAgentImp::GroupByGirder(const std::vector<pgsPointOfInterest>& vPoi)
{
   std::list<std::vector<pgsPointOfInterest>> lPoi;
   if ( vPoi.size() == 0 )
   {
      return lPoi;
   }

   CGirderKey currentGirderKey = vPoi.front().GetSegmentKey();
   std::vector<pgsPointOfInterest> vPoiThisGirder;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi(*iter);
      CGirderKey thisGirderKey(poi.GetSegmentKey());
      if ( !thisGirderKey.IsEqual(currentGirderKey) )
      {
         // a new girder was encountered

         // save the vector of POI into the list
         lPoi.push_back(vPoiThisGirder);

         // clear it so we start fresh for the next girder
         vPoiThisGirder.clear();

         // next girder
         currentGirderKey = thisGirderKey;
      }
      vPoiThisGirder.push_back(poi);
   }

   lPoi.push_back(vPoiThisGirder);

   return lPoi;
}

std::vector<CSegmentKey> CBridgeAgentImp::GetSegmentKeys(const std::vector<pgsPointOfInterest>& vPoi)
{
   std::vector<CSegmentKey> vSegmentKeys;
   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      const pgsPointOfInterest& poi(*poiIter);
      vSegmentKeys.push_back(poi.GetSegmentKey());
   }
   vSegmentKeys.erase(std::unique(vSegmentKeys.begin(),vSegmentKeys.end()),vSegmentKeys.end());
   return vSegmentKeys;
}

std::vector<CSegmentKey> CBridgeAgentImp::GetSegmentKeys(const std::vector<pgsPointOfInterest>& vPoi,const CGirderKey& girderKey)
{
   std::vector<CSegmentKey> vSegmentKeys;
   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      const pgsPointOfInterest& poi(*poiIter);
      if ( CGirderKey(poi.GetSegmentKey()) == girderKey )
      {
         vSegmentKeys.push_back(poi.GetSegmentKey());
      }
   }
   vSegmentKeys.erase(std::unique(vSegmentKeys.begin(),vSegmentKeys.end()),vSegmentKeys.end());
   return vSegmentKeys;
}

std::vector<CGirderKey> CBridgeAgentImp::GetGirderKeys(const std::vector<pgsPointOfInterest>& vPoi)
{
   std::vector<CSegmentKey> vSegmentKeys(GetSegmentKeys(vPoi));
   std::vector<CGirderKey> vGirderKeys;
   vGirderKeys.insert(vGirderKeys.begin(),vSegmentKeys.begin(),vSegmentKeys.end());
   vGirderKeys.erase(std::unique(vGirderKeys.begin(),vGirderKeys.end()),vGirderKeys.end());
   return vGirderKeys;
}

Float64 CBridgeAgentImp::ConvertPoiToSegmentPathCoordinate(const pgsPointOfInterest& poi)
{
   Float64 Xsp = 0;
   if ( poi.HasSegmentPathCoordinate() )
   {
#if defined CHECK_POI_CONVERSIONS
      // force manual computation to verify the dimension in the poi is correct
      pgsPointOfInterest testPoi(poi.GetSegmentKey(),poi.GetDistFromStart());
      Float64 XspTest = ConvertPoiToSegmentPathCoordinate(testPoi);
      ATLASSERT(IsEqual(XspTest,poi.GetSegmentPathCoordinate(),gs_PoiTolerance));
#endif
      Xsp = poi.GetSegmentPathCoordinate();
   }
   else
   {
      Xsp = ConvertSegmentCoordinateToSegmentPathCoordinate(poi.GetSegmentKey(),poi.GetDistFromStart());
      if ( poi.GetID() != INVALID_ID )
      {
         pgsPointOfInterest* pPoi = const_cast<pgsPointOfInterest*>(&poi);
         pPoi->SetSegmentPathCoordinate(Xsp);
      }
   }

#if defined CHECK_POI_CONVERSIONS
   static bool bTesting = false;
   if ( !bTesting )
   {
      bTesting = true;
      pgsPointOfInterest testPoi = ConvertSegmentPathCoordinateToPoi(poi.GetSegmentKey(),Xsp);
      ATLASSERT(poi.AtSamePlace(testPoi));
      bTesting = false;
   }
#endif

   return Xsp;
}

pgsPointOfInterest CBridgeAgentImp::ConvertSegmentPathCoordinateToPoi(const CSegmentKey& segmentKey,Float64 Xsp,Float64 tolerance)
{
   Float64 Xs = ConvertSegmentPathCoordinateToSegmentCoordinate(segmentKey,Xsp);
   pgsPointOfInterest poi = GetPointOfInterest(segmentKey,Xs,tolerance);

#if defined CHECK_POI_CONVERSIONS
   static bool bTesting = false;
   if ( !bTesting )
   {
      bTesting = true;
      Float64 testXsp = ConvertPoiToSegmentPathCoordinate(poi);
      ATLASSERT(IsEqual(testXsp,Xsp,tolerance));
      bTesting = false;
   }
#endif

   return poi;
}

Float64 CBridgeAgentImp::ConvertSegmentCoordinateToSegmentPathCoordinate(const CSegmentKey& segmentKey,Float64 Xs)
{
   // We want to measure from the left face of the segment
   // |<------- Xsp ----->>
   // |<-- brg offset->|
   // |   |<-end dist->|
   // |<->|-- add this distance (start_offset) to Xs to get Xsp
   // |   |<--- Xs -------->>
   // |   |
   // |   +---------------------------------------/
   // |   |         Segment i                     \
   // |   +---------------------------------------/
   Float64 brg_offset = GetSegmentStartBearingOffset(segmentKey);
   Float64 end_dist   = GetSegmentStartEndDistance(segmentKey);
   Float64 start_offset = brg_offset - end_dist;
   Float64 Xsp = Xs + start_offset;


#if defined CHECK_POI_CONVERSIONS
   static bool bTesting = false;
   if ( !bTesting )
   {
      bTesting = true;
      Float64 testXs = ConvertSegmentPathCoordinateToSegmentCoordinate(segmentKey,Xsp);
      ATLASSERT(IsEqual(testXs,Xs,gs_PoiTolerance));
      bTesting = false;
   }
#endif

   return Xsp;
}

Float64 CBridgeAgentImp::ConvertSegmentPathCoordinateToSegmentCoordinate(const CSegmentKey& segmentKey,Float64 Xsp)
{
   // We want to measure from the left face of the segment
   // |<------- Xsp ----->>
   // |<-- brg offset->|
   // |   |<-end dist->|
   // |<->|-- add this distance (start_offset) to Xs to get Xsp
   // |   |<--- Xs -------->>
   // |   |
   // |   +---------------------------------------/
   // |   |         Segment i                     \
   // |   +---------------------------------------/

   Float64 brg_offset = GetSegmentStartBearingOffset(segmentKey);
   Float64 end_dist   = GetSegmentStartEndDistance(segmentKey);
   Float64 start_offset = brg_offset - end_dist;
   Float64 Xs = Xsp - start_offset;

#if defined CHECK_POI_CONVERSIONS
   static bool bTesting = false;
   if ( !bTesting )
   {
      bTesting = true;
      Float64 testXsp = ConvertSegmentCoordinateToSegmentPathCoordinate(segmentKey,Xs);
      ATLASSERT(IsEqual(testXsp,Xsp,gs_PoiTolerance));
      bTesting = false;
   }
#endif

   return Xs;
}

Float64 CBridgeAgentImp::ConvertSegmentCoordinateToGirderCoordinate(const CSegmentKey& segmentKey,Float64 Xs)
{
   pgsPointOfInterest poi(segmentKey,Xs);
   Float64 Xg = ConvertPoiToGirderCoordinate(poi);

   return Xg;
}

//void CBridgeAgentImp::ConvertGirderCoordinateToSegmentCoordinate(const CGirderKey& girderKey,Float64 Xg,CSegmentKey* pSegmentKey,Float64* pXs)
//{
//}

Float64 CBridgeAgentImp::ConvertSegmentPathCoordinateToGirderPathCoordinate(const CSegmentKey& segmentKey,Float64 Xsp)
{
   pgsPointOfInterest poi = ConvertSegmentPathCoordinateToPoi(segmentKey,Xsp);
   return ConvertPoiToGirderPathCoordinate(poi);
}

Float64 CBridgeAgentImp::ConvertSegmentCoordinateToGirderlineCoordinate(const CSegmentKey& segmentKey,Float64 Xs)
{
   pgsPointOfInterest poi(segmentKey,Xs);
   return ConvertPoiToGirderlineCoordinate(poi);
}

Float64 CBridgeAgentImp::ConvertPoiToGirderPathCoordinate(const pgsPointOfInterest& poi)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   // must be a real segment
   ASSERT_SEGMENT_KEY(segmentKey);

   GroupIndexType  grpIdx = segmentKey.groupIndex;
   GirderIndexType gdrIdx = segmentKey.girderIndex;

   // Sum the layout length of the segment from the start of the girder, up to, but not including
   // the segment we are interested in
   Float64 Xgp = 0; // distance along the girder measured from the CL Pier at the start of the girder
   if ( poi.HasGirderPathCoordinate() )
   {
      Xgp = poi.GetGirderPathCoordinate();

#if defined CHECK_POI_CONVERSIONS
      pgsPointOfInterest testPoi;
      testPoi.SetLocation(poi.GetSegmentKey(),poi.GetDistFromStart());
      Float64 XgpTest = ConvertPoiToGirderPathCoordinate(testPoi);
      ATLASSERT(IsEqual(Xgp,XgpTest,gs_PoiTolerance));
#endif
   }
   else
   {
      for ( SegmentIndexType segIdx = 0; segIdx < segmentKey.segmentIndex; segIdx++ )
      {
         CSegmentKey thisSegmentKey(grpIdx,gdrIdx,segIdx);

         // CL Pier - CL Pier length of the segment centerline
         Float64 L = GetSegmentLayoutLength(thisSegmentKey);

         Xgp += L;
      }
      
      // add the distance from the start of the segment to the poi
      Float64 Xsp = ConvertPoiToSegmentPathCoordinate(poi);
      Xgp += Xsp;

      if ( poi.GetID() != INVALID_ID )
      {
         pgsPointOfInterest* pPoi = const_cast<pgsPointOfInterest*>(&poi);
         pPoi->SetGirderPathCoordinate(Xgp);
      }
   }

#if defined CHECK_POI_CONVERSIONS
   static bool bTesting = false;
   if ( !bTesting )
   {
      bTesting = true;
      pgsPointOfInterest testPoi = ConvertGirderPathCoordinateToPoi(poi.GetSegmentKey(),Xgp);
      ATLASSERT(poi.AtSamePlace(testPoi));
      bTesting = false;
   }
#endif

   return Xgp;
}

pgsPointOfInterest CBridgeAgentImp::ConvertGirderPathCoordinateToPoi(const CGirderKey& girderKey,Float64 Xgp,Float64 tolerance)
{
   Float64 Xg = ConvertGirderPathCoordinateToGirderCoordinate(girderKey,Xgp);
   pgsPointOfInterest poi = ConvertGirderCoordinateToPoi(girderKey,Xg,tolerance);

#if defined CHECK_POI_CONVERSIONS
   static bool bTesting = false;
   if ( !bTesting )
   {
      bTesting = true;
      Float64 testXgp = ConvertPoiToGirderPathCoordinate(poi);
      ATLASSERT(IsEqual(Xgp,testXgp,tolerance));
      bTesting = false;
   }
#endif

   return poi;
}

pgsPointOfInterest CBridgeAgentImp::ConvertGirderCoordinateToPoi(const CGirderKey& girderKey,Float64 Xg,Float64 tolerance)
{
   pgsPointOfInterest poi;

   Float64 Lg = GetGirderLength(girderKey);
   if ( Xg < 0 )
   {
      // before the start of the girder
      CSegmentKey segmentKey(girderKey,0);
      poi = pgsPointOfInterest(segmentKey,Xg);
   }
   else if ( Lg < Xg )
   {
      // after the end of the girder
      SegmentIndexType nSegments = GetSegmentCount(girderKey);
      CSegmentKey segmentKey(girderKey,nSegments-1);
      Float64 Ls = GetSegmentLength(segmentKey);
      Float64 Xs = Ls + Xg - Lg;
      poi = pgsPointOfInterest(segmentKey,Xs);
   }
   else
   {
      // somewhere within the girder
      Float64 running_distance = 0;
      GroupIndexType nGroups = GetGirderGroupCount();
      SegmentIndexType nSegments = GetSegmentCount(girderKey);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(girderKey,segIdx);
         Float64 segmentLength = GetSegmentLayoutLength(segmentKey);

         // if this is the first segment, remove the start offset distance so that segmentLength
         // is measured on the same basis as Xg
         if ( segIdx == 0 )
         {
            Float64 brgOffset = GetSegmentStartBearingOffset(segmentKey);
            Float64 endDist   = GetSegmentStartEndDistance(segmentKey);
            Float64 offset_dist = brgOffset - endDist;
            offset_dist = IsZero(offset_dist) ? 0 : offset_dist;
            segmentLength -= offset_dist;
         }
         
         // if this is the last segment, remove the end offset distance so that we
         // don't go beyond the end of the girder
         if ( segIdx == nSegments-1 )
         {
            Float64 brgOffset = GetSegmentEndBearingOffset(segmentKey);
            Float64 endDist   = GetSegmentEndEndDistance(segmentKey);
            Float64 offset_dist = brgOffset - endDist;
            offset_dist = IsZero(offset_dist) ? 0 : offset_dist;
            segmentLength -= offset_dist;
         }

         if ( ::IsLE(running_distance,Xg) && ::IsLE(Xg,running_distance+segmentLength) )
         {
            // the POI occurs in this segment
            
            Float64 Xpoi = Xg - running_distance; // this is the distance from the
            // CLPier/CLTempSupport to the POI (basically a segment coordinate unless this is in
            // the first segment, then Xpoi is measured from the start face of the segment)
            if (segIdx != 0) 
            {
               // if this is not the first segment, adjust Xpoi so that it is measured from the start
               // face of the segment
               Float64 brgOffset = GetSegmentStartBearingOffset(segmentKey);
               Float64 endDist   = GetSegmentStartEndDistance(segmentKey);
               Float64 offset_dist = brgOffset - endDist;
               offset_dist = IsZero(offset_dist) ? 0 : offset_dist;
               Xpoi -= offset_dist;
               segmentLength -= offset_dist;
            }

            Xpoi = IsZero(Xpoi) ? 0 : Xpoi;
            Xpoi = IsEqual(Xpoi,segmentLength) ? segmentLength : Xpoi;

            // Gets the actual POI at this location, or returns a temporary poi if not found
            poi = GetPointOfInterest(segmentKey,Xpoi,tolerance);
            ATLASSERT(IsEqual(poi.GetDistFromStart(),Xpoi,tolerance));
            break;
         }
         else
         {
            running_distance += segmentLength;
         }
      }
   }

   ATLASSERT(poi.GetSegmentKey() != CSegmentKey());// should have found it or you are asking for a POI that is off the bridge

#if defined CHECK_POI_CONVERSIONS
   static bool bTesting;
   if ( !bTesting )
   {
      bTesting = true;
      Float64 testXg = ConvertPoiToGirderCoordinate(poi);
      ATLASSERT(IsEqual(Xg,testXg,tolerance));
      bTesting = false;
   }
#endif
   return poi;
}

Float64 CBridgeAgentImp::ConvertPoiToGirderCoordinate(const pgsPointOfInterest& poi)
{
   Float64 Xg = 0;
   if ( poi.HasGirderCoordinate() )
   {
      Xg = poi.GetGirderCoordinate();

#if defined CHECK_POI_CONVERSIONS
      pgsPointOfInterest testPoi;
      testPoi.SetLocation(poi.GetSegmentKey(),poi.GetDistFromStart());
      Float64 XgTest = ConvertPoiToGirderCoordinate(testPoi);
      ATLASSERT(IsEqual(Xg,XgTest,gs_PoiTolerance));
#endif
   }
   else
   {
      Float64 Xgp = ConvertPoiToGirderPathCoordinate(poi);

      // We want to measure from the left face of the girder so adjust for the connection geometry

      // |<------- Xgp ----->>
      // |<-- brg offset->|
      // |   |<-end dist->|
      // |<->|-- deduct this distance (start_offset) from Xgp to get Xg
      // |   |<--- Xg -------->>
      // |   |
      // |   +---------------------------------------/
      // |   |          Segment 0                    \
      // |   +---------------------------------------/
      CSegmentKey segmentKey(poi.GetSegmentKey());
      CSegmentKey firstSegmentKey(segmentKey.groupIndex,segmentKey.girderIndex,0);
      Float64 brgOffset = GetSegmentStartBearingOffset(firstSegmentKey);
      Float64 endDist   = GetSegmentStartEndDistance(firstSegmentKey);
      Float64 start_offset = brgOffset - endDist;
      start_offset = IsZero(start_offset) ? 0 : start_offset;

      Xg = Xgp - start_offset;

      if ( poi.GetID() != INVALID_ID )
      {
         pgsPointOfInterest* pPoi = const_cast<pgsPointOfInterest*>(&poi);
         pPoi->SetGirderCoordinate(Xg);
      }
   }

#if defined CHECK_POI_CONVERSIONS
   static bool bTesting = false;
   if ( !bTesting )
   {
      bTesting = true;
      pgsPointOfInterest testPoi = ConvertGirderCoordinateToPoi(poi.GetSegmentKey(),Xg);
      ATLASSERT(poi.AtSamePlace(testPoi));
      bTesting = false;
   }
#endif

   return Xg;
}

Float64 CBridgeAgentImp::ConvertGirderCoordinateToGirderPathCoordinate(const CGirderKey& girderKey,Float64 Xg)
{
   // |<------- Xgp ----->>
   // |<-- brg offset->|
   // |   |<-end dist->|
   // |<->|-- add this distance (start_offset) to Xg to get Xgp
   // |   |<--- Xg -------->>
   // |   |
   // |   +---------------------------------------/
   // |   |          Segment 0                    \
   // |   +---------------------------------------/

   CSegmentKey segmentKey(girderKey,0);
   Float64 brgOffset = GetSegmentStartBearingOffset(segmentKey);
   Float64 endDist   = GetSegmentStartEndDistance(segmentKey);
   Float64 offset_dist = brgOffset - endDist;
   offset_dist = IsZero(offset_dist) ? 0 : offset_dist;
   Float64 Xgp = Xg + offset_dist;
   return Xgp;
}

Float64 CBridgeAgentImp::ConvertGirderPathCoordinateToGirderCoordinate(const CGirderKey& girderKey,Float64 Xgp)
{
   // |<------- Xgp ----->>
   // |<-- brg offset->|
   // |   |<-end dist->|
   // |<->|-- deduct this distance (start_offset) from Xgp to get Xg
   // |   |<--- Xg -------->>
   // |   |
   // |   +---------------------------------------/
   // |   |          Segment 0                    \
   // |   +---------------------------------------/

   CSegmentKey segmentKey(girderKey,0);
   Float64 brgOffset = GetSegmentStartBearingOffset(segmentKey);
   Float64 endDist   = GetSegmentStartEndDistance(segmentKey);
   Float64 offset_dist = brgOffset - endDist;
   offset_dist = IsZero(offset_dist) ? 0 : offset_dist;
   Float64 Xg = Xgp - offset_dist;
   return Xg;
}

Float64 CBridgeAgentImp::ConvertGirderPathCoordinateToGirderlineCoordinate(const CGirderKey& girderKey,Float64 Xgp)
{
   pgsPointOfInterest poi = ConvertGirderPathCoordinateToPoi(girderKey,Xgp);
   return ConvertPoiToGirderlineCoordinate(poi);
}

Float64 CBridgeAgentImp::ConvertPoiToGirderlineCoordinate(const pgsPointOfInterest& poi)
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   if ( segmentKey.groupIndex == 0 )
   {
      return ConvertPoiToGirderCoordinate(poi);
   }

   Float64 Xgp = ConvertPoiToGirderPathCoordinate(poi);
   Float64 sum_girder_layout_length = 0; // sum of girder length from start of bridge up to but not including the group
   // this poi is in
   for ( GroupIndexType grpIdx = 0; grpIdx < segmentKey.groupIndex; grpIdx++ )
   {
      CGirderKey girderKey(grpIdx,segmentKey.girderIndex);
      Float64 girder_layout_length = GetGirderLayoutLength(girderKey);
      sum_girder_layout_length += girder_layout_length;
   }

   // sum_girder_layout_length is measured from CL Pier 0, we want it measured from the start face of the girder
   Float64 brg_offset = GetSegmentStartBearingOffset(CSegmentKey(0,segmentKey.girderIndex,0));
   Float64 end_dist   = GetSegmentStartEndDistance(CSegmentKey(0,segmentKey.girderIndex,0));
   Float64 start_offset = brg_offset - end_dist; // distance from CL Pier 0 to start face of girder
   sum_girder_layout_length -= start_offset;

   Float64 Xgl = Xgp + sum_girder_layout_length;
   return Xgl;
}

pgsPointOfInterest CBridgeAgentImp::ConvertGirderlineCoordinateToPoi(GirderIndexType gdrIdx,Float64 Xgl,Float64 tolerance)
{
   // It will be easier to find which grouop Xgl is located in if we
   // convert it to a measurement from the Pier line at abutment 0.
   CSegmentKey segmentKey(0,gdrIdx,0);
   Float64 brg_offset = GetSegmentStartBearingOffset(segmentKey);
   Float64 end_dist   = GetSegmentStartEndDistance(segmentKey);
   Float64 start_offset = brg_offset - end_dist;
   Float64 X = Xgl + start_offset;

   GroupIndexType grpIdx = 0;
   Float64 Xstart = 0; // distance from Pier 0 to start of current group
   if ( X < 0 )
   {
      grpIdx = 0;
   }
   else
   {
      // Search for the group where the point under consideration occurs
      GroupIndexType nGroups = GetGirderGroupCount();
      Float64 Xend   = 0; // distance from Pier 0 to end of current group
      for ( grpIdx = 0; grpIdx < nGroups; grpIdx++ )
      {
         CGirderKey girderKey(grpIdx,gdrIdx);
         Xend += GetGirderLayoutLength(girderKey); // update end of current group

         // is target point in this group?
         if ( ::InRange(Xstart,X,Xend) )
         {
            // we found the group
            break;
         }

         Xstart = Xend; // next group starts where current group ends
      }

      if ( nGroups <= grpIdx )
      {
         ATLASSERT(false); // if this fires, the group wasn't found
         grpIdx = nGroups - 1; // use the last group
      }
   }

   Float64 Xgp = X - Xstart;
   CGirderKey girderKey(grpIdx,gdrIdx);
   return ConvertGirderPathCoordinateToPoi(girderKey,Xgp,tolerance);
}

Float64 CBridgeAgentImp::ConvertRouteToBridgeLineCoordinate(Float64 station)
{
   VALIDATE( BRIDGE );
   CComPtr<IPierCollection> piers;
   m_Bridge->get_Piers(&piers);

   CComPtr<IBridgePier> pier;
   piers->get_Item(0,&pier);

   CComPtr<IStation> objStation;
   pier->get_Station(&objStation);

   Float64 sta_value;
   objStation->get_Value(&sta_value);

   Float64 dist = station - sta_value;

   return dist;
}

Float64 CBridgeAgentImp::ConvertBridgeLineToRouteCoordinate(Float64 Xb)
{
   Float64 station = GetPierStation(0);
   Float64 Xr = station + Xb;
   return Xr;
}

Float64 CBridgeAgentImp::ConvertPoiToBridgeLineCoordinate(const pgsPointOfInterest& poi)
{
   Float64 station,offset;
   GetStationAndOffset(poi,&station,&offset);
   return ConvertRouteToBridgeLineCoordinate(station);
}

PoiAttributeType g_TargetAttribute;
PoiAttributeType g_ExceptionAttribute;
bool RemovePOI(const pgsPointOfInterest& poi)
{
   return poi.HasAttribute(g_TargetAttribute) && !poi.HasAttribute(g_ExceptionAttribute);
}

void CBridgeAgentImp::RemovePointsOfInterest(std::vector<pgsPointOfInterest>& vPoi,PoiAttributeType targetAttribute,PoiAttributeType exceptionAttribute)
{
   g_TargetAttribute = targetAttribute;
   g_ExceptionAttribute  = exceptionAttribute;
   std::vector<pgsPointOfInterest>::iterator new_end = std::remove_if(vPoi.begin(),vPoi.end(),RemovePOI);
   std::vector<pgsPointOfInterest>::size_type newSize = new_end - vPoi.begin();
   vPoi.resize(newSize);
}

bool CBridgeAgentImp::IsInClosureJoint(const pgsPointOfInterest& poi,CClosureKey* pClosureKey)
{
   CSegmentKey segmentKey(poi.GetSegmentKey());
   Float64 Xpoi = poi.GetDistFromStart();

   bool bAdjustSegmentIndex = false;
   if ( Xpoi < 0 && 0 < segmentKey.segmentIndex )
   {
      // POI is before the start of this segment so the closure pour
      // test has to be based on the previous segment
      segmentKey.segmentIndex--; // adjust the segment index
      bAdjustSegmentIndex = true;
   }

   const CClosureKey& closureKey(segmentKey);
   Float64 Lc = (segmentKey.segmentIndex == GetSegmentCount(segmentKey)-1 ? 0 : GetClosureJointLength(closureKey));
   Float64 Ls = GetSegmentLength(segmentKey);

   if ( Xpoi < 0 && bAdjustSegmentIndex )
   {
      // POI location needs to be adjusted because we switched to the previous segment
      // Xpoi is located from the start of the previous segment
      //
      //                               CL Closure Joint
      //                               :
      //                               : |<->| Xpoi < 0
      // +-------------------------+   : *   +-----------------------------\
      // |                         |   :     |                             /
      // +-------------------------+   :     +-----------------------------\
      // |    Adjusted Xpoi              |   |
      // |<----------------------------->|   |
      // |          Ls             |  Lc     |
      // |<----------------------->|<------->|

      Xpoi = Ls + Lc + Xpoi;
   }

#if defined _DEBUG
   if ( poi.HasAttribute(POI_CLOSURE) )
   {
      Float64 left, right;
      GetClosureJointSize(closureKey,&left,&right);
      ATLASSERT(::IsEqual(Ls+left,Xpoi));
   }
#endif


   // NOTE: don't use <= , ::InRange(), or ::LE() because if Xpoi is exactly
   // at the interface between the segment and the closure, we consider this
   // being in the segment.
   bool bIsInClosure = (Ls < Xpoi && Xpoi < Ls+Lc) ? true : false;
   if ( bIsInClosure )
   {
      *pClosureKey = closureKey;
   }
   else
   {
      *pClosureKey = CClosureKey(INVALID_INDEX,INVALID_INDEX,INVALID_INDEX);
   }

   return bIsInClosure;
}

bool CBridgeAgentImp::IsOnSegment(const pgsPointOfInterest& poi)
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   Float64 Ls = GetSegmentLength(segmentKey);
   Float64 Xpoi = poi.GetDistFromStart();
   return ::InRange(0.0,Xpoi,Ls) ? true : false;
}

bool CBridgeAgentImp::IsOffSegment(const pgsPointOfInterest& poi)
{
   return !IsOnSegment(poi);
}

bool CBridgeAgentImp::IsOnGirder(const pgsPointOfInterest& poi)
{
   const CGirderKey& girderKey(poi.GetSegmentKey());
   Float64 Lg = GetGirderLength(girderKey);
   Float64 Xg = ConvertPoiToGirderCoordinate(poi);
   return ::InRange(0.0,Xg,Lg) ? true : false;
}

bool CBridgeAgentImp::IsInIntermediateDiaphragm(const pgsPointOfInterest& poi)
{
   return !IsOnGirder(poi);
}

bool CBridgeAgentImp::IsInCriticalSectionZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState)
{
   ATLASSERT(IsStrengthLimitState(limitState));
   PoiAttributeType csAttribute = (IsStrengthILimitState(limitState) ? POI_CRITSECTSHEAR1 : POI_CRITSECTSHEAR2);

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   // LRFD 2004 and later, critical section is only a function of dv, which comes from the calculation of Mu,
   // so critical section is not a function of the limit state. We will work with the Strength I limit state
   if ( lrfdVersionMgr::ThirdEdition2004 <= pSpecEntry->GetSpecificationType() )
   {
      csAttribute = POI_CRITSECTSHEAR1;
   }

   if ( poi.HasAttribute(csAttribute) )
   {
      // this may seem backwards... may seem like true should be returned, but false is correct.
      // The intent of this method is to determine of a poi is located such that shear requirements
      // are applicable. Returning true means that the poi is located somewhere between the face of
      // support and the CS and the shear requirements are not applicable. Shear requirements
      // are applicable at the CS so we return false here so this poi doesn't get ignored
      return false;
   }

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   std::vector<pgsPointOfInterest> vPoi = GetCriticalSections(limitState,segmentKey);
   ATLASSERT( 2 <= vPoi.size() );

   Float64 Xgl = ConvertPoiToGirderlineCoordinate(poi);

   //
   // The critical section for shear locations alternate along the length of the girder
   // Critical sections are near supports and not in the center portion of spans.
   //
   // The vector of critical section POI contains the CS locations that occur
   // at the boundary of the CS zones.
   // We will walk the vector zone by zone to see if the subject POI is between
   // two CS zone boundaries
   //
   // Yes      No          Yes         No      Yes       No      Yes
   // ----|-----------|------------|-------|----------|--------|-----
   // O                      ^                  ^                   O
   //     ^           ^
   //     |           |
   //                 ^            ^
   //                 |            |
   //                              ^       ^
   //                              |       | ... etc
   //
   // The first check, the poi will not be in a CS zone. The second check, the poi will be
   // in a CS zone... and so on. The in/out of a CS zone alternates.

   bool bIsInCSZone = false;
   std::vector<pgsPointOfInterest>::iterator iter = vPoi.begin();
   ATLASSERT((*iter).HasAttribute(csAttribute));
   Float64 Xprev = ConvertPoiToGirderlineCoordinate(*iter++);
   std::vector<pgsPointOfInterest>::iterator endIter = vPoi.end();
   for ( ; iter != endIter; iter++, bIsInCSZone = !bIsInCSZone )
   {
      pgsPointOfInterest& nextPoi(*iter);
      ATLASSERT(nextPoi.HasAttribute(csAttribute));
      Float64 Xnext = ConvertPoiToGirderlineCoordinate(nextPoi);
      if ( ::InRange(Xprev,Xgl,Xnext) )
      {
         return bIsInCSZone;
      }

      Xprev = Xnext;
   }

   // if we got this far, the poi is either near the start of the end of the girder in which
   // case it is in a critical section zone.
   return true;
}

/////////////////////////////////////////////////////////////////////////
// ISectionProperties
//
pgsTypes::SectionPropertyMode CBridgeAgentImp::GetSectionPropertiesMode()
{
   GET_IFACE(ILibrary,       pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   pgsTypes::SectionPropertyMode sectPropMode = pSpecEntry->GetSectionPropertyMode();
   return sectPropMode;
}

Float64 CBridgeAgentImp::GetHg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   // Height of the girder is invariant to the section property type. The height will
   // always be the same for gross and transformed properties.
   pgsTypes::SectionPropertyType sectPropType = GetSectionPropertiesType();

   SectProp& props = GetSectionProperties(intervalIdx,poi,sectPropType);
   Float64 Yt, Yb;
   props.ShapeProps->get_Ytop(&Yt);
   props.ShapeProps->get_Ybottom(&Yb);

   return Yt+Yb;
}

Float64 CBridgeAgentImp::GetAg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   pgsTypes::SectionPropertyType sectPropType = GetSectionPropertiesType();
   return GetAg(sectPropType,intervalIdx,poi);
}

Float64 CBridgeAgentImp::GetIx(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   pgsTypes::SectionPropertyType sectPropType = GetSectionPropertiesType();
   return GetIx(sectPropType,intervalIdx,poi);
}

Float64 CBridgeAgentImp::GetIy(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   pgsTypes::SectionPropertyType sectPropType = GetSectionPropertiesType();
   return GetIy(sectPropType,intervalIdx,poi);
}

Float64 CBridgeAgentImp::GetY(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location)
{
   pgsTypes::SectionPropertyType sectPropType = GetSectionPropertiesType();
   return GetY(sectPropType,intervalIdx,poi,location);
}

Float64 CBridgeAgentImp::GetS(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location)
{
   pgsTypes::SectionPropertyType sectPropType = GetSectionPropertiesType();
   return GetS(sectPropType,intervalIdx,poi,location);
}

Float64 CBridgeAgentImp::GetKt(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   pgsTypes::SectionPropertyType sectPropType = GetSectionPropertiesType();
   return GetKt(sectPropType,intervalIdx,poi);
}

Float64 CBridgeAgentImp::GetKb(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   pgsTypes::SectionPropertyType sectPropType = GetSectionPropertiesType();
   return GetKb(sectPropType,intervalIdx,poi);
}

Float64 CBridgeAgentImp::GetEIx(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   pgsTypes::SectionPropertyType sectPropType = GetSectionPropertiesType();
   return GetEIx(sectPropType,intervalIdx,poi);
}

Float64 CBridgeAgentImp::GetAg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr)
{
   pgsTypes::SectionPropertyType sectPropType = GetSectionPropertiesType();
   return GetAg(sectPropType,intervalIdx,poi,fcgdr);
}

Float64 CBridgeAgentImp::GetIx(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr)
{
   pgsTypes::SectionPropertyType sectPropType = GetSectionPropertiesType();
   return GetIx(sectPropType,intervalIdx,poi,fcgdr);
}

Float64 CBridgeAgentImp::GetIy(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr)
{
   pgsTypes::SectionPropertyType sectPropType = GetSectionPropertiesType();
   return GetIy(sectPropType,intervalIdx,poi,fcgdr);
}

Float64 CBridgeAgentImp::GetY(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,Float64 fcgdr)
{
   pgsTypes::SectionPropertyType sectPropType = GetSectionPropertiesType();
   return GetY(sectPropType,intervalIdx,poi,location,fcgdr);
}

Float64 CBridgeAgentImp::GetS(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,Float64 fcgdr)
{
   pgsTypes::SectionPropertyType sectPropType = GetSectionPropertiesType();
   return GetS(sectPropType,intervalIdx,poi,location,fcgdr);
}

Float64 CBridgeAgentImp::GetHg(pgsTypes::SectionPropertyType sectPropType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(intervalIdx,poi,sectPropType);
   Float64 Yt, Yb;
   props.ShapeProps->get_Ytop(&Yt);
   props.ShapeProps->get_Ybottom(&Yb);

   return Yt+Yb;
}

Float64 CBridgeAgentImp::GetAg(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(intervalIdx,poi,spType);
   Float64 area;
   props.ShapeProps->get_Area(&area);
   return area;
}

Float64 CBridgeAgentImp::GetIx(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(intervalIdx,poi,spType);
   Float64 ixx;
   props.ShapeProps->get_Ixx(&ixx);
   return ixx;
}

Float64 CBridgeAgentImp::GetIy(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(intervalIdx,poi,spType);
   Float64 iyy;
   props.ShapeProps->get_Iyy(&iyy);
   return iyy;
}

Float64 CBridgeAgentImp::GetY(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location)
{
   SectProp& props = GetSectionProperties(intervalIdx,poi,spType);

   Float64 Y;
   switch (location )
   {
   case pgsTypes::TopGirder:
   case pgsTypes::BottomDeck:
      // top of girder and bottom of deck are at the same location
      Y = props.YtopGirder;
      break;

   case pgsTypes::BottomGirder:
      props.ShapeProps->get_Ybottom(&Y);
      break;

   case pgsTypes::TopDeck:
      {
         IntervalIndexType compositeDeckIntervalIdx = GetCompositeDeckInterval();
         if ( compositeDeckIntervalIdx <= intervalIdx && IsCompositeDeck() )
         {
            props.ShapeProps->get_Ytop(&Y);
         }
         else
         {
            Y = props.YtopGirder;
         }
      }
   }

   return Y;
}

Float64 CBridgeAgentImp::GetS(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location)
{
   Float64 Ix = GetIx(spType,intervalIdx,poi);
   Float64 Y  = GetY(spType,intervalIdx,poi,location);
   
   Float64 S = (IsZero(Y) ? 0 : Ix/Y);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   // S is negative above the neutral axis that way
   // M/S results in compression top for positive moment (compression is < 0)
   if ( location != pgsTypes::BottomGirder )
   {
      S *= -1;
   }

   if ( IsDeckStressLocation(location) )
   {
      IntervalIndexType compositeDeckInterval = m_IntervalManager.GetCompositeDeckInterval();
      if (  compositeDeckInterval <= intervalIdx && IsCompositeDeck() // and the deck is composite with the section
         )
      {
         // make S be in terms of deck material
         CClosureKey closureKey;
         Float64 Eg = (IsInClosureJoint(poi,&closureKey) ? GetClosureJointEc(closureKey,intervalIdx) : GetSegmentEc(segmentKey,intervalIdx));
         Float64 Es = GetDeckEc(intervalIdx);

         Float64 n = Eg/Es;
         S *= n;
      }
      else
      {
         // no deck, deck isn't composite yet, 
         S = 0;
      }
   }

   return S;
}

Float64 CBridgeAgentImp::GetKt(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   Float64 Sb = fabs(GetS(spType,intervalIdx,poi,pgsTypes::BottomGirder));
   Float64 A  = GetAg(spType,intervalIdx,poi);
   Float64 Kt = IsZero(A) ? 0 : Sb/A ;
   return Kt;
}

Float64 CBridgeAgentImp::GetKb(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   pgsTypes::StressLocation topLocation = pgsTypes::TopGirder;
   IntervalIndexType compositeDeckIntervalIdx = GetCompositeDeckInterval();
   if ( compositeDeckIntervalIdx <= intervalIdx && IsCompositeDeck() )
   {
      pgsTypes::TopDeck;
   }

   Float64 St = fabs(GetS(spType,intervalIdx,poi,topLocation));
   Float64 A  = GetAg(spType,intervalIdx,poi);
   Float64 Kb = IsZero(A) ? 0 : St/A ;
   return Kb;
}

Float64 CBridgeAgentImp::GetEIx(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(intervalIdx,poi,spType);

   Float64 EIx;
   props.ElasticProps->get_EIxx(&EIx);
   return EIx;
}

Float64 CBridgeAgentImp::GetAg(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   bool bEcChanged;
   Float64 E = GetSegmentEc(segmentKey,intervalIdx,fcgdr,&bEcChanged);

   // if the "trial" girder strength is the same as the real girder strength
   // don't do a bunch of extra work. Return the properties for the real girder
   if ( !bEcChanged )
   {
      return GetAg(spType,intervalIdx,poi);
   }

   CComPtr<IShapeProperties> sprops;
   GetShapeProperties(spType,intervalIdx,poi,E,&sprops);

   Float64 Ag;
   sprops->get_Area(&Ag);
   return Ag;
}

Float64 CBridgeAgentImp::GetIx(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   // Compute what the secant modulus would be if f'c = fcgdr
   bool bEcChanged;
   Float64 E = GetSegmentEc(segmentKey,intervalIdx,fcgdr,&bEcChanged);

   // if the "trial" girder strength is the same as the real girder strength
   // don't do a bunch of extra work. Return the properties for the real girder
   if ( !bEcChanged )
   {
      return GetIx(spType,intervalIdx,poi);
   }

   CComPtr<IShapeProperties> sprops;
   GetShapeProperties(spType,intervalIdx,poi,E,&sprops);

   Float64 Ix;
   sprops->get_Ixx(&Ix);
   return Ix;
}

Float64 CBridgeAgentImp::GetIy(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   bool bEcChanged;
   Float64 E = GetSegmentEc(segmentKey,intervalIdx,fcgdr,&bEcChanged);

   // if the "trial" girder strength is the same as the real girder strength
   // don't do a bunch of extra work. Return the properties for the real girder
   if ( !bEcChanged )
   {
      return GetIy(spType,intervalIdx,poi);
   }

   CComPtr<IShapeProperties> sprops;
   GetShapeProperties(spType,intervalIdx,poi,E,&sprops);

   Float64 Iy;
   sprops->get_Iyy(&Iy);
   return Iy;
}

Float64 CBridgeAgentImp::GetY(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,Float64 fcgdr)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   bool bEcChanged;
   Float64 E = GetSegmentEc(segmentKey,intervalIdx,fcgdr,&bEcChanged);

   // if the "trial" girder strength is the same as the real girder strength
   // don't do a bunch of extra work. Return the properties for the real girder
   if ( !bEcChanged )
   {
      return GetY(spType,intervalIdx,poi,location);
   }

   CComPtr<IShapeProperties> sprops;
   GetShapeProperties(spType,intervalIdx,poi,E,&sprops);

   Float64 Y = ComputeY(intervalIdx,poi,location,sprops);

   return Y;
}

Float64 CBridgeAgentImp::GetS(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,Float64 fcgdr)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   bool bEcChanged;
   Float64 E = GetSegmentEc(segmentKey,intervalIdx,fcgdr,&bEcChanged);

   // if the "trial" girder strength is the same as the real girder strength
   // don't do a bunch of extra work. Return the properties for the real girder
   if ( !bEcChanged )
   {
      return GetS(spType,intervalIdx,poi,location);
   }

   CComPtr<IShapeProperties> sprops;
   GetShapeProperties(spType,intervalIdx,poi,E,&sprops);
   Float64 ixx;
   sprops->get_Ixx(&ixx);

   Float64 y = ComputeY(intervalIdx,poi,location,sprops);

   Float64 S = (IsZero(y) ? 0 : ixx/y);

   // S is negative above the neutral axis that way
   // M/S results in compression top for positive moment (compression is < 0)
   if ( location != pgsTypes::BottomGirder )
   {
      S *= -1;
   }

   IntervalIndexType compositeDeckInterval = m_IntervalManager.GetCompositeDeckInterval();
   if ( (location == pgsTypes::TopDeck || location == pgsTypes::BottomDeck) && // want S for deck
         compositeDeckInterval <= intervalIdx && IsCompositeDeck() // and the deck is composite with the section
      )
   {
      // make S be in terms of deck material
      Float64 Eg = E;
      Float64 Es = GetDeckEc(intervalIdx);

      Float64 n = Eg/Es;
      S *= n;
   }

   return S;
}

Float64 CBridgeAgentImp::GetNetAg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(intervalIdx,poi,pgsTypes::sptNetGirder);
   Float64 area;
   props.ShapeProps->get_Area(&area);
   return area;
}

Float64 CBridgeAgentImp::GetNetIg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(intervalIdx,poi,pgsTypes::sptNetGirder);
   Float64 ixx;
   props.ShapeProps->get_Ixx(&ixx);
   return ixx;
}

Float64 CBridgeAgentImp::GetNetYbg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(intervalIdx,poi,pgsTypes::sptNetGirder);
   Float64 yb;
   props.ShapeProps->get_Ybottom(&yb);
   return yb;
}

Float64 CBridgeAgentImp::GetNetYtg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(intervalIdx,poi,pgsTypes::sptNetGirder);
   Float64 yt;
   props.ShapeProps->get_Ytop(&yt);
   return yt;
}

Float64 CBridgeAgentImp::GetNetAd(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(intervalIdx,poi,pgsTypes::sptNetDeck);
   Float64 area;
   props.ShapeProps->get_Area(&area);
   return area;
}

Float64 CBridgeAgentImp::GetNetId(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(intervalIdx,poi,pgsTypes::sptNetDeck);
   Float64 ixx;
   props.ShapeProps->get_Ixx(&ixx);
   return ixx;
}

Float64 CBridgeAgentImp::GetNetYbd(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(intervalIdx,poi,pgsTypes::sptNetDeck);
   Float64 yb;
   props.ShapeProps->get_Ybottom(&yb);
   return yb;
}

Float64 CBridgeAgentImp::GetNetYtd(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   SectProp& props = GetSectionProperties(intervalIdx,poi,pgsTypes::sptNetDeck);
   Float64 yt;
   props.ShapeProps->get_Ytop(&yt);
   return yt;
}

Float64 CBridgeAgentImp::GetQSlab(const pgsPointOfInterest& poi)
{
   pgsTypes::SectionPropertyType sectPropType = GetSectionPropertiesType();

   IntervalIndexType compositeDeckIntervalIdx = m_IntervalManager.GetCompositeDeckInterval();
   SectProp& props = GetSectionProperties(compositeDeckIntervalIdx,poi,sectPropType);
   return props.Qslab;
}

Float64 CBridgeAgentImp::GetAcBottomHalf(const pgsPointOfInterest& poi)
{
   pgsTypes::SectionPropertyType sectPropType = GetSectionPropertiesType();

   IntervalIndexType compositeDeckInterval = m_IntervalManager.GetCompositeDeckInterval();
   SectProp& props = GetSectionProperties(compositeDeckInterval,poi,sectPropType);
   return props.AcBottomHalf;
}

Float64 CBridgeAgentImp::GetAcTopHalf(const pgsPointOfInterest& poi)
{
   pgsTypes::SectionPropertyType sectPropType = GetSectionPropertiesType();

   IntervalIndexType compositeDeckInterval = m_IntervalManager.GetCompositeDeckInterval();
   SectProp& props = GetSectionProperties(compositeDeckInterval,poi,sectPropType);
   return props.AcTopHalf;
}

Float64 CBridgeAgentImp::GetTributaryFlangeWidth(const pgsPointOfInterest& poi)
{
   Float64 tfw = 0;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   if ( IsCompositeDeck() )
   {
      GirderIDType leftGdrID, gdrID, rightGdrID;
      GetAdjacentSuperstructureMemberIDs(segmentKey,&leftGdrID,&gdrID,&rightGdrID);
      Float64 Xs = poi.GetDistFromStart();
      HRESULT hr = m_EffFlangeWidthTool->TributaryFlangeWidthBySegment(m_Bridge,gdrID,segmentKey.segmentIndex,Xs,leftGdrID,rightGdrID,&tfw);
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
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      GirderIDType leftGdrID,gdrID,rightGdrID;
      GetAdjacentSuperstructureMemberIDs(segmentKey,&leftGdrID,&gdrID,&rightGdrID);

      Float64 Xs = poi.GetDistFromStart();
      HRESULT hr = m_EffFlangeWidthTool->TributaryFlangeWidthBySegmentEx(m_Bridge,gdrID,segmentKey.segmentIndex,Xs,leftGdrID,rightGdrID,pLftFw,pRgtFw,&tfw);
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
         const CSegmentKey& segmentKey = poi.GetSegmentKey();

         GirderIDType leftGdrID,gdrID,rightGdrID;
         GetAdjacentSuperstructureMemberIDs(segmentKey,&leftGdrID,&gdrID,&rightGdrID);

         Float64 Xg = ConvertPoiToGirderCoordinate(poi);
         HRESULT hr = m_EffFlangeWidthTool->EffectiveFlangeWidthBySSMbr(m_Bridge,gdrID,Xg,leftGdrID,rightGdrID,&efw);
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

void CBridgeAgentImp::ReportEffectiveFlangeWidth(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
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
   report->ReportEffectiveFlangeWidth(m_pBroker,m_Bridge,girderKey,pChapter,pDisplayUnits);
}

Float64 CBridgeAgentImp::GetPerimeter(const pgsPointOfInterest& poi)
{
   // want the perimeter of the plain girder...
   IntervalIndexType intervalIdx;
   CClosureKey closureKey;
   if ( IsInClosureJoint(poi,&closureKey) )
   {
      intervalIdx = GetCompositeClosureJointInterval(closureKey);
   }
   else
   {
      intervalIdx = GetPrestressReleaseInterval(poi.GetSegmentKey());
   }

   SectProp& props = GetSectionProperties(intervalIdx,poi,pgsTypes::sptNetGirder);
   return props.Perimeter;
}

Float64 CBridgeAgentImp::GetSegmentSurfaceArea(const CSegmentKey& segmentKey)
{
   // Because of the time-step analysis and age adjusted concrete, we must
   // compute surface area without the aid of the generic bridge model
   IntervalIndexType releaseIntervalIdx = GetPrestressReleaseInterval(segmentKey);

   std::vector<pgsPointOfInterest> vPOI( GetPointsOfInterest(segmentKey,POI_SECTCHANGE) );
   ATLASSERT( 2 <= vPOI.size() );
   Float64 S = 0;
   std::vector<pgsPointOfInterest>::iterator iter( vPOI.begin() );
   pgsPointOfInterest prev_poi = *iter;

   CComPtr<IShape> shape;
   GetSegmentShape(releaseIntervalIdx,prev_poi,true,pgsTypes::scGirder,&shape);
   Float64 prev_perimeter;
   shape->get_Perimeter(&prev_perimeter);

   CComPtr<IShapeProperties> shapeProps;
   shape->get_ShapeProperties(&shapeProps);
   Float64 start_area;
   shapeProps->get_Area(&start_area);

   iter++;

   std::vector<pgsPointOfInterest>::const_iterator end(vPOI.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;

      shape.Release();
      GetSegmentShape(releaseIntervalIdx,poi,true,pgsTypes::scGirder,&shape);
      Float64 perimeter;
      shape->get_Perimeter(&perimeter);

      Float64 avg_perimeter = (prev_perimeter + perimeter)/2;
      S += avg_perimeter*(poi.GetDistFromStart() - prev_poi.GetDistFromStart());

      prev_poi = poi;
      prev_perimeter = perimeter;
   }

   shapeProps.Release();
   shape->get_ShapeProperties(&shapeProps);
   Float64 end_area;
   shapeProps->get_Area(&end_area);

   S += (start_area + end_area);

   const GirderLibraryEntry* pGdrEntry = GetGirderLibraryEntry(segmentKey);

   CComPtr<IBeamFactory> factory;
   pGdrEntry->GetBeamFactory(&factory);

   Float64 Sv = factory->GetInternalSurfaceAreaOfVoids(m_pBroker,segmentKey);

   // The surface area of internal voids is reduce by 50% per LRFD 5.4.2.3.2
   Sv *= 0.5;

   S += Sv;

   return S;
}

Float64 CBridgeAgentImp::GetSegmentVolume(const CSegmentKey& segmentKey)
{
   // Because of the time-step analysis and age adjusted concrete, we must
   // compute volume without the aid of the generic bridge model
   IntervalIndexType releaseIntervalIdx = GetPrestressReleaseInterval(segmentKey);

   std::vector<pgsPointOfInterest> vPOI( GetPointsOfInterest(segmentKey,POI_SECTCHANGE) );
   ATLASSERT( 2 <= vPOI.size() );
   Float64 V = 0;
   std::vector<pgsPointOfInterest>::iterator iter( vPOI.begin() );
   pgsPointOfInterest prev_poi = *iter;

   CComPtr<IShape> shape;
   GetSegmentShape(releaseIntervalIdx,prev_poi,true,pgsTypes::scGirder,&shape);
   CComPtr<IShapeProperties> shapeProps;
   shape->get_ShapeProperties(&shapeProps);
   Float64 prev_area;
   shapeProps->get_Area(&prev_area);

   iter++;

   std::vector<pgsPointOfInterest>::const_iterator end(vPOI.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;

      shape.Release();
      GetSegmentShape(releaseIntervalIdx,poi,true,pgsTypes::scGirder,&shape);
      shapeProps.Release();
      shape->get_ShapeProperties(&shapeProps);
      Float64 area;
      shapeProps->get_Area(&area);

      Float64 avg_area = (prev_area + area)/2;
      V += avg_area*(poi.GetDistFromStart() - prev_poi.GetDistFromStart());

      prev_poi = poi;
      prev_area = area;
   }

   return V;
}

Float64 CBridgeAgentImp::GetClosureJointSurfaceArea(const CClosureKey& closureKey)
{
   IntervalIndexType compositeClosureIntervalIdx = GetCompositeClosureJointInterval(closureKey);

   std::vector<pgsPointOfInterest> vPoi(GetPointsOfInterest(closureKey,POI_CLOSURE));
   ATLASSERT(vPoi.size() == 1);
   pgsPointOfInterest closurePoi(vPoi.back());
   ATLASSERT(closurePoi.GetID() != INVALID_ID);
   Float64 L = GetClosureJointLength(closureKey);

   CComPtr<IShape> shape;
   GetSegmentShape(compositeClosureIntervalIdx,closurePoi,true,pgsTypes::scGirder,&shape);
   Float64 P;
   shape->get_Perimeter(&P);
   Float64 S = P*L;
   return S;
}

Float64 CBridgeAgentImp::GetClosureJointVolume(const CClosureKey& closureKey)
{
   IntervalIndexType compositeClosureIntervalIdx = GetCompositeClosureJointInterval(closureKey);

   std::vector<pgsPointOfInterest> vPoi(GetPointsOfInterest(closureKey,POI_CLOSURE));
   ATLASSERT(vPoi.size() == 1);
   pgsPointOfInterest closurePoi(vPoi.back());
   ATLASSERT(closurePoi.GetID() != INVALID_ID);
   Float64 L = GetClosureJointLength(closureKey);

   CComPtr<IShape> shape;
   GetSegmentShape(compositeClosureIntervalIdx,closurePoi,true,pgsTypes::scGirder,&shape);
   CComPtr<IShapeProperties> shapeProps;
   shape->get_ShapeProperties(&shapeProps);
   Float64 A;
   shapeProps->get_Area(&A);
   Float64 V = A*L;
   return V;
}

Float64 CBridgeAgentImp::GetDeckSurfaceArea()
{
   ValidateDeckParameters();
   return m_DeckSurfaceArea;
}

Float64 CBridgeAgentImp::GetDeckVolume()
{
   ValidateDeckParameters();
   return m_DeckVolume;
}

Float64 CBridgeAgentImp::GetBridgeEIxx(Float64 Xb)
{
   IntervalIndexType lastIntervalIdx = m_IntervalManager.GetIntervalCount() - 1;

   CComPtr<ISection> section;
   m_SectCutTool->CreateBridgeSection(m_Bridge,Xb,lastIntervalIdx,bscStructurallyContinuousOnly,&section);

   CComPtr<IElasticProperties> eprops;
   section->get_ElasticProperties(&eprops);

   Float64 EIxx;
   eprops->get_EIxx(&EIxx);

   return EIxx;
}

Float64 CBridgeAgentImp::GetBridgeEIyy(Float64 Xb)
{
   IntervalIndexType lastIntervalIdx = m_IntervalManager.GetIntervalCount() - 1;

   CComPtr<ISection> section;
   m_SectCutTool->CreateBridgeSection(m_Bridge,Xb,lastIntervalIdx,bscStructurallyContinuousOnly,&section);

   CComPtr<IElasticProperties> eprops;
   section->get_ElasticProperties(&eprops);

   Float64 EIyy;
   eprops->get_EIyy(&EIyy);

   return EIyy;
}

Float64 CBridgeAgentImp::GetSegmentWeightPerLength(const CSegmentKey& segmentKey)
{
   IntervalIndexType releaseIntervalIdx = GetPrestressReleaseInterval(segmentKey);
   Float64 ag = GetAg(pgsTypes::sptGross,releaseIntervalIdx,pgsPointOfInterest(segmentKey,0.00));
   Float64 dens = GetSegmentWeightDensity(segmentKey,releaseIntervalIdx);
   Float64 weight_per_length = ag * dens * unitSysUnitsMgr::GetGravitationalAcceleration();
   return weight_per_length;
}

Float64 CBridgeAgentImp::GetSegmentWeight(const CSegmentKey& segmentKey)
{
   Float64 weight_per_length = GetSegmentWeightPerLength(segmentKey);
   Float64 length = GetSegmentLength(segmentKey);
   return weight_per_length * length;
}

Float64 CBridgeAgentImp::GetSegmentHeightAtPier(const CSegmentKey& segmentKey,PierIndexType pierIdx)
{
   CComPtr<IPoint2d> pntSupport1,pntEnd1,pntBrg1,pntBrg2,pntEnd2,pntSupport2;
   GetSegmentEndPoints(segmentKey,&pntSupport1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntSupport2);

   CComPtr<IPoint2d> pntPier;
   GetSegmentPierIntersection(segmentKey,pierIdx,&pntPier);

   Float64 dist_from_start;
   pntPier->DistanceEx(pntEnd1,&dist_from_start);

   // make sure we don't go past the end of the segment since we are going to ask for
   // the segment height at the release stage.
   dist_from_start = Min(dist_from_start,GetSegmentLength(segmentKey));

   pgsPointOfInterest poi(segmentKey,dist_from_start);

   return GetHg(GetPrestressReleaseInterval(segmentKey),poi);
}

Float64 CBridgeAgentImp::GetSegmentHeightAtTemporarySupport(const CSegmentKey& segmentKey,SupportIndexType tsIdx)
{
   CComPtr<IPoint2d> pntSupport1,pntEnd1,pntBrg1,pntBrg2,pntEnd2,pntSupport2;
   GetSegmentEndPoints(segmentKey,&pntSupport1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntSupport2);

   CComPtr<IPoint2d> pntTS;
   GetSegmentTempSupportIntersection(segmentKey,tsIdx,&pntTS);

   Float64 Xpoi;
   pntTS->DistanceEx(pntEnd1,&Xpoi);

   pgsPointOfInterest poi = GetPointOfInterest(segmentKey,Xpoi);

   IntervalIndexType lastIntervalIdx = GetIntervalCount()-1;
   return GetHg(lastIntervalIdx,poi);
}

/////////////////////////////////////////////////////////////////////////
// IShapes
//
void CBridgeAgentImp::GetSegmentShape(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bOrient,pgsTypes::SectionCoordinateType coordinateType,IShape** ppShape)
{
   VALIDATE(BRIDGE);
#pragma Reminder("UPDATE: these shapes can probably be cached")
   const CSegmentKey& segmentKey  = poi.GetSegmentKey();

   Float64 Xs = poi.GetDistFromStart();

   GirderIDType leftGdrID,gdrID,rightGdrID;
   GetAdjacentSuperstructureMemberIDs(segmentKey,&leftGdrID,&gdrID,&rightGdrID);
   
   // returns a copy of the shape so we can move it around without cloning it
   HRESULT hr = m_SectCutTool->CreateGirderShapeBySegment(m_Bridge,gdrID,segmentKey.segmentIndex,Xs,leftGdrID,rightGdrID,intervalIdx,ppShape);
   ATLASSERT(SUCCEEDED(hr));

   // Right now, ppShape is in Bridge Section Coordinates
   if ( coordinateType == pgsTypes::scGirder )
   {
      // Convert to Girder Section Coordinates....
      // Move Top Center point of the bare girder to (0,0)
      CComPtr<ICompositeShape> compositeShape;
      (*ppShape)->QueryInterface(&compositeShape);
      CComPtr<ICompositeShapeItem> item;
      compositeShape->get_Item(0,&item);
      CComPtr<IShape> shape;
      item->get_Shape(&shape);
      CComQIPtr<IXYPosition> position(shape);

      CComPtr<IPoint2d> point;
      position->get_LocatorPoint(lpTopCenter,&point);

      Float64 dx,dy;
      point->Location(&dx,&dy);

      position.Release();
      (*ppShape)->QueryInterface(&position);
      position->Offset(-dx,-dy);
   }

   if ( bOrient )
   {
      Float64 orientation = GetOrientation(poi.GetSegmentKey());

      Float64 rotation_angle = -orientation;

      CComQIPtr<IXYPosition> position(*ppShape);
      CComPtr<IPoint2d> top_center;
      position->get_LocatorPoint(lpTopCenter,&top_center);
      position->RotateEx(top_center,rotation_angle);
   }
}

void CBridgeAgentImp::GetSlabShape(Float64 station,IDirection* pDirection,IShape** ppShape)
{
   VALIDATE(BRIDGE);
   SectionCutKey key;
   key.station = station;
   if ( pDirection )
   {
      pDirection->get_Value(&key.direction);
   }
   else
   {
      key.direction = 0;
   }
   ShapeContainer::iterator found = m_DeckShapes.find(key);

   if ( found != m_DeckShapes.end() )
   {
      (*ppShape) = found->second;
      (*ppShape)->AddRef();
   }
   else
   {
      HRESULT hr = m_SectCutTool->CreateSlabShape(m_Bridge,station,pDirection,ppShape);
      ATLASSERT(SUCCEEDED(hr));

      if ( *ppShape )
      {
         m_DeckShapes.insert(std::make_pair(key,CComPtr<IShape>(*ppShape)));
      }
   }
}

void CBridgeAgentImp::GetLeftTrafficBarrierShape(Float64 station,IDirection* pDirection,IShape** ppShape)
{
   VALIDATE(BRIDGE);
   SectionCutKey key;
   key.station = station;
   if ( pDirection )
   {
      pDirection->get_Value(&key.direction);
   }
   else
   {
      key.direction = 0;
   }
   ShapeContainer::iterator found = m_LeftBarrierShapes.find(key);
   if ( found != m_LeftBarrierShapes.end() )
   {
      found->second->Clone(ppShape);
   }
   else
   {
      CComPtr<IShape> shape;
      HRESULT hr = m_SectCutTool->CreateLeftBarrierShape(m_Bridge,station,pDirection,&shape);

      if ( SUCCEEDED(hr) )
      {
         m_LeftBarrierShapes.insert(std::make_pair(key,CComPtr<IShape>(shape)));
         shape->Clone(ppShape);
      }
   }
}

void CBridgeAgentImp::GetRightTrafficBarrierShape(Float64 station,IDirection* pDirection,IShape** ppShape)
{
   VALIDATE(BRIDGE);
   SectionCutKey key;
   key.station = station;
   if ( pDirection )
   {
      pDirection->get_Value(&key.direction);
   }
   else
   {
      key.direction = 0;
   }
   ShapeContainer::iterator found = m_RightBarrierShapes.find(key);
   if ( found != m_RightBarrierShapes.end() )
   {
      found->second->Clone(ppShape);
   }
   else
   {
      CComPtr<IShape> shape;
      HRESULT hr = m_SectCutTool->CreateRightBarrierShape(m_Bridge,station,pDirection,&shape);

      if ( SUCCEEDED(hr) )
      {
         m_RightBarrierShapes.insert(std::make_pair(key,CComPtr<IShape>(shape)));
         shape->Clone(ppShape);
      }
   }
}

/////////////////////////////////////////////////////////////////////////
// IBarriers
//
Float64 CBridgeAgentImp::GetAtb(pgsTypes::TrafficBarrierOrientation orientation)
{
   CComPtr<IShapeProperties> props;
   GetBarrierProperties(orientation,&props);

   if ( props == NULL )
   {
      return 0;
   }

   Float64 area;
   props->get_Area(&area);

   return area;
}

Float64 CBridgeAgentImp::GetItb(pgsTypes::TrafficBarrierOrientation orientation)
{
   CComPtr<IShapeProperties> props;
   GetBarrierProperties(orientation,&props);

   if ( props == NULL )
   {
      return 0;
   }

   Float64 Ix;
   props->get_Ixx(&Ix);

   return Ix;
}

Float64 CBridgeAgentImp::GetYbtb(pgsTypes::TrafficBarrierOrientation orientation)
{
   CComPtr<IShapeProperties> props;
   GetBarrierProperties(orientation,&props);

   if ( props == NULL )
   {
      return 0;
   }

   Float64 Yb;
   props->get_Ybottom(&Yb);

   return Yb;
}

Float64 CBridgeAgentImp::GetSidewalkWeight(pgsTypes::TrafficBarrierOrientation orientation)
{
   VALIDATE(BRIDGE);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CRailingSystem* pRailingSystem = NULL;
   if ( orientation == pgsTypes::tboLeft )
   {
      pRailingSystem = pBridgeDesc->GetLeftRailingSystem();
   }
   else
   {
      pRailingSystem = pBridgeDesc->GetRightRailingSystem();
   }

   Float64 Wsw = 0;
   if ( pRailingSystem->bUseSidewalk )
   {
      // real dl width of sidwalk
      IntervalIndexType railingSystemIntervalIdx = GetInstallRailingSystemInterval();

      Float64 intEdge, extEdge;
      GetSidewalkDeadLoadEdges(orientation, &intEdge, &extEdge);

      Float64 w = fabs(intEdge-extEdge);
      Float64 tl = pRailingSystem->LeftDepth;
      Float64 tr = pRailingSystem->RightDepth;
      Float64 area = w*(tl + tr)/2;
      Float64 density = GetRailingSystemWeightDensity(orientation,railingSystemIntervalIdx);
      Float64 mpl = area * density; // mass per length
      Float64 g = unitSysUnitsMgr::GetGravitationalAcceleration();
      Wsw = mpl * g;
   }

   return Wsw;
}

bool CBridgeAgentImp::HasSidewalk(pgsTypes::TrafficBarrierOrientation orientation)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CRailingSystem* pRailingSystem = NULL;
   if ( orientation == pgsTypes::tboLeft )
   {
      pRailingSystem = pBridgeDesc->GetLeftRailingSystem();
   }
   else
   {
      pRailingSystem = pBridgeDesc->GetRightRailingSystem();
   }

   return pRailingSystem->bUseSidewalk;
}

Float64 CBridgeAgentImp::GetExteriorBarrierWeight(pgsTypes::TrafficBarrierOrientation orientation)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CRailingSystem* pRailingSystem = NULL;
   if ( orientation == pgsTypes::tboLeft )
   {
      pRailingSystem = pBridgeDesc->GetLeftRailingSystem();
   }
   else
   {
      pRailingSystem = pBridgeDesc->GetRightRailingSystem();
   }

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

      IntervalIndexType railingSystemIntervalIdx = GetInstallRailingSystemInterval();
      Float64 density = GetRailingSystemWeightDensity(orientation,railingSystemIntervalIdx);

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
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CRailingSystem* pRailingSystem = NULL;
   if ( orientation == pgsTypes::tboLeft )
   {
      pRailingSystem = pBridgeDesc->GetLeftRailingSystem();
   }
   else
   {
      pRailingSystem = pBridgeDesc->GetRightRailingSystem();
   }

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

         IntervalIndexType railingSystemIntervalIdx = GetInstallRailingSystemInterval();
         Float64 density = GetRailingSystemWeightDensity(orientation,railingSystemIntervalIdx);
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
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CRailingSystem* pRailingSystem = NULL;
   if ( orientation == pgsTypes::tboLeft )
   {
      pRailingSystem = pBridgeDesc->GetLeftRailingSystem();
   }
   else
   {
      pRailingSystem = pBridgeDesc->GetRightRailingSystem();
   }

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
      ATLASSERT(false); // client should be checking this
      return 0.0;
   }
}


Float64 CBridgeAgentImp::GetInterfaceWidth(pgsTypes::TrafficBarrierOrientation orientation)
{
   // This is the offset from the edge of deck to the curb line (basically the connection 
   // width of the barrier)
   CComPtr<ISidewalkBarrier> barrier;

   if ( orientation == pgsTypes::tboLeft )
   {
      m_Bridge->get_LeftBarrier(&barrier);
   }
   else
   {
      m_Bridge->get_RightBarrier(&barrier);
   }

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
      ATLASSERT(false); // client should not call this if no sidewalk
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
      ATLASSERT(false); // client should not call this if no sidewalk
      *pintEdge = 0.0;
      *pextEdge = 0.0;
   }
}

pgsTypes::TrafficBarrierOrientation CBridgeAgentImp::GetNearestBarrier(const CSegmentKey& segmentKey)
{
   GirderIndexType nGirders = GetGirderCount(segmentKey.groupIndex);
   if ( segmentKey.girderIndex < nGirders/2 )
   {
      return pgsTypes::tboLeft;
   }
   else
   {
      return pgsTypes::tboRight;
   }
}

////////////////////////////////////////////////////////////////////////
// ISegmentLiftingPointsOfInterest
std::vector<pgsPointOfInterest> CBridgeAgentImp::GetLiftingPointsOfInterest(const CSegmentKey& segmentKey,PoiAttributeType attrib,Uint32 mode)
{
   ValidatePointsOfInterest(segmentKey);

   Uint32 mgrMode = (mode == POIFIND_AND ? POIMGR_AND : POIMGR_OR);

   std::vector<pgsPointOfInterest> vPoi;
   m_pPoiMgr->GetPointsOfInterest(segmentKey,POI_LIFT_SEGMENT | attrib, mgrMode,&vPoi);

   if ( attrib == 0 )
   {
      std::vector<pgsPointOfInterest> vPoi2;
      m_pPoiMgr->GetPointsOfInterest(segmentKey,POI_SECTCHANGE,POIMGR_OR,&vPoi2);
      vPoi.insert(vPoi.end(),vPoi2.begin(),vPoi2.end());
      std::sort(vPoi.begin(),vPoi.end());
      vPoi.erase(std::unique(vPoi.begin(),vPoi.end()),vPoi.end());
   }

   return vPoi;
}

std::vector<pgsPointOfInterest> CBridgeAgentImp::GetLiftingDesignPointsOfInterest(const CSegmentKey& segmentKey,Float64 overhang,PoiAttributeType attrib,Uint32 mode)
{
   // Generates points of interest for the supplied overhang.
   pgsPoiMgr poiMgr;
   LayoutHandlingPoi(segmentKey,10,overhang,overhang,attrib,POI_PICKPOINT,POI_LIFT_SEGMENT,&poiMgr);

   Uint32 mgrMode = (mode == POIFIND_AND ? POIMGR_AND : POIMGR_OR);
   std::vector<pgsPointOfInterest> poi;
   poiMgr.GetPointsOfInterest(segmentKey, attrib, mgrMode, &poi );
   return poi;
}

////////////////////////////////////////////////////////////////////////
// ISegmentHaulingPointsOfInterest
std::vector<pgsPointOfInterest> CBridgeAgentImp::GetHaulingPointsOfInterest(const CSegmentKey& segmentKey,PoiAttributeType attrib,Uint32 mode)
{
   ValidatePointsOfInterest(segmentKey);

   Uint32 mgrMode = (mode == POIFIND_AND ? POIMGR_AND : POIMGR_OR);

   std::vector<pgsPointOfInterest> vPoi;
   m_pPoiMgr->GetPointsOfInterest(segmentKey,POI_HAUL_SEGMENT | attrib, mgrMode,&vPoi);

   if ( attrib == 0 )
   {
      std::vector<pgsPointOfInterest> vPoi2;
      m_pPoiMgr->GetPointsOfInterest(segmentKey,POI_SECTCHANGE,POIMGR_OR,&vPoi2);
      vPoi.insert(vPoi.end(),vPoi2.begin(),vPoi2.end());
      std::sort(vPoi.begin(),vPoi.end());
      vPoi.erase(std::unique(vPoi.begin(),vPoi.end()),vPoi.end());
   }

   return vPoi;
}

std::vector<pgsPointOfInterest> CBridgeAgentImp::GetHaulingDesignPointsOfInterest(const CSegmentKey& segmentKey,Uint16 nPnts,Float64 leftOverhang,Float64 rightOverhang,PoiAttributeType attrib,Uint32 mode)
{
   // Generates points of interest for the supplied overhang.
   pgsPoiMgr poiMgr;
   LayoutHandlingPoi(segmentKey,nPnts,leftOverhang,rightOverhang,attrib,POI_BUNKPOINT,POI_HAUL_SEGMENT,&poiMgr);

   // add pois at harping points if harped is possible
   const GirderLibraryEntry* pGirderEntry = GetGirderLibraryEntry(segmentKey);

   StrandIndexType nhMax = pGirderEntry->GetMaxHarpedStrands();
   if ( 0 < nhMax )
   {
      CComPtr<IPrecastGirder> girder;
      GetGirder(segmentKey,&girder);

      Float64 hp1, hp2;
      girder->GetHarpingPointLocations( &hp1, &hp2 );

      poiMgr.AddPointOfInterest( pgsPointOfInterest(segmentKey,hp1,attrib | POI_HARPINGPOINT) );
      poiMgr.AddPointOfInterest( pgsPointOfInterest(segmentKey,hp2,attrib | POI_HARPINGPOINT) );
   }

   Uint32 mgrMode = (mode == POIFIND_AND ? POIMGR_AND : POIMGR_OR);
   std::vector<pgsPointOfInterest> poi;
   poiMgr.GetPointsOfInterest(segmentKey, attrib, mgrMode, &poi );
   return poi;
}

Float64 CBridgeAgentImp::GetMinimumOverhang(const CSegmentKey& segmentKey)
{
   Float64 gdrLength = GetSegmentLength(segmentKey);

   GET_IFACE(ISegmentHaulingSpecCriteria,pCriteria);
   Float64 maxDistBetweenSupports = pCriteria->GetAllowableDistanceBetweenSupports();

   Float64 minOverhang = (gdrLength - maxDistBetweenSupports)/2;
   if ( minOverhang < 0 )
   {
      minOverhang = 0;
   }

   return minOverhang;
}

////////////////////////////////////////////////////////////////////////
// IUserDefinedLoads
bool CBridgeAgentImp::DoUserLoadsExist(const CSpanKey& spanKey)
{
   IntervalIndexType nIntervals = m_IntervalManager.GetIntervalCount();
   for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
   {
      if ( DoUserLoadsExist(spanKey,intervalIdx) )
      {
         return true;
      }
   }

   return false;
}

bool CBridgeAgentImp::DoUserLoadsExist(const CGirderKey& girderKey)
{
   SpanIndexType startSpanIdx = GetGirderGroupStartSpan(girderKey.groupIndex);
   SpanIndexType endSpanIdx   = GetGirderGroupEndSpan(girderKey.groupIndex);
   for (SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      if ( DoUserLoadsExist(CSpanKey(spanIdx,girderKey.girderIndex)) )
      {
         return true;
      }
   }
   return false;
}

bool CBridgeAgentImp::DoUserLoadsExist(const CSpanKey& spanKey,IUserDefinedLoads::UserDefinedLoadCase loadCase)
{
   GroupIndexType grpIdx = GetGirderGroupIndex(spanKey.spanIndex);
   CGirderKey girderKey(grpIdx,spanKey.girderIndex);
   IntervalIndexType nIntervals = m_IntervalManager.GetIntervalCount();
   for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
   {
      if ( DoUserLoadsExist(spanKey,intervalIdx,loadCase) )
      {
         return true;
      }
   }

   return false;
}

bool CBridgeAgentImp::DoUserLoadsExist(const CGirderKey& girderKey,IUserDefinedLoads::UserDefinedLoadCase loadCase)
{
   SpanIndexType startSpanIdx = GetGirderGroupStartSpan(girderKey.groupIndex);
   SpanIndexType endSpanIdx   = GetGirderGroupEndSpan(girderKey.groupIndex);
   for (SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      if ( DoUserLoadsExist(CSpanKey(spanIdx,girderKey.girderIndex),loadCase) )
      {
         return true;
      }
   }
   return false;
}

bool CBridgeAgentImp::DoUserLoadsExist(const CSpanKey& spanKey,IntervalIndexType intervalIdx)
{
   if ( DoUserLoadsExist(spanKey,intervalIdx,IUserDefinedLoads::userDC) )
   {
      return true;
   }

   if ( DoUserLoadsExist(spanKey,intervalIdx,IUserDefinedLoads::userDW) )
   {
      return true;
   }

   if ( DoUserLoadsExist(spanKey,intervalIdx,IUserDefinedLoads::userLL_IM) )
   {
      return true;
   }

   return false;
}

bool CBridgeAgentImp::DoUserLoadsExist(const CGirderKey& girderKey,IntervalIndexType intervalIdx)
{
   SpanIndexType startSpanIdx = GetGirderGroupStartSpan(girderKey.groupIndex);
   SpanIndexType endSpanIdx   = GetGirderGroupEndSpan(girderKey.groupIndex);
   for (SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      if ( DoUserLoadsExist(CSpanKey(spanIdx,girderKey.girderIndex),intervalIdx) )
      {
         return true;
      }
   }
   return false;
}

bool CBridgeAgentImp::DoUserLoadsExist(const CSpanKey& spanKey,IntervalIndexType intervalIdx,IUserDefinedLoads::UserDefinedLoadCase loadCase)
{
   return DoUserLoadsExist(spanKey,intervalIdx,intervalIdx,loadCase);
}

bool CBridgeAgentImp::DoUserLoadsExist(const CGirderKey& girderKey,IntervalIndexType intervalIdx,IUserDefinedLoads::UserDefinedLoadCase loadCase)
{
   SpanIndexType startSpanIdx = GetGirderGroupStartSpan(girderKey.groupIndex);
   SpanIndexType endSpanIdx   = GetGirderGroupEndSpan(girderKey.groupIndex);
   for (SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      if ( DoUserLoadsExist(CSpanKey(spanIdx,girderKey.girderIndex),intervalIdx,loadCase) )
      {
         return true;
      }
   }
   return false;
}

bool CBridgeAgentImp::DoUserLoadsExist(const CSpanKey& spanKey,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx)
{
   if ( DoUserLoadsExist(spanKey,firstIntervalIdx,lastIntervalIdx,IUserDefinedLoads::userDC) )
   {
      return true;
   }

   if ( DoUserLoadsExist(spanKey,firstIntervalIdx,lastIntervalIdx,IUserDefinedLoads::userDW) )
   {
      return true;
   }

   if ( DoUserLoadsExist(spanKey,firstIntervalIdx,lastIntervalIdx,IUserDefinedLoads::userLL_IM) )
   {
      return true;
   }

   return false;
}

bool CBridgeAgentImp::DoUserLoadsExist(const CGirderKey& girderKey,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx)
{
   SpanIndexType startSpanIdx = GetGirderGroupStartSpan(girderKey.groupIndex);
   SpanIndexType endSpanIdx   = GetGirderGroupEndSpan(girderKey.groupIndex);
   for (SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      if ( DoUserLoadsExist(CSpanKey(spanIdx,girderKey.girderIndex),firstIntervalIdx,lastIntervalIdx) )
      {
         return true;
      }
   }
   return false;
}

bool CBridgeAgentImp::DoUserLoadsExist(const CSpanKey& spanKey,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx,IUserDefinedLoads::UserDefinedLoadCase loadCase)
{
   for ( IntervalIndexType intervalIdx = firstIntervalIdx; intervalIdx <= lastIntervalIdx; intervalIdx++ )
   {
      const std::vector<IUserDefinedLoads::UserPointLoad>* uplv = GetPointLoads(intervalIdx,spanKey);
      if ( uplv )
      {
         std::vector<IUserDefinedLoads::UserPointLoad>::const_iterator iter(uplv->begin());
         std::vector<IUserDefinedLoads::UserPointLoad>::const_iterator iterEnd(uplv->end());
         for ( ; iter != iterEnd; iter++ )
         {
            const IUserDefinedLoads::UserPointLoad& load(*iter);
            if ( load.m_LoadCase == loadCase )
            {
               return true;
            }
         }
      }

      const std::vector<IUserDefinedLoads::UserDistributedLoad>* udlv = GetDistributedLoads(intervalIdx,spanKey);
      if ( udlv )
      {
         std::vector<IUserDefinedLoads::UserDistributedLoad>::const_iterator iter(udlv->begin());
         std::vector<IUserDefinedLoads::UserDistributedLoad>::const_iterator iterEnd(udlv->end());
         for ( ; iter != iterEnd; iter++ )
         {
            const IUserDefinedLoads::UserDistributedLoad& load(*iter);
            if ( load.m_LoadCase == loadCase )
            {
               return true;
            }
         }
      }

      const std::vector<IUserDefinedLoads::UserMomentLoad>* umlv = GetMomentLoads(intervalIdx,spanKey);
      if ( umlv )
      {
         std::vector<IUserDefinedLoads::UserMomentLoad>::const_iterator iter(umlv->begin());
         std::vector<IUserDefinedLoads::UserMomentLoad>::const_iterator iterEnd(umlv->end());
         for ( ; iter != iterEnd; iter++ )
         {
            const IUserDefinedLoads::UserMomentLoad& load(*iter);
            if ( load.m_LoadCase == loadCase )
            {
               return true;
            }
         }
      }
   }

   return false;
}

bool CBridgeAgentImp::DoUserLoadsExist(const CGirderKey& girderKey,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx,IUserDefinedLoads::UserDefinedLoadCase loadCase)
{
   SpanIndexType startSpanIdx = GetGirderGroupStartSpan(girderKey.groupIndex);
   SpanIndexType endSpanIdx   = GetGirderGroupEndSpan(girderKey.groupIndex);
   for (SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      if ( DoUserLoadsExist(CSpanKey(spanIdx,girderKey.girderIndex),firstIntervalIdx,lastIntervalIdx,loadCase) )
      {
         return true;
      }
   }
   return false;
}

const std::vector<IUserDefinedLoads::UserPointLoad>* CBridgeAgentImp::GetPointLoads(IntervalIndexType intervalIdx,const CSpanKey& spanKey)
{
   VALIDATE(LOADS);

   CUserLoadKey key(spanKey,intervalIdx);
   std::map<CUserLoadKey,std::vector<UserPointLoad>>::iterator found( m_PointLoads.find(key) );

   if (found == m_PointLoads.end())
   {
      return NULL;
   }

   return &(found->second);
}

const std::vector<IUserDefinedLoads::UserDistributedLoad>* CBridgeAgentImp::GetDistributedLoads(IntervalIndexType intervalIdx,const CSpanKey& spanKey)
{
   VALIDATE(LOADS);

   CUserLoadKey key(spanKey,intervalIdx);
   std::map<CUserLoadKey,std::vector<UserDistributedLoad>>::iterator found( m_DistributedLoads.find(key) );

   if (found == m_DistributedLoads.end())
   {
      return NULL;
   }

   return &(found->second);
}

const std::vector<IUserDefinedLoads::UserMomentLoad>* CBridgeAgentImp::GetMomentLoads(IntervalIndexType intervalIdx,const CSpanKey& spanKey)
{
   VALIDATE(LOADS);

   CUserLoadKey key(spanKey,intervalIdx);
   std::map<CUserLoadKey,std::vector<UserMomentLoad>>::iterator found( m_MomentLoads.find(key) );

   if (found == m_MomentLoads.end())
   {
      return NULL;
   }
      
   return &(found->second);
}

/////////////////////////////////////////////////////
// ITempSupport
void CBridgeAgentImp::GetControlPoints(SupportIndexType tsIdx,IPoint2d** ppLeft,IPoint2d** ppAlignment,IPoint2d** ppBridge,IPoint2d** ppRight)
{
   VALIDATE( BRIDGE );

   CComPtr<IBridgeGeometry> geometry;
   m_Bridge->get_BridgeGeometry(&geometry);

   CComPtr<IPierLine> pier;
   SupportIDType tsID = ::GetTempSupportLineID(tsIdx);
   geometry->FindPierLine( tsID, &pier);
   pier->get_AlignmentPoint(ppAlignment);

   // left and right edge points... intersect CL pier with deck edge
   CComPtr<ILine2d> line;
   pier->get_Centerline(&line);

   // get the left and right segment girder line that touch this temporary support
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CTemporarySupportData* pTS = pBridgeDesc->GetTemporarySupport(tsIdx);
   Float64 tsStation = pTS->GetStation();

   // need to get the girder group this temporary support belongs with
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pTS->GetSpan());
   GroupIndexType grpIdx = pGroup->GetIndex();

   // use left girder to get the number of segments as every girder in a group has the same number of segments
   GirderIndexType gdrIdx = 0;
   const CSplicedGirderData* pGirder = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirder(gdrIdx);
   SegmentIndexType nSegments = pGirder->GetSegmentCount();
   SegmentIndexType segIdx = INVALID_INDEX;
   for ( segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
      Float64 startStation,endStation;
      pSegment->GetStations(&startStation,&endStation);
      if ( ::InRange(startStation,tsStation,endStation) )
      {
         break;
      }
   }
   
   ATLASSERT( segIdx < nSegments ); // if this fails, the temp support wasn't found

   // since we don't have deck edges just yet, use the first and last girderline
   CComPtr<IGirderLine> left_girderline;
   LineIDType segID = ::GetGirderSegmentLineID(grpIdx,gdrIdx,segIdx);
   geometry->FindGirderLine( segID, &left_girderline);
   CComPtr<IPath> left_path;
   left_girderline->get_Path(&left_path);
   left_path->Intersect(line,*ppAlignment,ppLeft);

   GirderIndexType nGirderLines = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();

   CComPtr<IGirderLine> right_girderline;
   segID = ::GetGirderSegmentLineID(grpIdx,nGirderLines-1,segIdx);
   geometry->FindGirderLine( segID, &right_girderline);
   CComPtr<IPath> right_path;
   right_girderline->get_Path( &right_path);
   right_path->Intersect(line,*ppAlignment,ppRight);
}

void CBridgeAgentImp::GetDirection(SupportIndexType tsIdx,IDirection** ppDirection)
{
   VALIDATE(BRIDGE);

   CComPtr<IBridgeGeometry> geometry;
   m_Bridge->get_BridgeGeometry(&geometry);

   CComPtr<IPierLine> pier;
   SupportIDType tsID = ::GetTempSupportLineID(tsIdx);
   geometry->FindPierLine( tsID, &pier );

   pier->get_Direction(ppDirection);
}

void CBridgeAgentImp::GetSkew(SupportIndexType tsIdx,IAngle** ppAngle)
{
   VALIDATE(BRIDGE);

   CComPtr<IBridgeGeometry> geometry;
   m_Bridge->get_BridgeGeometry(&geometry);

   CComPtr<IPierLine> pier;
   SupportIDType tsID = ::GetTempSupportLineID(tsIdx);
   geometry->FindPierLine(tsID,&pier);

   pier->get_Skew(ppAngle);
}

/////////////////////////////////////////////////////
// IGirder

bool CBridgeAgentImp::IsPrismatic(IntervalIndexType intervalIdx,const CSegmentKey& segmentKey)
{
   VALIDATE( BRIDGE );

   // assume non-prismatic for all transformed sections
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
   if ( pSpecEntry->GetSectionPropertyMode() == pgsTypes::spmTransformed )
   {
      return false;
   }

   const GirderLibraryEntry* pGirderEntry = GetGirderLibraryEntry(segmentKey);

   CComPtr<IBeamFactory> beamFactory;
   pGirderEntry->GetBeamFactory(&beamFactory);

   bool bPrismaticGirder = beamFactory->IsPrismatic(m_pBroker,segmentKey);
   if ( !bPrismaticGirder )
   {
      return false; // if the bare girder is non-prismiatc... it will always be non-prismatic
   }

   if ( IsCompositeDeck() )
   {
      // there is a composite deck... compare the interval when the deck is made composite
      // against the interval we are evaluating prismatic-ness to

      // if the event we are evaluating is before the composite event then
      // we just have a bare girder... return its prismatic-ness
      if ( intervalIdx < m_IntervalManager.GetCompositeDeckInterval() )
      {
         return bPrismaticGirder;
      }
   }

   Float64 segmentLength = GetSegmentLength(segmentKey);
   CSpanKey startSpanKey, middleSpanKey, endSpanKey;
   Float64 XspanStart, XspanMiddle, XspanEnd;
   ConvertSegmentCoordinateToSpanPoint(segmentKey,            0.0,&startSpanKey, &XspanStart);
   ConvertSegmentCoordinateToSpanPoint(segmentKey,segmentLength/2,&middleSpanKey,&XspanMiddle);
   ConvertSegmentCoordinateToSpanPoint(segmentKey,segmentLength,  &endSpanKey,   &XspanEnd);

   // we have a prismatic section made composite and are evaluating in a composite event.
   // check to see if the composite section is non-prismatic

   // check the direction of each segment... if they aren't parallel, then the composite properties
   // are non-prismatic
   // for exterior segments, if the deck edge offset isn't the same at each end of the segment,
   // then the segment will be considered to be non-prismatic as well.
   //
   // this may not be the best way to figure it out, but it mostly works well and there isn't
   // any harm in being wrong except that section properties will be reported verbosely for
   // prismatic beams when a compact reporting would suffice.
   CComPtr<IDirection> dirLeftSegment, dirThisSegment, dirRightSegment;
   Float64 startOverhang = -1;
   Float64 endOverhang = -1;
   Float64 middleOverhang = -1;
   Float64 leftDir,thisDir,rightDir;
   bool bIsExterior = IsExteriorGirder(segmentKey);
   if ( bIsExterior )
   {
      GirderIndexType nGirders = GetGirderCount(segmentKey.groupIndex);
      if ( nGirders == 1 )
      {
         // if there is only one girder, then consider the section prismatic if the left and right 
         // overhangs match at both ends and mid-span of the span
         Float64 startOverhangLeft  = GetLeftSlabOverhang(startSpanKey.spanIndex,XspanStart);
         Float64 middleOverhangLeft = GetLeftSlabOverhang(middleSpanKey.spanIndex,XspanMiddle);
         Float64 endOverhangLeft    = GetLeftSlabOverhang(endSpanKey.spanIndex,XspanEnd);

         bool bLeftEqual = IsEqual(startOverhangLeft,middleOverhangLeft) && IsEqual(middleOverhangLeft,endOverhangLeft);

         Float64 startOverhangRight  = GetRightSlabOverhang(startSpanKey.spanIndex,XspanStart);
         Float64 middleOverhangRight = GetRightSlabOverhang(middleSpanKey.spanIndex,XspanMiddle);
         Float64 endOverhangRight    = GetRightSlabOverhang(endSpanKey.spanIndex,XspanEnd);

         bool bRightEqual = IsEqual(startOverhangRight,middleOverhangRight) && IsEqual(middleOverhangRight,endOverhangRight);

         return (bLeftEqual && bRightEqual);
      }
      else
      {
         if ( segmentKey.girderIndex == 0 )
         {
            // left exterior girder
            startOverhang  = GetLeftSlabOverhang(startSpanKey.spanIndex,XspanStart);
            middleOverhang = GetLeftSlabOverhang(middleSpanKey.spanIndex,XspanMiddle);
            endOverhang    = GetLeftSlabOverhang(endSpanKey.spanIndex,XspanEnd);

            GetSegmentBearing(segmentKey,  &dirThisSegment);
            CSegmentKey rightSegmentKey(segmentKey);
            rightSegmentKey.girderIndex++;
            GetSegmentBearing(rightSegmentKey,&dirRightSegment);
            dirThisSegment->get_Value(&thisDir);
            dirRightSegment->get_Value(&rightDir);
         }
         else
         {
            // right exterior girder
            CSegmentKey leftSegmentKey(segmentKey);
            leftSegmentKey.girderIndex--;
            GetSegmentBearing(leftSegmentKey,&dirLeftSegment);
            GetSegmentBearing(segmentKey,  &dirThisSegment);
            dirLeftSegment->get_Value(&leftDir);
            dirThisSegment->get_Value(&thisDir);

            startOverhang  = GetRightSlabOverhang(startSpanKey.spanIndex,XspanStart);
            middleOverhang = GetRightSlabOverhang(middleSpanKey.spanIndex,XspanMiddle);
            endOverhang    = GetRightSlabOverhang(endSpanKey.spanIndex,XspanEnd);
         }
      }
   }
   else
   {
      // if interior girders are not parallel, then they aren't going to be
      // prismatic if they have a composite deck
      CSegmentKey leftSegmentKey,rightSegmentKey;
      GetAdjacentGirderKeys(segmentKey,&leftSegmentKey,&rightSegmentKey);
      GetSegmentBearing(leftSegmentKey,&dirLeftSegment);
      GetSegmentBearing(segmentKey,&dirThisSegment);
      GetSegmentBearing(rightSegmentKey,&dirRightSegment);

      dirLeftSegment->get_Value(&leftDir);
      dirThisSegment->get_Value(&thisDir);
      dirRightSegment->get_Value(&rightDir);
   }

   if ( dirLeftSegment == NULL )
   {
      return (IsEqual(thisDir,rightDir) && IsEqual(startOverhang,middleOverhang) && IsEqual(middleOverhang,endOverhang) ? true : false);
   }
   else if ( dirRightSegment == NULL )
   {
      return (IsEqual(leftDir,thisDir) && IsEqual(startOverhang,middleOverhang) && IsEqual(middleOverhang,endOverhang) ? true : false);
   }
   else
   {
      return (IsEqual(leftDir,thisDir) && IsEqual(thisDir,rightDir) ? true : false);
   }
}

bool CBridgeAgentImp::IsSymmetric(IntervalIndexType intervalIdx,const CGirderKey& girderKey)
{
   IntervalIndexType liveLoadIntervalIdx = m_IntervalManager.GetLiveLoadInterval();

   // need to look at girder spacing to see if it is constant
   if ( liveLoadIntervalIdx <= intervalIdx )
   {
      return false;
   }

   // other places in the program can gain huge speed efficiencies if the 
   // girder is symmetric (only half the work needs to be done)
   SegmentIndexType nSegments = GetSegmentCount(girderKey);
   Float64 start_length = GetSegmentStartEndDistance(CSegmentKey(girderKey,0));
   Float64 end_length   = GetSegmentEndEndDistance(CSegmentKey(girderKey,nSegments-1));
   
   if ( !IsEqual(start_length,end_length) )
   {
      return false; // different end lengths
   }

   SegmentIndexType leftSegIdx = 0;
   SegmentIndexType rightSegIdx = nSegments-1;
   for ( ; leftSegIdx < rightSegIdx && rightSegIdx != INVALID_INDEX; leftSegIdx++, rightSegIdx-- )
   {
      CSegmentKey leftSegmentKey(girderKey,leftSegIdx);
      CSegmentKey rightSegmentKey(girderKey,rightSegIdx);
      Float64 left_segment_length = GetSegmentLength(leftSegmentKey);
      Float64 right_segment_length = GetSegmentLength(rightSegmentKey);

      if ( !IsEqual(left_segment_length,right_segment_length) )
      {
         return false;
      }

      Float64 left_hp1, left_hp2;
      GetHarpingPointLocations(leftSegmentKey,&left_hp1,&left_hp2);

      Float64 right_hp1, right_hp2;
      GetHarpingPointLocations(rightSegmentKey,&right_hp1,&right_hp2); // measured from left end of segment
      right_hp1 = right_segment_length - right_hp1; // measured from right end of segment
      right_hp2 = right_segment_length - right_hp2; // measured from right end of segment
      std::swap(right_hp1,right_hp2);

      if ( !IsEqual(left_hp1,right_hp1) || !IsEqual(left_hp2,right_hp2) )
      {
         return false;
      }

      if ( !IsDebondingSymmetric(leftSegmentKey) || !IsDebondingSymmetric(rightSegmentKey) )
      {
         return false;
      }
   }

   return true;
}

MatingSurfaceIndexType CBridgeAgentImp::GetNumberOfMatingSurfaces(const CGirderKey& girderKey)
{
   VALIDATE( BRIDGE );

   CSegmentKey segmentKey(girderKey,0);

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(pgsPointOfInterest(segmentKey,0.00),pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   FlangeIndexType count;
   girder_section->get_MatingSurfaceCount(&count);

   return count;
}

Float64 CBridgeAgentImp::GetMatingSurfaceLocation(const pgsPointOfInterest& poi,MatingSurfaceIndexType webIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 location;
   girder_section->get_MatingSurfaceLocation(webIdx,&location);
   
   return location;
}

Float64 CBridgeAgentImp::GetMatingSurfaceWidth(const pgsPointOfInterest& poi,MatingSurfaceIndexType webIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 width;
   girder_section->get_MatingSurfaceWidth(webIdx,&width);

   return width;
}

FlangeIndexType CBridgeAgentImp::GetNumberOfTopFlanges(const CGirderKey& girderKey)
{
   VALIDATE( BRIDGE );

   pgsPointOfInterest poi(CSegmentKey(girderKey,0),0.00);

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   FlangeIndexType nFlanges;
   girder_section->get_TopFlangeCount(&nFlanges);
   return nFlanges;
}

Float64 CBridgeAgentImp::GetTopFlangeLocation(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 location;
   girder_section->get_TopFlangeLocation(flangeIdx,&location);
   return location;
}

Float64 CBridgeAgentImp::GetTopFlangeWidth(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 width;
   girder_section->get_TopFlangeWidth(flangeIdx,&width);
   return width;
}

Float64 CBridgeAgentImp::GetTopFlangeThickness(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 thickness;
   girder_section->get_TopFlangeThickness(flangeIdx,&thickness);
   return thickness;
}

Float64 CBridgeAgentImp::GetTopFlangeSpacing(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 spacing;
   girder_section->get_TopFlangeSpacing(flangeIdx,&spacing);
   return spacing;
}

Float64 CBridgeAgentImp::GetTopFlangeWidth(const pgsPointOfInterest& poi)
{
   MatingSurfaceIndexType nMS = GetNumberOfMatingSurfaces(poi.GetSegmentKey());
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
   VALIDATE( GIRDER );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 width;
   girder_section->get_TopWidth(&width);

   return width;
}

FlangeIndexType CBridgeAgentImp::GetNumberOfBottomFlanges(const CSegmentKey& segmentKey)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(pgsPointOfInterest(segmentKey,0.00),pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   FlangeIndexType nFlanges;
   girder_section->get_BottomFlangeCount(&nFlanges);
   return nFlanges;
}

Float64 CBridgeAgentImp::GetBottomFlangeLocation(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 location;
   girder_section->get_BottomFlangeLocation(flangeIdx,&location);
   return location;
}

Float64 CBridgeAgentImp::GetBottomFlangeWidth(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 width;
   girder_section->get_BottomFlangeWidth(flangeIdx,&width);
   return width;
}

Float64 CBridgeAgentImp::GetBottomFlangeThickness(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 thickness;
   girder_section->get_BottomFlangeThickness(flangeIdx,&thickness);
   return thickness;
}

Float64 CBridgeAgentImp::GetBottomFlangeSpacing(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 spacing;
   girder_section->get_BottomFlangeSpacing(flangeIdx,&spacing);
   return spacing;
}

Float64 CBridgeAgentImp::GetBottomFlangeWidth(const pgsPointOfInterest& poi)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
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
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 width;
   girder_section->get_BottomWidth(&width);

   return width;
}

Float64 CBridgeAgentImp::GetMinWebWidth(const pgsPointOfInterest& poi)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 width;
   girder_section->get_MinWebThickness(&width);

   return width;
}

Float64 CBridgeAgentImp::GetWebThicknessAtDuct(const pgsPointOfInterest& poi,DuctIndexType ductIdx)
{
   // Use a better implementation
   return GetMinWebWidth(poi); // this is good enough for now, but there is a better way to do this

   // If we have a 3D solid model in WBFLGenericBridge we could cut a section at this poi and then
   // shoot a line across the section at the duct level (line-shape intersection). The points on the
   // intersecting line define the width of the member at the level of the duct.
   // This would be a more general implementation and would cut down on the requirements of IBeamFactory
}

Float64 CBridgeAgentImp::GetMinTopFlangeThickness(const pgsPointOfInterest& poi)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 ttf;
   girder_section->get_MinTopFlangeThickness(&ttf);
   return ttf;
}

Float64 CBridgeAgentImp::GetMinBottomFlangeThickness(const pgsPointOfInterest& poi)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 tbf;
   girder_section->get_MinBottomFlangeThickness(&tbf);
   return tbf;
}

Float64 CBridgeAgentImp::GetHeight(const pgsPointOfInterest& poi)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 height;
   girder_section->get_GirderHeight(&height);

   return height;
}

Float64 CBridgeAgentImp::GetShearWidth(const pgsPointOfInterest& poi)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 shear_width;
   girder_section->get_ShearWidth(&shear_width);

   CGirderKey girderKey(poi.GetSegmentKey());
   DuctIndexType nDucts = GetDuctCount(girderKey);

   if ( nDucts == 0 )
   {
      // No ducts... don't have to make an adjustment.... leave now and
      // skip all work below
      return shear_width;
   }

   // Deduct for duct diameter. See LRFD 5.8.2.9 and C5.8.2.9
   // the method for determining if the shear width should be adjusted for the presense
   // of a duct, and the amount of duct to reduce the shear width, changed in LRFD 2nd Edition, 2003 interims
   //
   IntervalIndexType intervalIdx = GetIntervalCount() - 1;

   GET_IFACE(IShearCapacity,pShearCapacity);
   pgsTypes::FaceType tensionSide = pShearCapacity->GetFlexuralTensionSide(pgsTypes::StrengthI,intervalIdx,poi);

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter2002 = ( lrfdVersionMgr::SecondEditionWith2003Interims <= pSpecEntry->GetSpecificationType() ? true : false );

   // Limits of deduction for ducts is between the tensile and compression resultant
   // (limit is within dv for LRFD before 2003... see below)


   // get moment capacity details
   GET_IFACE(IMomentCapacity,pMomentCapacity);
   MOMENTCAPACITYDETAILS capdet;
   pMomentCapacity->GetMomentCapacityDetails(intervalIdx, poi, tensionSide == pgsTypes::BottomFace ? true : false, &capdet);

   Float64 struct_slab_h = GetStructuralSlabDepth(poi);
   Float64 Ytop = struct_slab_h - capdet.dc; // compression resultant (from top of bare girder in Girder Section Coordinates)
   Float64 Ybot = struct_slab_h - capdet.de; // tension resultant (from top of bare girder in Girder Section Coordinates)

   if ( !bAfter2002 )
   {
      // determine top and bottom of "dv"
      // we have to compute dv here otherwise we get recursion with the shear capacity engineer
      Float64 de = capdet.de_shear; // see PCI BDM 8.4.1.2
      Float64 MomentArm = capdet.MomentArm;

      Float64 h = GetHeight(poi) + struct_slab_h;

      // lrfd 5.8.2.7
      Float64 dv1 = MomentArm;
      Float64 dv2 = 0.9*de;
      Float64 dv3 = 0.72*h;

      // assume dv1 controls. Ytop and Ybot are based on the moment arm (moment arm = de - dc)
      IndexType i = MaxIndex(dv1,dv2,dv3);
      if ( i == 1 || i == 2 )
      {
         // dv = moment arm does not control...

         Float64 Ymid = 0.5*(Ytop+Ybot); // assume middle of depth "dv" is at a fixed location
         if ( i == 1 )
         {
            // 0.9de controls
            Ytop = Ymid + dv2/2;
            Ybot = Ymid - dv2/2;
         }
         else
         {
            // 0.72h controls
            Ytop = Ymid + dv3/2;
            Ybot = Ymid - dv3/2;
         }
      }
   }

   Float64 duct_deduction = 0;
   if ( IsOnGirder(poi) )
   {
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         // assumed ducts are grouted and cured in the interval following their installation and stressing
         IntervalIndexType groutDuctIntervalIdx = GetStressTendonInterval(girderKey,ductIdx)+1;

         CComPtr<IPoint2d> point;
         GetDuctPoint(poi,ductIdx,&point); // in Girder Section Coordinates
         Float64 y;
         point->get_Y(&y);

         if ( ::InRange(Ybot,y,Ytop) )
         {
            Float64 deduct_factor;
            if ( bAfter2002 )
            {
               if ( intervalIdx < groutDuctIntervalIdx )
               {
                  deduct_factor =  0.50;
               }
               else
               {
                  deduct_factor =  0.25;
               }
            }
            else
            {
               if ( intervalIdx < groutDuctIntervalIdx )
               {
                  deduct_factor =  1.00;
               }
               else
               {
                  deduct_factor =  0.50;
               }
            }

            Float64 dia = GetOutsideDiameter(girderKey,ductIdx);
            duct_deduction = Max(duct_deduction,deduct_factor*dia);
         }
      }
   }

   shear_width -= duct_deduction;

   return shear_width;
}

Float64 CBridgeAgentImp::GetShearInterfaceWidth(const pgsPointOfInterest& poi)
{
   VALIDATE( BRIDGE );

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   Float64 wMating = 0; // sum of mating surface widths... less deck panel support width

   if ( pDeck->DeckType == pgsTypes::sdtCompositeCIP || pDeck->DeckType == pgsTypes::sdtCompositeOverlay )
   {
      MatingSurfaceIndexType nMatingSurfaces = GetNumberOfMatingSurfaces(segmentKey);
      for ( MatingSurfaceIndexType i = 0; i < nMatingSurfaces; i++ )
      {
         wMating += GetMatingSurfaceWidth(poi,i);
      }
   }
   else if ( pDeck->DeckType == pgsTypes::sdtCompositeSIP )
   {
      // SIP Deck Panel System... Area beneath the deck panesl aren't part of the
      // shear transfer area
      MatingSurfaceIndexType nMatingSurfaces = GetNumberOfMatingSurfaces(segmentKey);
      Float64 panel_support = pDeck->PanelSupport;
      for ( MatingSurfaceIndexType i = 0; i < nMatingSurfaces; i++ )
      {
         CComPtr<ISuperstructureMember> ssMbr;
         GetSuperstructureMember(segmentKey,&ssMbr);
         LocationType locationType;
         ssMbr->get_LocationType(&locationType);

         if ( (locationType == ltLeftExteriorGirder && i == 0) ||
              (locationType == ltRightExteriorGirder && i == nMatingSurfaces-1)
            )
         {
            wMating += GetMatingSurfaceWidth(poi,i) - panel_support;
         }
         else
         {
            wMating += GetMatingSurfaceWidth(poi,i) - 2*panel_support;
         }
      }

      if ( wMating < 0 )
      {
         wMating = 0;

         CString strMsg;
         strMsg.Format(_T("%s, Deck panel support width exceeds half the width of the supporting flange. An interface shear width of 0.0 will be used"),GIRDER_LABEL(segmentKey));

         pgsBridgeDescriptionStatusItem* pStatusItem = 
            new pgsBridgeDescriptionStatusItem(m_StatusGroupID,m_scidBridgeDescriptionWarning,pgsBridgeDescriptionStatusItem::Deck,strMsg);

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

WebIndexType CBridgeAgentImp::GetWebCount(const CGirderKey& girderKey)
{
   VALIDATE( BRIDGE );

   CSegmentKey segmentKey(girderKey,0);

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(pgsPointOfInterest(segmentKey,0.00),pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   WebIndexType count;
   girder_section->get_WebCount(&count);

   return count;
}

Float64 CBridgeAgentImp::GetWebLocation(const pgsPointOfInterest& poi,WebIndexType webIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 location;
   girder_section->get_WebLocation(webIdx,&location);
   
   return location;
}

Float64 CBridgeAgentImp::GetWebSpacing(const pgsPointOfInterest& poi,SpacingIndexType spaceIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 spacing;
   girder_section->get_WebSpacing(spaceIdx,&spacing);

   return spacing;
}

Float64 CBridgeAgentImp::GetWebThickness(const pgsPointOfInterest& poi,WebIndexType webIdx)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 thickness;
   girder_section->get_WebThickness(webIdx,&thickness);
   
   return thickness;
}

Float64 CBridgeAgentImp::GetCL2ExteriorWebDistance(const pgsPointOfInterest& poi)
{
   VALIDATE( BRIDGE );

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   GET_IFACE(IBarriers,         pBarriers);
   pgsTypes::TrafficBarrierOrientation side = pBarriers->GetNearestBarrier(segmentKey);
   DirectionType dtside = (side==pgsTypes::tboLeft) ? qcbLeft : qcbRight;

   Float64 dist;
   girder_section->get_CL2ExteriorWebDistance(dtside, &dist);

   return dist;
}

Float64 CBridgeAgentImp::GetWebWidth(const pgsPointOfInterest& poi)
{
   VALIDATE( BRIDGE );

   CComPtr<IGirderSection> girder_section;
   HRESULT hr = GetGirderSection(poi,pgsTypes::scBridge,&girder_section);
   ATLASSERT(SUCCEEDED(hr));

   Float64 shear_width;
   girder_section->get_ShearWidth(&shear_width);

   return shear_width;
}

Float64 CBridgeAgentImp::GetOrientation(const CSegmentKey& segmentKey)
{
   ValidateSegmentOrientation(segmentKey);

   CComPtr<ISuperstructureMemberSegment> segment;
   GetSegment(segmentKey,&segment);

   Float64 orientation;
   segment->get_Orientation(&orientation);

   return orientation;
}

Float64 CBridgeAgentImp::GetTopGirderReferenceChordElevation(const pgsPointOfInterest& poi)
{
   // elevation of the top of girder reference chord
   // the reference chord is a straight line parallel to the top of the
   // undeformed girder, based on a constant slab offset, that intersects
   // the deck at the start and end CL bearings
   VALIDATE( BRIDGE );

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CComPtr<IPoint2d> pntPier1, pntEnd1, pntBrg1, pntBrg2, pntEnd2, pntPier2;
   GetSegmentEndPoints(segmentKey,&pntPier1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntPier2);

   Float64 startStation, startOffset;
   GetStationAndOffset(pntBrg1,&startStation,&startOffset);

   Float64 endStation, endOffset;
   GetStationAndOffset(pntBrg2,&endStation,&endOffset);

   Float64 startElevation = GetElevation(startStation,startOffset);
   Float64 endElevation   = GetElevation(endStation,  endOffset);

   Float64 length;
   pntBrg1->DistanceEx(pntBrg2,&length);

   Float64 Xpoi = poi.GetDistFromStart();

   Float64 end_size = GetSegmentStartEndDistance(segmentKey);

   Float64 dist_from_left_bearing = Xpoi - end_size;

   Float64 yc = ::LinInterp(dist_from_left_bearing,startElevation,endElevation,length);

   return yc;
}

Float64 CBridgeAgentImp::GetTopGirderElevation(const pgsPointOfInterest& poi,MatingSurfaceIndexType matingSurfaceIndex)
{
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

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   Float64 end_size = GetSegmentStartEndDistance(segmentKey);

   // girder offset at start bearing
   Float64 station, zs;
   GetStationAndOffset(pgsPointOfInterest(segmentKey,end_size),&station,&zs);

   Float64 webOffset = (matingSurfaceIdx == INVALID_INDEX ? 0 : GetMatingSurfaceLocation(poi,matingSurfaceIdx));

   // roadway surface elevation at start bearing, directly above web centerline
   Float64 Ysb = GetElevation(station,zs + webOffset);

   Float64 girder_slope = GetSegmentSlope(segmentKey); // accounts for elevation changes at temporary supports

   Float64 dist_from_start_bearing = poi.GetDistFromStart() - end_size;

   PierIndexType pierIdx = GetGirderGroupStartPier(segmentKey.groupIndex);
   Float64 slab_offset_at_start = GetSlabOffset(segmentKey.groupIndex,pierIdx,segmentKey.girderIndex);

   // get the camber
   GET_IFACE(ICamber,pCamber);
   Float64 excess_camber = (bUseConfig ? pCamber->GetExcessCamber(poi,config,CREEP_MAXTIME)
                                       : pCamber->GetExcessCamber(poi,CREEP_MAXTIME) );

   Float64 top_gdr_elev = Ysb - slab_offset_at_start + girder_slope*dist_from_start_bearing + excess_camber;
   return top_gdr_elev;
}

void CBridgeAgentImp::GetProfileShape(const CSegmentKey& segmentKey,IShape** ppShape)
{
   const GirderLibraryEntry* pGirderEntry = GetGirderLibraryEntry(segmentKey);

   CComPtr<IBeamFactory> beamFactory;
   pGirderEntry->GetBeamFactory(&beamFactory);

   beamFactory->CreateGirderProfile(m_pBroker,m_StatusGroupID,segmentKey,pGirderEntry->GetDimensions(),ppShape);
}

bool CBridgeAgentImp::HasShearKey(const CGirderKey& girderKey,pgsTypes::SupportedBeamSpacing spacingType)
{
   const GirderLibraryEntry* pGirderEntry = GetGirderLibraryEntry(girderKey);
   CComPtr<IBeamFactory> beamFactory;
   pGirderEntry->GetBeamFactory(&beamFactory);

   return beamFactory->IsShearKey(pGirderEntry->GetDimensions(), spacingType);
}

void CBridgeAgentImp::GetShearKeyAreas(const CGirderKey& girderKey,pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint)
{
   const GirderLibraryEntry* pGirderEntry = GetGirderLibraryEntry(girderKey);
   CComPtr<IBeamFactory> beamFactory;
   pGirderEntry->GetBeamFactory(&beamFactory);

   beamFactory->GetShearKeyAreas(pGirderEntry->GetDimensions(), spacingType, uniformArea, areaPerJoint);
}

void CBridgeAgentImp::GetSegmentEndPoints(const CSegmentKey& segmentKey,IPoint2d** ppSupport1,IPoint2d** ppEnd1,IPoint2d** ppBrg1,IPoint2d** ppBrg2,IPoint2d** ppEnd2,IPoint2d** ppSupport2)
{
   CComPtr<IGirderLine> girderLine;
   GetGirderLine(segmentKey,&girderLine);
   girderLine->GetEndPoints(ppSupport1,ppEnd1,ppBrg1,ppBrg2,ppEnd2,ppSupport2);
}

Float64 CBridgeAgentImp::GetSegmentLength(const CSegmentKey& segmentKey)
{
   // returns the end-to-end length of the segment.
   CComPtr<IGirderLine> girderLine;
   GetGirderLine(segmentKey,&girderLine);

   Float64 length;
   girderLine->get_GirderLength(&length);

   return length;
}

Float64 CBridgeAgentImp::GetSegmentSpanLength(const CSegmentKey& segmentKey)
{
   // returns the CL-Brg to CL-Brg span length
   CComPtr<IGirderLine> girderLine;
   GetGirderLine(segmentKey,&girderLine);

   Float64 length;
   girderLine->get_SpanLength(&length);

   return length;
}

Float64 CBridgeAgentImp::GetSegmentLayoutLength(const CSegmentKey& segmentKey)
{
   // Pier to pier length
   CComPtr<IGirderLine> girderLine;
   GetGirderLine(segmentKey,&girderLine);

   Float64 length;
   girderLine->get_LayoutLength(&length);

   return length;
}

Float64 CBridgeAgentImp::GetSegmentPlanLength(const CSegmentKey& segmentKey)
{
   // returns the length of the segment adjusted for slope
   Float64 length = GetSegmentLength(segmentKey);
   Float64 slope  = GetSegmentSlope(segmentKey);

   Float64 plan_length = length*sqrt(1.0 + slope*slope);

   return plan_length;
}

Float64 CBridgeAgentImp::GetSegmentSlope(const CSegmentKey& segmentKey)
{
   VALIDATE(BRIDGE);

   CComPtr<IPoint2d> pntPier1, pntEnd1, pntBrg1, pntBrg2, pntEnd2, pntPier2;
   GetSegmentEndPoints(segmentKey,&pntPier1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntPier2);

   Float64 station,offset;
   GetStationAndOffset(pntBrg1,&station,&offset);
   Float64 elev1 = GetElevation(station,offset);

   GetStationAndOffset(pntBrg2,&station,&offset);
   Float64 elev2 = GetElevation(station,offset);

   Float64 dist;
   pntBrg1->DistanceEx(pntBrg2,&dist);

   Float64 slope = (elev2 - elev1)/dist;
   return slope;
}

void CBridgeAgentImp::GetSegmentProfile(const CSegmentKey& segmentKey,bool bIncludeClosure,IShape** ppShape)
{
   CComPtr<ISuperstructureMemberSegment> segment;
   GetSegment(segmentKey,&segment);

   segment->get_Profile(bIncludeClosure ? VARIANT_TRUE : VARIANT_FALSE,ppShape);
}

void CBridgeAgentImp::GetSegmentProfile(const CSegmentKey& segmentKey,const CSplicedGirderData* pGirder,bool bIncludeClosure,IShape** ppShape)
{
   // X-values for start and end of the segment under consideration
   // measured from the intersection of the CL Pier and CL Girder/Segment at
   // the start of the girder
   Float64 xStart,xEnd;
   GetSegmentRange(segmentKey,&xStart,&xEnd); // this is the start/end of the Girder Path Coordinates for this segment

   if ( segmentKey.segmentIndex == 0 )
   {
      // if this is the first segment, adjust the girder path coordinate so that it starts at the face of the segment
      Float64 brgOffset = GetSegmentStartBearingOffset(segmentKey);
      Float64 endDist   = GetSegmentStartEndDistance(segmentKey);
      Float64 offset_dist = brgOffset - endDist;
      offset_dist = IsZero(offset_dist) ? 0 : offset_dist;
      xStart += offset_dist;
   }

   SegmentIndexType nSegments = GetSegmentCount(segmentKey);
   if ( segmentKey.segmentIndex == nSegments-1 )
   {
      // if this is the last segment, adjust the girder path coordinate so that it end at the face of the segment
      Float64 brgOffset = GetSegmentEndBearingOffset(segmentKey);
      Float64 endDist   = GetSegmentEndEndDistance(segmentKey);
      Float64 offset_dist = brgOffset - endDist;
      offset_dist = IsZero(offset_dist) ? 0 : offset_dist;
      xEnd -= offset_dist;
   }

   boost::shared_ptr<mathFunction2d> f = CreateGirderProfile(pGirder);
   CComPtr<IPolyShape> polyShape;
   polyShape.CoCreateInstance(CLSID_PolyShape);

   std::vector<Float64> xValues;

   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);

   Float64 segmentLength = GetSegmentLayoutLength(segmentKey);

   // if not including closure joint, adjust the start/end locations to be
   // at the actual start/end of the segment
   if ( !bIncludeClosure )
   {
      Float64 startBrgOffset, endBrgOffset;
      GetSegmentBearingOffset(segmentKey,&startBrgOffset,&endBrgOffset);

      Float64 startEndDist, endEndDist;
      GetSegmentEndDistance(segmentKey,&startEndDist,&endEndDist);

      if ( segmentKey.segmentIndex != 0 )
      {
         xStart += startBrgOffset - startEndDist;
      }

      if ( segmentKey.segmentIndex != nSegments-1 )
      {
         xEnd -= endBrgOffset   - endEndDist;
      }
   }

   Float64 variationLength[4];
   for ( int i = 0; i < 4; i++ )
   {
      variationLength[i] = pSegment->GetVariationLength((pgsTypes::SegmentZoneType)i);
      if ( variationLength[i] < 0 )
      {
         ATLASSERT(-1.0 <= variationLength[i] && variationLength[i] <= 0.0);
         variationLength[i] *= -segmentLength;
      }
   }

   // capture key values in segment
   pgsTypes::SegmentVariationType variationType = pSegment->GetVariationType();
   if ( variationType == pgsTypes::svtLinear )
   {
      xValues.push_back(xStart + variationLength[pgsTypes::sztLeftPrismatic] );
      xValues.push_back(xEnd   - variationLength[pgsTypes::sztRightPrismatic] );
   }
   else if ( variationType == pgsTypes::svtParabolic )
   {
      Float64 xStartParabola = xStart + variationLength[pgsTypes::sztLeftPrismatic];
      Float64 xEndParabola   = xEnd   - variationLength[pgsTypes::sztRightPrismatic];
      xValues.push_back(xStartParabola);
      for ( int i = 0; i < 5; i++ )
      {
         Float64 x = i*(xEndParabola-xStartParabola)/5 + xStartParabola;
         xValues.push_back(x);
      }
      xValues.push_back(xEndParabola);
   }
   else if ( variationType == pgsTypes::svtDoubleLinear )
   {
      xValues.push_back(xStart + variationLength[pgsTypes::sztLeftPrismatic] );
      xValues.push_back(xStart + variationLength[pgsTypes::sztLeftPrismatic]  + variationLength[pgsTypes::sztLeftTapered] );
      xValues.push_back(xEnd   - variationLength[pgsTypes::sztRightPrismatic] - variationLength[pgsTypes::sztRightTapered] );
      xValues.push_back(xEnd   - variationLength[pgsTypes::sztRightPrismatic] );
   }
   else if ( variationType == pgsTypes::svtDoubleParabolic )
   {
      // left parabola
      Float64 xStartParabola = xStart + variationLength[pgsTypes::sztLeftPrismatic];
      Float64 xEndParabola   = xStart + variationLength[pgsTypes::sztLeftPrismatic]  + variationLength[pgsTypes::sztLeftTapered];

      xValues.push_back(xStartParabola);
      for ( int i = 0; i < 5; i++ )
      {
         Float64 x = i*(xEndParabola-xStartParabola)/5 + xStartParabola;
         xValues.push_back(x);
      }
      xValues.push_back(xEndParabola);

      // right parabola
      xStartParabola = xEnd   - variationLength[pgsTypes::sztRightPrismatic] - variationLength[pgsTypes::sztRightTapered];
      xEndParabola   = xEnd   - variationLength[pgsTypes::sztRightPrismatic];
      xValues.push_back(xStartParabola);
      for ( int i = 0; i < 5; i++ )
      {
         Float64 x = i*(xEndParabola-xStartParabola)/5 + xStartParabola;
         xValues.push_back(x);
      }
      xValues.push_back(xEndParabola);
   }

   // fill up with other points
   for ( int i = 0; i < 11; i++ )
   {
      Float64 x = i*(xEnd - xStart)/10 + xStart;
      xValues.push_back(x);
   }

   std::sort(xValues.begin(),xValues.end());

   std::vector<Float64>::iterator iter(xValues.begin());
   std::vector<Float64>::iterator end(xValues.end());
   for ( ; iter != end; iter++ )
   {
      Float64 x = *iter;
      Float64 y = f->Evaluate(x);
      polyShape->AddPoint(x,-y);
   }

   // points across the top of the segment
   polyShape->AddPoint(xEnd,0);
   polyShape->AddPoint(xStart,0);

   polyShape->get_Shape(ppShape);
}

Float64 CBridgeAgentImp::GetSegmentHeight(const CSegmentKey& segmentKey,const CSplicedGirderData* pSplicedGirder,Float64 Xsp)
{
   Float64 Xgp = ConvertSegmentPathCoordinateToGirderPathCoordinate(segmentKey,Xsp);
   boost::shared_ptr<mathFunction2d> f = CreateGirderProfile(pSplicedGirder);
   Float64 Y = f->Evaluate(Xgp);
   return Y;
}

void CBridgeAgentImp::GetSegmentEndDistance(const CSegmentKey& segmentKey,Float64* pStartEndDistance,Float64* pEndEndDistance)
{
   CComPtr<IGirderLine> girderLine;
   GetGirderLine(segmentKey,&girderLine);

   girderLine->get_EndDistance(etStart,pStartEndDistance);
   girderLine->get_EndDistance(etEnd,  pEndEndDistance);
}

void CBridgeAgentImp::GetSegmentEndDistance(const CSegmentKey& segmentKey,const CSplicedGirderData* pGirder,Float64* pStartEndDistance,Float64* pEndEndDistance)
{
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);
   const CClosureJointData* pStartClosure  = pSegment->GetStartClosure();
   const CClosureJointData* pEndClosure = pSegment->GetEndClosure();

   // Assume pGirder is not associated with our bridge, but rather a detached copy that is
   // being used in an editing situation.

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // Left End
   Float64 leftEndDistance;
   ConnectionLibraryEntry::EndDistanceMeasurementType leftMeasureType;
   CComPtr<IAngle> leftSkewAngle;
   if ( pStartClosure )
   {
      const CTemporarySupportData* pTS = pStartClosure->GetTemporarySupport();
      const CPierData2* pPier = pStartClosure->GetPier();
      ATLASSERT( pTS != NULL || pPier != NULL );

      if ( pTS )
      {
         pTS->GetGirderEndDistance(&leftEndDistance,&leftMeasureType);
         GetSkew(pTS->GetIndex(),&leftSkewAngle);
      }
      else
      {
         pPier->GetGirderEndDistance(pgsTypes::Ahead,&leftEndDistance,&leftMeasureType);
         GetPierSkew(pPier->GetIndex(),&leftSkewAngle);
      }
   }
   else
   {
      const CPierData2* pPier = pBridgeDesc->GetPier(pGirder->GetPierIndex(pgsTypes::metStart));
      pPier->GetGirderEndDistance(pgsTypes::Ahead,&leftEndDistance,&leftMeasureType);
      GetPierSkew(pPier->GetIndex(),&leftSkewAngle);
   }
   
   // Adjust for measurement datum
   if ( leftMeasureType == ConnectionLibraryEntry::FromPierNormalToPier )
   {
      Float64 skew;
      leftSkewAngle->get_Value(&skew);

      Float64 leftBrgOffset, rightBrgOffset;
      GetSegmentBearingOffset(segmentKey,&leftBrgOffset,&rightBrgOffset);

      leftEndDistance = leftBrgOffset - leftEndDistance/cos(fabs(skew));
   }
   else if ( leftMeasureType == ConnectionLibraryEntry::FromPierAlongGirder )
   {
      Float64 leftBrgOffset, rightBrgOffset;
      GetSegmentBearingOffset(segmentKey,&leftBrgOffset,&rightBrgOffset);

      leftEndDistance = leftBrgOffset - leftEndDistance;
   }
   else if ( leftMeasureType == ConnectionLibraryEntry::FromBearingNormalToPier )
   {
      Float64 skew;
      leftSkewAngle->get_Value(&skew);
      leftEndDistance /= cos(fabs(skew));
   }
#if defined _DEBUG
   else
   {
      // end distance is already in the format we want it
      ATLASSERT( leftMeasureType == ConnectionLibraryEntry::FromBearingAlongGirder );
   }
#endif

   Float64 rightEndDistance;
   ConnectionLibraryEntry::EndDistanceMeasurementType rightMeasureType;
   CComPtr<IAngle> rightSkewAngle;
   if ( pEndClosure )
   {
      const CTemporarySupportData* pTS = pEndClosure->GetTemporarySupport();
      const CPierData2* pPier = pEndClosure->GetPier();
      ATLASSERT( pTS != NULL || pPier != NULL );

      if ( pTS )
      {
         pTS->GetGirderEndDistance(&rightEndDistance,&rightMeasureType);
         GetSkew(pTS->GetIndex(),&rightSkewAngle);
      }
      else
      {
         pPier->GetGirderEndDistance(pgsTypes::Back,&rightEndDistance,&rightMeasureType);
         GetPierSkew(pPier->GetIndex(),&rightSkewAngle);
      }
   }
   else
   {
      const CPierData2* pPier = pBridgeDesc->GetPier(pGirder->GetPierIndex(pgsTypes::metEnd));
      pPier->GetGirderEndDistance(pgsTypes::Back,&rightEndDistance,&rightMeasureType);
      GetPierSkew(pPier->GetIndex(),&rightSkewAngle);
   }
   
   // Adjust for measurement datum
   if ( rightMeasureType == ConnectionLibraryEntry::FromPierNormalToPier )
   {
      Float64 skew;
      rightSkewAngle->get_Value(&skew);

      Float64 leftBrgOffset, rightBrgOffset;
      GetSegmentBearingOffset(segmentKey,&leftBrgOffset,&rightBrgOffset);

      rightEndDistance = rightBrgOffset - rightEndDistance/cos(fabs(skew));
   }
   else if ( rightMeasureType == ConnectionLibraryEntry::FromPierAlongGirder )
   {
      Float64 leftBrgOffset, rightBrgOffset;
      GetSegmentBearingOffset(segmentKey,&leftBrgOffset,&rightBrgOffset);

      rightEndDistance = rightBrgOffset - rightEndDistance;
   }
   else if ( rightMeasureType == ConnectionLibraryEntry::FromBearingNormalToPier )
   {
      Float64 skew;
      rightSkewAngle->get_Value(&skew);
      rightEndDistance /= cos(fabs(skew));
   }
#if defined _DEBUG
   else
   {
      // end distance is already in the format we want it
      ATLASSERT( rightMeasureType == ConnectionLibraryEntry::FromBearingAlongGirder );
   }
#endif

   *pStartEndDistance = leftEndDistance;
   *pEndEndDistance   = rightEndDistance;
}

void CBridgeAgentImp::GetSegmentBearingOffset(const CSegmentKey& segmentKey,Float64* pStartBearingOffset,Float64* pEndBearingOffset)
{
   CComPtr<IGirderLine> girderLine;
   GetGirderLine(segmentKey,&girderLine);

   girderLine->get_BearingOffset(etStart,pStartBearingOffset);
   girderLine->get_BearingOffset(etEnd,  pEndBearingOffset);
}

void CBridgeAgentImp::GetSegmentStorageSupportLocations(const CSegmentKey& segmentKey,Float64* pDistFromLeftEnd,Float64* pDistFromRightEnd)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   *pDistFromLeftEnd  = pSegment->HandlingData.LeftStoragePoint;
   *pDistFromRightEnd = pSegment->HandlingData.RightStoragePoint;
}

void CBridgeAgentImp::GetSegmentBottomFlangeProfile(const CSegmentKey& segmentKey,bool bIncludeClosure,IPoint2dCollection** points)
{
   GroupIndexType grpIdx = segmentKey.groupIndex;
   GirderIndexType gdrIdx = segmentKey.girderIndex;

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CSplicedGirderData* pGirder = pIBridgeDesc->GetBridgeDescription()->GetGirderGroup(grpIdx)->GetGirder(gdrIdx);
   GetSegmentBottomFlangeProfile(segmentKey,pGirder,bIncludeClosure,points);
}

void CBridgeAgentImp::GetSegmentBottomFlangeProfile(const CSegmentKey& segmentKey,const CSplicedGirderData* pGirder,bool bIncludeClosure,IPoint2dCollection** ppPoints)
{
   Float64 xStart,xEnd;
   GetSegmentRange(segmentKey,&xStart,&xEnd);

   if ( segmentKey.segmentIndex == 0 )
   {
      // if this is the first segment, adjust the girder path coordinate so that it starts at the face of the segment
      Float64 brgOffset = GetSegmentStartBearingOffset(segmentKey);
      Float64 endDist   = GetSegmentStartEndDistance(segmentKey);
      Float64 offset_dist = brgOffset - endDist;
      offset_dist = IsZero(offset_dist) ? 0 : offset_dist;
      xStart += offset_dist;
   }

   SegmentIndexType nSegments = GetSegmentCount(segmentKey);
   if ( segmentKey.segmentIndex == nSegments-1 )
   {
      // if this is the last segment, adjust the girder path coordinate so that it end at the face of the segment
      Float64 brgOffset = GetSegmentEndBearingOffset(segmentKey);
      Float64 endDist   = GetSegmentEndEndDistance(segmentKey);
      Float64 offset_dist = brgOffset - endDist;
      offset_dist = IsZero(offset_dist) ? 0 : offset_dist;
      xEnd -= offset_dist;
   }

   // if not including closure joint, adjust the start/end locations to be
   // at the actual start/end of the segment
   if ( !bIncludeClosure )
   {
      Float64 startBrgOffset, endBrgOffset;
      GetSegmentBearingOffset(segmentKey,&startBrgOffset,&endBrgOffset);

      Float64 startEndDist, endEndDist;
      GetSegmentEndDistance(segmentKey,&startEndDist,&endEndDist);

      if ( segmentKey.segmentIndex != 0 )
      {
         xStart += startBrgOffset - startEndDist;
      }

      if ( segmentKey.segmentIndex != nSegments-1 )
      {
         xEnd -= endBrgOffset   - endEndDist;
      }
   }

   Float64 segmentLength = GetSegmentLayoutLength(segmentKey);

   boost::shared_ptr<mathFunction2d> girder_depth         = CreateGirderProfile(pGirder);
   boost::shared_ptr<mathFunction2d> bottom_flange_height = CreateGirderBottomFlangeProfile(pGirder);

   CComPtr<IPoint2dCollection> points;
   points.CoCreateInstance(CLSID_Point2dCollection);

   std::vector<Float64> xValues;

   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);

   // Get variation lengths and convert from fractional measure if necessary (neg lengths are fractional)
   Float64 variationLength[4];
   for ( int i = 0; i < 4; i++ )
   {
      variationLength[i] = pSegment->GetVariationLength((pgsTypes::SegmentZoneType)i);
      if ( variationLength[i] < 0 )
      {
         ATLASSERT(-1.0 <= variationLength[i] && variationLength[i] <= 0.0);
         variationLength[i] *= -segmentLength;
      }
   }

   pgsTypes::SegmentVariationType variationType = pSegment->GetVariationType();
   if ( variationType == pgsTypes::svtLinear )
   {
      xValues.push_back(xStart + variationLength[pgsTypes::sztLeftPrismatic] );
      xValues.push_back(xEnd   - variationLength[pgsTypes::sztRightPrismatic] );
   }
   else if ( variationType == pgsTypes::svtParabolic )
   {
      Float64 xStartParabola = xStart + variationLength[pgsTypes::sztLeftPrismatic];
      Float64 xEndParabola   = xEnd   - variationLength[pgsTypes::sztRightPrismatic];
      xValues.push_back(xStartParabola);
      for ( int i = 0; i < 5; i++ )
      {
         Float64 x = i*(xEndParabola-xStartParabola)/5 + xStartParabola;
         xValues.push_back(x);
      }
      xValues.push_back(xEndParabola);
   }
   else if ( variationType == pgsTypes::svtDoubleLinear )
   {
      xValues.push_back(xStart + variationLength[pgsTypes::sztLeftPrismatic] );
      xValues.push_back(xStart + variationLength[pgsTypes::sztLeftPrismatic]  + variationLength[pgsTypes::sztLeftTapered] );
      xValues.push_back(xEnd   - variationLength[pgsTypes::sztRightPrismatic] - variationLength[pgsTypes::sztRightTapered] );
      xValues.push_back(xEnd   - variationLength[pgsTypes::sztRightPrismatic] );
   }
   else if ( variationType == pgsTypes::svtDoubleParabolic )
   {
      // left parabola
      Float64 xStartParabola = xStart + variationLength[pgsTypes::sztLeftPrismatic];
      Float64 xEndParabola   = xStart + variationLength[pgsTypes::sztLeftPrismatic]  + variationLength[pgsTypes::sztLeftTapered];

      xValues.push_back(xStartParabola);
      for ( int i = 0; i < 5; i++ )
      {
         Float64 x = i*(xEndParabola-xStartParabola)/5 + xStartParabola;
         xValues.push_back(x);
      }
      xValues.push_back(xEndParabola);

      // right parabola
      xStartParabola = xEnd   - variationLength[pgsTypes::sztRightPrismatic] - variationLength[pgsTypes::sztRightTapered];
      xEndParabola   = xEnd   - variationLength[pgsTypes::sztRightPrismatic];
      xValues.push_back(xStartParabola);
      for ( int i = 0; i < 5; i++ )
      {
         Float64 x = i*(xEndParabola-xStartParabola)/5 + xStartParabola;
         xValues.push_back(x);
      }
      xValues.push_back(xEndParabola);
   }

   // fill up with other points
   for ( int i = 0; i < 11; i++ )
   {
      Float64 x = i*(xEnd - xStart)/10 + xStart;
      xValues.push_back(x);
   }

   std::sort(xValues.begin(),xValues.end());

   std::vector<Float64>::iterator iter(xValues.begin());
   std::vector<Float64>::iterator end(xValues.end());
   for ( ; iter != end; iter++ )
   {
      Float64 x = *iter;
      Float64 y = -girder_depth->Evaluate(x) + bottom_flange_height->Evaluate(x);
      
      CComPtr<IPoint2d> point;
      point.CoCreateInstance(CLSID_Point2d);
      point->Move(x,y);
      points->Add(point);
   }

   points.CopyTo(ppPoints);
}

boost::shared_ptr<mathFunction2d> CBridgeAgentImp::CreateGirderProfile(const CSplicedGirderData* pGirder)
{
   return CreateGirderProfile(pGirder,true);
}

boost::shared_ptr<mathFunction2d> CBridgeAgentImp::CreateGirderBottomFlangeProfile(const CSplicedGirderData* pGirder)
{
   return CreateGirderProfile(pGirder,false);
}

boost::shared_ptr<mathFunction2d> CBridgeAgentImp::CreateGirderProfile(const CSplicedGirderData* pGirder,bool bGirderProfile)
{
#pragma Reminder("UPDATE: cache the profile function")
#pragma Reminder("UPDATE: use the methods from WBFL::GenericBridge") // remove this implementation and use the code in WBFL Generic Bridge
   // Generic Bridge does not have "What if" versions... that is, what if the girder looked like pGirder
   boost::shared_ptr<mathCompositeFunction2d> pCompositeFunction( new mathCompositeFunction2d() );

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GroupIndexType grpIdx = pGirder->GetGirderGroupIndex();
   GirderIndexType gdrIdx = pGirder->GetIndex();
   CSegmentKey segmentKey(grpIdx,gdrIdx,INVALID_INDEX);

   Float64 xSegmentStart = 0; // start of segment
   Float64 xStart        = 0; // start of curve section
   Float64 xEnd          = 0; // end of curve section

   bool bParabola = false;
   Float64 xParabolaStart,xParabolaEnd;
   Float64 yParabolaStart,yParabolaEnd;
   Float64 slopeParabola;

   // go down each segment and create piece-wise functions for each part of the spliced girder profile
   SegmentIndexType nSegments = pGirder->GetSegmentCount();
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      segmentKey.segmentIndex = segIdx;

      const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

      pgsTypes::SegmentVariationType  variation_type = pSegment->GetVariationType();

      Float64 segment_length = GetSegmentLayoutLength(segmentKey);
      if ( segIdx == 0 )
      {
         // start at face of first segment
         Float64 brgOffset = GetSegmentStartBearingOffset(segmentKey);
         Float64 endDist   = GetSegmentStartEndDistance(segmentKey);
         Float64 offset_dist = brgOffset - endDist;
         offset_dist = IsZero(offset_dist) ? 0 : offset_dist;
         xSegmentStart = offset_dist;
         segment_length -= offset_dist;
      }

      if ( segIdx == nSegments-1 )
      {
         Float64 brgOffset = GetSegmentEndBearingOffset(segmentKey);
         Float64 endDist   = GetSegmentEndEndDistance(segmentKey);
         Float64 offset_dist = brgOffset - endDist;
         offset_dist = IsZero(offset_dist) ? 0 : offset_dist;
         segment_length -= offset_dist;
      }

      xStart = xSegmentStart;

      Float64 h1,h2,h3,h4;
      if ( bGirderProfile )
      {
         // we are creating a girder profile
         h1 = pSegment->GetVariationHeight(pgsTypes::sztLeftPrismatic);
         h2 = pSegment->GetVariationHeight(pgsTypes::sztRightPrismatic);
         h3 = pSegment->GetVariationHeight(pgsTypes::sztLeftTapered);
         h4 = pSegment->GetVariationHeight(pgsTypes::sztRightTapered);
      }
      else
      {
         // we are creating a bottom flange profile
         h1 = pSegment->GetVariationBottomFlangeDepth(pgsTypes::sztLeftPrismatic);
         h2 = pSegment->GetVariationBottomFlangeDepth(pgsTypes::sztRightPrismatic);
         h3 = pSegment->GetVariationBottomFlangeDepth(pgsTypes::sztLeftTapered);
         h4 = pSegment->GetVariationBottomFlangeDepth(pgsTypes::sztRightTapered);
      }

      Float64 left_prismatic_length  = pSegment->GetVariationLength(pgsTypes::sztLeftPrismatic);
      Float64 left_taper_length      = pSegment->GetVariationLength(pgsTypes::sztLeftTapered);
      Float64 right_taper_length     = pSegment->GetVariationLength(pgsTypes::sztRightTapered);
      Float64 right_prismatic_length = pSegment->GetVariationLength(pgsTypes::sztRightPrismatic);

      // deal with fractional measure (fraction measures are < 0)
      if ( left_prismatic_length < 0 )
      {
         ATLASSERT(-1.0 <= left_prismatic_length && left_prismatic_length <= 0.0);
         left_prismatic_length *= -segment_length;
      }

      if ( left_taper_length < 0 )
      {
         ATLASSERT(-1.0 <= left_taper_length && left_taper_length <= 0.0);
         left_taper_length *= -segment_length;
      }

      if ( right_taper_length < 0 )
      {
         ATLASSERT(-1.0 <= right_taper_length && right_taper_length <= 0.0);
         right_taper_length *= -segment_length;
      }

      if ( right_prismatic_length < 0 )
      {
         ATLASSERT(-1.0 <= right_prismatic_length && right_prismatic_length <= 0.0);
         right_prismatic_length *= -segment_length;
      }

         // the girder profile has been modified by the user input


      if ( variation_type == pgsTypes::svtNone || 0 < left_prismatic_length )
      {
         // create a prismatic segment
         mathLinFunc2d func(0.0, h1);
         xEnd = xStart + (variation_type == pgsTypes::svtNone ? segment_length/2 : left_prismatic_length);
         pCompositeFunction->AddFunction(xStart,xEnd,func);
         slopeParabola = 0;
         xStart = xEnd;
      }

      if ( variation_type == pgsTypes::svtLinear )
      {
         // create a linear taper segment
         Float64 taper_length = segment_length - left_prismatic_length - right_prismatic_length;
         Float64 slope = (h2 - h1)/taper_length;
         Float64 b = h1 - slope*xStart;

         mathLinFunc2d func(slope, b);
         xEnd = xStart + taper_length;
         pCompositeFunction->AddFunction(xStart,xEnd,func);
         xStart = xEnd;
         slopeParabola = slope;
      }
      else if ( variation_type == pgsTypes::svtDoubleLinear )
      {
         // create a linear taper for left side of segment
         Float64 slope = (h3 - h1)/left_taper_length;
         Float64 b = h1 - slope*xStart;

         mathLinFunc2d left_func(slope, b);
         xEnd = xStart + left_taper_length;
         pCompositeFunction->AddFunction(xStart,xEnd,left_func);
         xStart = xEnd;

         // create a linear segment between left and right tapers
         Float64 taper_length = segment_length - left_prismatic_length - left_taper_length - right_prismatic_length - right_taper_length;
         slope = (h4 - h3)/taper_length;
         b = h3 - slope*xStart;

         mathLinFunc2d middle_func(slope, b);
         xEnd = xStart + taper_length;
         pCompositeFunction->AddFunction(xStart,xEnd,middle_func);
         xStart = xEnd;

         // create a linear taper for right side of segment
         slope = (h2 - h4)/right_taper_length;
         b = h4 - slope*xStart;

         mathLinFunc2d right_func(slope, b);
         xEnd = xStart + right_taper_length;
         pCompositeFunction->AddFunction(xStart,xEnd,right_func);
         xStart = xEnd;
         slopeParabola = slope;
      }
      else if ( variation_type == pgsTypes::svtParabolic )
      {
         if ( !bParabola )
         {
            // this is the start of a parabolic segment
            bParabola = true;
            xParabolaStart = xStart;
            yParabolaStart = h1;
         }

         // Parabola ends in this segment if
         // 1) this segment has a prismatic segment on the right end -OR-
         // 2) this is the last segment -OR-
         // 3) the next segment starts with a prismatic segment -OR-
         // 4) the next segment has a linear transition

         const CPrecastSegmentData* pNextSegment = (segIdx == nSegments-1 ? NULL : pGirder->GetSegment(segIdx+1));

         Float64 next_segment_left_prismatic_length = pNextSegment->GetVariationLength(pgsTypes::sztLeftPrismatic);
         if ( next_segment_left_prismatic_length < 0 )
         {
            ATLASSERT(-1.0 <= next_segment_left_prismatic_length && next_segment_left_prismatic_length < 0.0);
            next_segment_left_prismatic_length *= -GetSegmentLayoutLength(pNextSegment->GetSegmentKey());
         }

         if (
              0 < right_prismatic_length || // parabola ends in this segment -OR-
              segIdx == nSegments-1      || // this is the last segment (parabola ends here) -OR-
              0 < next_segment_left_prismatic_length || // next segment starts with prismatic section -OR-
              (pNextSegment->GetVariationType() == pgsTypes::svtNone || pNextSegment->GetVariationType() == pgsTypes::svtLinear || pNextSegment->GetVariationType() == pgsTypes::svtDoubleLinear) // next segment is linear 
            )
         {
            // parabola ends in this segment
            Float64 xParabolaEnd = xStart + segment_length - right_prismatic_length;
            Float64 yParabolaEnd = h2;

            if ( yParabolaEnd < yParabolaStart )
            {
               // slope at end is zero
               mathPolynomial2d func = GenerateParabola2(xParabolaStart,yParabolaStart,xParabolaEnd,yParabolaEnd,0.0);
               pCompositeFunction->AddFunction(xParabolaStart,xParabolaEnd,func);
            }
            else
            {
               // slope at start is zero
               mathPolynomial2d func = GenerateParabola1(xParabolaStart,yParabolaStart,xParabolaEnd,yParabolaEnd,slopeParabola);
               pCompositeFunction->AddFunction(xParabolaStart,xParabolaEnd,func);
            }

            bParabola = false;
            xStart = xParabolaEnd;
         }
         else
         {
            // parabola ends further down the girderline
            // do nothing???
         }
      }
      else if ( variation_type == pgsTypes::svtDoubleParabolic )
      {
         // left parabola ends in this segment
         if ( !bParabola )
         {
            // not currently in a parabola, based the start point on this segment
            xParabolaStart = xSegmentStart + left_prismatic_length;
            yParabolaStart = h1;
         }

#pragma Reminder("BUG: Assuming slope at start is zero, but it may not be if tangent to a linear segment")
         xParabolaEnd = xSegmentStart + left_prismatic_length + left_taper_length;
         yParabolaEnd = h3;
         mathPolynomial2d func_left_parabola;
         if ( yParabolaEnd < yParabolaStart )
         {
            func_left_parabola = GenerateParabola2(xParabolaStart,yParabolaStart,xParabolaEnd,yParabolaEnd,0.0);
         }
         else
         {
            func_left_parabola = GenerateParabola1(xParabolaStart,yParabolaStart,xParabolaEnd,yParabolaEnd,slopeParabola);
         }

         pCompositeFunction->AddFunction(xParabolaStart,xParabolaEnd,func_left_parabola);

         // parabola on right side of this segment starts here
         xParabolaStart = xSegmentStart + segment_length - right_prismatic_length - right_taper_length;
         yParabolaStart = h4;
         bParabola = true;

         if ( !IsZero(xParabolaStart - xParabolaEnd) )
         {
            // create a line segment between parabolas
            Float64 taper_length = segment_length - left_prismatic_length - left_taper_length - right_prismatic_length - right_taper_length;
            Float64 slope = -(h4 - h3)/taper_length;
            Float64 b = h3 - slope*xParabolaEnd;

            mathLinFunc2d middle_func(slope, b);
            pCompositeFunction->AddFunction(xParabolaEnd,xParabolaStart,middle_func);
            slopeParabola = slope;
         }

         // parabola ends in this segment if
         // 1) this is the last segment
         // 2) right prismatic section length > 0
         // 3) next segment is not parabolic
         const CPrecastSegmentData* pNextSegment = (segIdx == nSegments-1 ? 0 : pGirder->GetSegment(segIdx+1));
         if ( 0 < right_prismatic_length || 
              segIdx == nSegments-1      || 
              (pNextSegment->GetVariationType() == pgsTypes::svtNone || pNextSegment->GetVariationType() == pgsTypes::svtLinear || pNextSegment->GetVariationType() == pgsTypes::svtDoubleLinear) // next segment is linear 
            )
         {
            bParabola = false;
            xParabolaEnd = xSegmentStart + segment_length - right_prismatic_length;
            yParabolaEnd = h2;

     
            mathPolynomial2d func_right_parabola;
            if ( yParabolaEnd < yParabolaStart )
            {
               // compute slope at end of parabola
               if ( pNextSegment )
               {
                  CSegmentKey nextSegmentKey(segmentKey);
                  nextSegmentKey.segmentIndex++;

                  Float64 next_segment_length = GetSegmentLayoutLength(nextSegmentKey);
                  Float64 next_segment_left_prismatic_length = pNextSegment->GetVariationLength(pgsTypes::sztLeftPrismatic);
                  if ( next_segment_left_prismatic_length < 0 )
                  {
                     ATLASSERT(-1.0 <= next_segment_left_prismatic_length && next_segment_left_prismatic_length < 0.0);
                     next_segment_left_prismatic_length *= -next_segment_length;
                  }

                  Float64 next_segment_right_prismatic_length = pNextSegment->GetVariationLength(pgsTypes::sztRightPrismatic);
                  if ( next_segment_right_prismatic_length < 0 )
                  {
                     ATLASSERT(-1.0 <= next_segment_right_prismatic_length && next_segment_right_prismatic_length < 0.0);
                     next_segment_right_prismatic_length *= -next_segment_length;
                  }

                  Float64 next_segment_left_tapered_length = pNextSegment->GetVariationLength(pgsTypes::sztLeftTapered);
                  if ( next_segment_left_tapered_length < 0 )
                  {
                     ATLASSERT(-1.0 <= next_segment_left_tapered_length && next_segment_left_tapered_length < 0.0);
                     next_segment_left_tapered_length *= -next_segment_length;
                  }

                  if ( pNextSegment->GetVariationType() == pgsTypes::svtLinear )
                  {
                     // next segment is linear
                     if ( IsZero(next_segment_left_prismatic_length) )
                     {
                        Float64 dist = next_segment_length - next_segment_left_prismatic_length - next_segment_right_prismatic_length;
                        slopeParabola = -(pNextSegment->GetVariationHeight(pgsTypes::sztRightPrismatic) - pNextSegment->GetVariationHeight(pgsTypes::sztLeftPrismatic))/dist;
                     }
                  }
                  else if ( pNextSegment->GetVariationType() == pgsTypes::svtDoubleLinear )
                  {
                     if ( IsZero(next_segment_left_prismatic_length) )
                     {
                        Float64 dist = next_segment_left_tapered_length;
                        slopeParabola = -(pNextSegment->GetVariationHeight(pgsTypes::sztLeftTapered) - pNextSegment->GetVariationHeight(pgsTypes::sztLeftPrismatic))/dist;
                     }
                  }
               }
               else
               {
                  slopeParabola = 0;
               }
               func_right_parabola = GenerateParabola2(xParabolaStart,yParabolaStart,xParabolaEnd,yParabolaEnd,slopeParabola);
            }
            else
            {
               func_right_parabola = GenerateParabola1(xParabolaStart,yParabolaStart,xParabolaEnd,yParabolaEnd,slopeParabola);
            }

            pCompositeFunction->AddFunction(xParabolaStart,xParabolaEnd,func_right_parabola);
         }
         else
         {
            // parabola ends further down the girderline
            bParabola = true;
         }

         xStart = xSegmentStart + segment_length - right_prismatic_length;
      }

      if ( variation_type == pgsTypes::svtNone || 0 < right_prismatic_length )
      {
         // create a prismatic segment
         mathLinFunc2d func(0.0, h2);
         xEnd = xStart + (variation_type == pgsTypes::svtNone ? segment_length/2 : right_prismatic_length);
         pCompositeFunction->AddFunction(xStart,xEnd,func);
         slopeParabola = 0;
         xStart = xEnd;
      }

      xSegmentStart += segment_length;
   }

   return pCompositeFunction;
}

void CBridgeAgentImp::GetSegmentRange(const CSegmentKey& segmentKey,Float64* pXgpStart,Float64* pXgpEnd)
{
   // Returns the start and end of the segment in Girder Path Coordiantes
   CSegmentKey firstSegmentKey(segmentKey);
   firstSegmentKey.segmentIndex = 0;

   *pXgpStart = 0;

   // adding layout length of all preceeding segments will get us to the start of this segment
   for ( SegmentIndexType s = 0; s < segmentKey.segmentIndex; s++ )
   {
      CSegmentKey key(segmentKey.groupIndex,segmentKey.girderIndex,s);
      *pXgpStart += GetSegmentLayoutLength(key);
   }

   *pXgpEnd = *pXgpStart + GetSegmentLayoutLength(segmentKey);
}

void CBridgeAgentImp::GetSegmentDirection(const CSegmentKey& segmentKey,IDirection** ppDirection)
{
   VALIDATE(BRIDGE);

   CComPtr<IGirderLine> girderLine;
   GetGirderLine(segmentKey,&girderLine);

   girderLine->get_Direction(ppDirection);
}

///////////////////////////////////////////////////////////////////////////////////////////
// ITendonGeometry
void CBridgeAgentImp::GetDuctCenterline(const CGirderKey& girderKey,DuctIndexType ductIdx,IPoint2dCollection** ppPoints)
{
   // Gets the duct centerline from the generic bridge model
   CComPtr<IPoint2dCollection> points;
   points.CoCreateInstance(CLSID_Point2dCollection);

   CComPtr<ISuperstructureMember> ssMbr;
   GetSuperstructureMember(girderKey,&ssMbr);

   CComQIPtr<IItemData> itemData(ssMbr);
   CComPtr<IUnknown> unk;
   itemData->GetItemData(CComBSTR(_T("Tendons")),&unk);

   CComQIPtr<ITendonCollection> tendons(unk);

   CComPtr<ITendon> tendon;
   tendons->get_Item(ductIdx,&tendon);

   CComPtr<IPoint3dCollection> centerline;
   tendon->get_Centerline(tmPath,&centerline);

   CComPtr<IEnumPoint3d> enumPoints;
   centerline->get__Enum(&enumPoints);
   CComPtr<IPoint3d> point;
   while ( enumPoints->Next(1,&point,NULL) != S_FALSE )
   {
      Float64 x,y,z;
      point->Location(&x,&y,&z);

      CComPtr<IPoint2d> p;
      p.CoCreateInstance(CLSID_Point2d);
      p->Move(z,y);
      
      points->Add(p);

      point.Release();
   }

   points.CopyTo(ppPoints);
}

DuctIndexType CBridgeAgentImp::GetDuctCount(const CGirderKey& girderKey)
{
   CComPtr<ISuperstructureMember> ssMbr;
   GetSuperstructureMember(girderKey,&ssMbr);

   CComQIPtr<IItemData> itemData(ssMbr);

   CComPtr<IUnknown> unk;
   itemData->GetItemData(CComBSTR(_T("Tendons")),&unk);

   CComQIPtr<ITendonCollection> tendons(unk);

   DuctIndexType nDucts;
   tendons->get_Count(&nDucts);
   return nDucts;
}

void CBridgeAgentImp::GetDuctCenterline(const CGirderKey& girderKey,DuctIndexType ductIdx,const CSplicedGirderData* pGirder,IPoint2dCollection** ppPoints)
{
   // Get the duct centerline based on the spliced girder input data
   const CDuctData* pDuctData = pGirder->GetPostTensioning()->GetDuct(ductIdx);

   switch ( pDuctData->DuctGeometryType )
   {
   case CDuctGeometry::Linear:
      CreateDuctCenterline(girderKey,pDuctData->LinearDuctGeometry,ppPoints);
      break;

   case CDuctGeometry::Parabolic:
      CreateDuctCenterline(girderKey,pDuctData->ParabolicDuctGeometry,ppPoints);
      break;

   case CDuctGeometry::Offset:
      GetDuctCenterline(girderKey,pDuctData->OffsetDuctGeometry.RefDuctIdx,pGirder,ppPoints);
      CreateDuctCenterline(girderKey,pDuctData->OffsetDuctGeometry,ppPoints);
      break;
   }
}

mathPolynomial2d CBridgeAgentImp::GenerateParabola1(Float64 x1,Float64 y1,Float64 x2,Float64 y2,Float64 slope)
{
   // slope is known at left end
   Float64 A = ((y2-y1) - (x2-x1)*slope)/((x2-x1)*(x2-x1));
   Float64 B = slope - 2*A*x1;
   Float64 C = y1 - A*x1*x1 - B*x1;

   std::vector<Float64> coefficients;
   coefficients.push_back(A);
   coefficients.push_back(B);
   coefficients.push_back(C);

   return mathPolynomial2d(coefficients);
}

mathPolynomial2d CBridgeAgentImp::GenerateParabola2(Float64 x1,Float64 y1,Float64 x2,Float64 y2,Float64 slope)
{
   // slope is known at right end
   Float64 A = -((y2-y1) - (x2-x1)*slope)/((x2-x1)*(x2-x1));
   Float64 B = slope - 2*A*x2;
   Float64 C = y1 - A*x1*x1 - B*x1;

   std::vector<Float64> coefficients;
   coefficients.push_back(A);
   coefficients.push_back(B);
   coefficients.push_back(C);

   return mathPolynomial2d(coefficients);
}

void CBridgeAgentImp::GenerateReverseParabolas(Float64 x1,Float64 y1,Float64 x2,Float64 x3,Float64 y3,mathPolynomial2d* pLeftParabola,mathPolynomial2d* pRightParabola)
{
   Float64 y2 = (y3*(x2-x1) + y1*(x3-x2))/(x3 - x1);

   *pLeftParabola  = GenerateParabola1(x1,y1,x2,y2,0.0);
   *pRightParabola = GenerateParabola2(x2,y2,x3,y3,0.0);
}

mathCompositeFunction2d CBridgeAgentImp::CreateDuctCenterline(const CGirderKey& girderKey,const CLinearDuctGeometry& geometry)
{
   mathCompositeFunction2d fnCenterline;

   Float64 x1 = 0;
   Float64 y1 = 0;
   CollectionIndexType nPoints = geometry.GetPointCount();
   for ( CollectionIndexType idx = 1; idx < nPoints; idx++ )
   {
      Float64 distFromPrev;
      Float64 offset;
      CDuctGeometry::OffsetType offsetType;
      geometry.GetPoint(idx-1,&distFromPrev,&offset,&offsetType);

      x1 += distFromPrev;
      y1 = ConvertDuctOffsetToDuctElevation(girderKey,x1,offset,offsetType);

      geometry.GetPoint(idx,&distFromPrev,&offset,&offsetType);

      Float64 x2,y2;
      x2 = x1 + distFromPrev;
      y2 = ConvertDuctOffsetToDuctElevation(girderKey,x2,offset,offsetType);

      Float64 m = (y2-y1)/(x2-x1);
      Float64 b = y2 - m*x2;

      mathLinFunc2d fn(m,b);

      fnCenterline.AddFunction(x1,x2,fn);

      x1 = x2;
      y1 = y2;
   }

   return fnCenterline;
}

void CBridgeAgentImp::CreateDuctCenterline(const CGirderKey& girderKey,const CLinearDuctGeometry& geometry,IPoint2dCollection** ppPoints)
{
   CComPtr<IPoint2dCollection> points;
   points.CoCreateInstance(CLSID_Point2dCollection);
   points.CopyTo(ppPoints);

   Float64 Lg = GetGirderLength(girderKey);

   Float64 Xg = 0;

   CollectionIndexType nPoints = geometry.GetPointCount();
   for ( CollectionIndexType idx = 0; idx < nPoints; idx++ )
   {
      Float64 location;
      Float64 offset;
      CDuctGeometry::OffsetType offsetType;
      geometry.GetPoint(idx,&location,&offset,&offsetType);

      if ( geometry.GetMeasurementType() == CLinearDuctGeometry::FromPrevious )
      {
         Xg += location;
      }
      else
      {
         if ( location < 0 )
         {
            // location is fractional
            location *= -Lg;
         }

         Xg = location;
      }

      Float64 y = ConvertDuctOffsetToDuctElevation(girderKey,Xg,offset,offsetType);

      CComPtr<IPoint2d> point;
      point.CoCreateInstance(CLSID_Point2d);
      point->Move(Xg,y);
      points->Add(point);
   }
}

void CBridgeAgentImp::GetDuctPoint(const pgsPointOfInterest& poi,DuctIndexType ductIdx,IPoint2d** ppPoint)
{
   ATLASSERT(IsOnGirder(poi));
   Float64 Xg = ConvertPoiToGirderCoordinate(poi);
   const CGirderKey& girderKey(poi.GetSegmentKey());
   GetDuctPoint(girderKey,Xg,ductIdx,ppPoint);
}

void CBridgeAgentImp::GetDuctPoint(const CGirderKey& girderKey,Float64 Xg,DuctIndexType ductIdx,IPoint2d** ppPoint)
{
   CComPtr<ISuperstructureMember> ssMbr;
   GetSuperstructureMember(girderKey,&ssMbr);

   CComQIPtr<IItemData> itemData(ssMbr);

   CComPtr<IUnknown> unk;
   itemData->GetItemData(CComBSTR(_T("Tendons")),&unk);

   CComQIPtr<ITendonCollection> tendons(unk);
   CComPtr<ITendon> tendon;
   tendons->get_Item(ductIdx,&tendon);

   CComPtr<IPoint3d> cg;
   tendon->get_CG(Xg,tmPath,&cg);

   Float64 x,y,z;
   cg->Location(&x,&y,&z);
   ATLASSERT(IsEqual(z,Xg));

   CComPtr<IPoint2d> pnt;
   pnt.CoCreateInstance(CLSID_Point2d);
   pnt->Move(x,y);

   pnt.CopyTo(ppPoint);
}

Float64 CBridgeAgentImp::GetOutsideDiameter(const CGirderKey& girderKey,DuctIndexType ductIdx)
{
   CComPtr<ISuperstructureMember> ssMbr;
   GetSuperstructureMember(girderKey,&ssMbr);

   CComQIPtr<IItemData> itemData(ssMbr);

   CComPtr<IUnknown> unk;
   itemData->GetItemData(CComBSTR(_T("Tendons")),&unk);

   CComQIPtr<ITendonCollection> tendons(unk);
   CComPtr<ITendon> tendon;
   tendons->get_Item(ductIdx,&tendon);

   Float64 dia;
   tendon->get_OutsideDiameter(&dia);

   return dia;
}

Float64 CBridgeAgentImp::GetInsideDiameter(const CGirderKey& girderKey,DuctIndexType ductIdx)
{
   CComPtr<ISuperstructureMember> ssMbr;
   GetSuperstructureMember(girderKey,&ssMbr);

   CComQIPtr<IItemData> itemData(ssMbr);

   CComPtr<IUnknown> unk;
   itemData->GetItemData(CComBSTR(_T("Tendons")),&unk);

   CComQIPtr<ITendonCollection> tendons(unk);
   CComPtr<ITendon> tendon;
   tendons->get_Item(ductIdx,&tendon);

   Float64 dia;
   tendon->get_InsideDiameter(&dia);

   return dia;
}

Float64 CBridgeAgentImp::GetInsideDuctArea(const CGirderKey& girderKey,DuctIndexType ductIdx)
{
   CComPtr<ISuperstructureMember> ssMbr;
   GetSuperstructureMember(girderKey,&ssMbr);

   CComQIPtr<IItemData> itemData(ssMbr);

   CComPtr<IUnknown> unk;
   itemData->GetItemData(CComBSTR(_T("Tendons")),&unk);

   CComQIPtr<ITendonCollection> tendons(unk);
   CComPtr<ITendon> tendon;
   tendons->get_Item(ductIdx,&tendon);

   Float64 Aduct;
   tendon->get_InsideDuctArea(&Aduct);

   return Aduct;
}

StrandIndexType CBridgeAgentImp::GetTendonStrandCount(const CGirderKey& girderKey,DuctIndexType ductIdx)
{
   CComPtr<ISuperstructureMember> ssMbr;
   GetSuperstructureMember(girderKey,&ssMbr);

   CComQIPtr<IItemData> itemData(ssMbr);

   CComPtr<IUnknown> unk;
   itemData->GetItemData(CComBSTR(_T("Tendons")),&unk);

   CComQIPtr<ITendonCollection> tendons(unk);
   CComPtr<ITendon> tendon;
   tendons->get_Item(ductIdx,&tendon);

   StrandIndexType nStrands;
   tendon->get_StrandCount(&nStrands);

   return nStrands;
}

Float64 CBridgeAgentImp::GetTendonArea(const CGirderKey& girderKey,IntervalIndexType intervalIdx,DuctIndexType ductIdx)
{
   if ( intervalIdx < GetStressTendonInterval(girderKey,ductIdx) )
   {
      return 0;
   }

   CComPtr<ISuperstructureMember> ssMbr;
   GetSuperstructureMember(girderKey,&ssMbr);

   CComQIPtr<IItemData> itemData(ssMbr);

   CComPtr<IUnknown> unk;
   itemData->GetItemData(CComBSTR(_T("Tendons")),&unk);

   CComQIPtr<ITendonCollection> tendons(unk);
   CComPtr<ITendon> tendon;
   tendons->get_Item(ductIdx,&tendon);

   Float64 Apt;
   tendon->get_TendonArea(&Apt);

   return Apt;
}

Float64 CBridgeAgentImp::GetPjack(const CGirderKey& girderKey,DuctIndexType ductIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPTData* pPTData = pIBridgeDesc->GetPostTensioning(girderKey);
   WebIndexType nWebs = GetWebCount(girderKey);
   return pPTData->GetPjack(ductIdx/nWebs);
}

Float64 CBridgeAgentImp::GetFpj(const CGirderKey& girderKey,DuctIndexType ductIdx)
{
   Float64 Pj = GetPjack(girderKey,ductIdx);
   IntervalIndexType stressTendonIntervalIdx = GetStressTendonInterval(girderKey,ductIdx);
   Float64 Apt = GetTendonArea(girderKey,stressTendonIntervalIdx,ductIdx);
   return (IsZero(Apt) ? 0 : Pj/Apt);
}

Float64 CBridgeAgentImp::GetDuctOffset(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,DuctIndexType ductIdx)
{
   // Returns the distance from the top of the non-composite girder to the CG of the tendon

   // if interval is before tendon is installed, there isn't an eccentricity with respect to the girder cross section
   if ( intervalIdx < GetStressTendonInterval(poi.GetSegmentKey(),ductIdx) )
   {
      return 0;
   }

   if ( !IsOnGirder(poi) )
   {
      return 0;
   }

   CComPtr<ISuperstructureMember> ssMbr;
   GetSuperstructureMember(poi.GetSegmentKey(),&ssMbr);

   CComQIPtr<IItemData> itemData(ssMbr);

   CComPtr<IUnknown> unk;
   itemData->GetItemData(CComBSTR(_T("Tendons")),&unk);

   CComQIPtr<ITendonCollection> tendons(unk);
   CComPtr<ITendon> tendon;
   tendons->get_Item(ductIdx,&tendon);

   StrandIndexType nStrands;
   tendon->get_StrandCount(&nStrands);
   if ( nStrands == 0 )
   {
      return 0;
   }

   Float64 Xg = ConvertPoiToGirderCoordinate(poi);
   CComPtr<IPoint3d> cg;
   tendon->get_CG(Xg,tmTendon /*account for tendon offset from center of duct*/,&cg);

   Float64 ecc;
   cg->get_Y(&ecc); // this is in Girder Section Coordinates so it should be < 0 if below top of girder
   ATLASSERT(ecc <= 0);

   return ecc;
}

Float64 CBridgeAgentImp::GetDuctLength(const CGirderKey& girderKey,DuctIndexType ductIdx)
{
   CComPtr<ISuperstructureMember> ssMbr;
   GetSuperstructureMember(girderKey,&ssMbr);

   CComQIPtr<IItemData> itemData(ssMbr);

   CComPtr<IUnknown> unk;
   itemData->GetItemData(CComBSTR(_T("Tendons")),&unk);

   CComQIPtr<ITendonCollection> tendons(unk);
   CComPtr<ITendon> tendon;
   tendons->get_Item(ductIdx,&tendon);

   Float64 length;
   tendon->get_Length(&length);
   return length;
}

Float64 CBridgeAgentImp::GetEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,DuctIndexType ductIdx)
{
   pgsTypes::SectionPropertyType spType = (pgsTypes::SectionPropertyType)(GetSectionPropertiesMode());
   return GetEccentricity(spType,intervalIdx,poi,ductIdx);
}

Float64 CBridgeAgentImp::GetEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,DuctIndexType ductIdx)
{
   ATLASSERT(ductIdx != INVALID_INDEX);

   // if interval is before tendon is installed, there isn't an eccentricity with respect to the girder cross section
   IntervalIndexType stressTendonIntervalIdx = GetStressTendonInterval(poi.GetSegmentKey(),ductIdx);
   if ( intervalIdx < stressTendonIntervalIdx )
   {
      return 0;
   }

   if ( !IsOnGirder(poi) )
   {
      return 0;
   }

   Float64 duct_offset = GetDuctOffset(intervalIdx,poi,ductIdx); 
   // distance from top of non-composite girder to CG of tendon
   // negative value indicates the duct is below the top of the girder

   // Get Ytop
   Float64 Ytop = GetY(spType,intervalIdx,poi,pgsTypes::TopGirder); 
   // distance from CG of total cross section in this interval to top of girder

   // Everything is measured from top of girder... compute eccentricity
   Float64 ecc = -(duct_offset + Ytop);

   // NOTE: ecc is positive if it is below the CG
   return ecc;
}

void CBridgeAgentImp::GetTendonSlope(const pgsPointOfInterest& poi,DuctIndexType ductIdx,IVector3d** ppSlope)
{
   ATLASSERT(IsOnGirder(poi));
   Float64 Xg = ConvertPoiToGirderCoordinate(poi);
   CGirderKey girderKey(poi.GetSegmentKey());
   GetTendonSlope(girderKey,Xg,ductIdx,ppSlope);
}

void CBridgeAgentImp::GetTendonSlope(const CGirderKey& girderKey,Float64 Xg,DuctIndexType ductIdx,IVector3d** ppSlope)
{
   CComPtr<ISuperstructureMember> ssMbr;
   GetSuperstructureMember(girderKey,&ssMbr);

   CComQIPtr<IItemData> itemData(ssMbr);

   CComPtr<IUnknown> unk;
   itemData->GetItemData(CComBSTR(_T("Tendons")),&unk);

   CComQIPtr<ITendonCollection> tendons(unk);
   CComPtr<ITendon> tendon;
   tendons->get_Item(ductIdx,&tendon);
   tendon->get_Slope(Xg,tmTendon,ppSlope);
}

Float64 CBridgeAgentImp::GetMinimumRadiusOfCurvature(const CGirderKey& girderKey,DuctIndexType ductIdx)
{
   CComPtr<ISuperstructureMember> ssMbr;
   GetSuperstructureMember(girderKey,&ssMbr);

   CComQIPtr<IItemData> itemData(ssMbr);

   CComPtr<IUnknown> unk;
   itemData->GetItemData(CComBSTR(_T("Tendons")),&unk);

   CComQIPtr<ITendonCollection> tendons(unk);
   CComPtr<ITendon> tendon;
   tendons->get_Item(ductIdx,&tendon);

   Float64 radiusOfCurvature;
   tendon->get_MinimumRadiusOfCurvature(&radiusOfCurvature);
   return radiusOfCurvature;
}

Float64 CBridgeAgentImp::GetAngularChange(const pgsPointOfInterest& poi,DuctIndexType ductIdx,pgsTypes::MemberEndType endType)
{
   if ( !IsOnGirder(poi) )
   {
      return 0;
   }

   // get the poi at the start of this girder
   CSegmentKey segmentKey(poi.GetSegmentKey());

   pgsPointOfInterest poi1, poi2;
   if ( endType == pgsTypes::metStart )
   {
      // angular changes are measured from the start of the girder (i.e. jacking at the left end)
      segmentKey.segmentIndex = 0; // always occurs in segment 0
      poi1 = GetPointOfInterest(segmentKey,0.0); // poi at start of first segment
      poi2 = poi;
   }
   else
   {
      // angular changes are measured from the end of the girder (i.e. jacking at the right end)
      poi2 = poi;
      SegmentIndexType nSegments = GetSegmentCount(segmentKey);
      segmentKey.segmentIndex = nSegments-1;
      Float64 L = GetSegmentLength(segmentKey);
      poi1 = GetPointOfInterest(segmentKey,L); // poi at end of last segment
   }

   return GetAngularChange(poi1,poi2,ductIdx);
}

Float64 CBridgeAgentImp::GetAngularChange(const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,DuctIndexType ductIdx)
{
   ATLASSERT(IsOnGirder(poi1));
   ATLASSERT(IsOnGirder(poi2));

   // POIs must be on the same girder
   ATLASSERT(CGirderKey(poi1.GetSegmentKey()) == CGirderKey(poi2.GetSegmentKey()));

   Float64 alpha = 0.0; // angular change

   // Get all the POI for this girder
   ATLASSERT(CGirderKey(poi1.GetSegmentKey()) == CGirderKey(poi2.GetSegmentKey()));
   GroupIndexType  grpIdx = poi1.GetSegmentKey().groupIndex;
   GirderIndexType gdrIdx = poi1.GetSegmentKey().girderIndex;

   std::vector<pgsPointOfInterest> vPoi( GetPointsOfInterest(CSegmentKey(grpIdx,gdrIdx,ALL_SEGMENTS)) );
   // vPoi is sorted left to right... if we are measuring angular change from right to left, order of the
   // elements in the vector have to be reversed
   bool bLeftToRight = true;
   if ( poi2 < poi1 )
   {
      bLeftToRight = false;
      std::reverse(vPoi.begin(),vPoi.end());
   }
   std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
   pgsPointOfInterest prevPoi = *iter++;

   std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      pgsPointOfInterest& poi = *iter;
      if ( bLeftToRight )
      {
         if ( prevPoi < poi1 )
         {
            continue; // before the start poi... continue with next poi
         }
         
         if ( poi2 < poi ) // after end poi... just break out of the loop
         {
            break;
         }
      }
      else
      {
         if ( poi1 < prevPoi )
         {
            continue; // prev poi is before the first poi (going right to left)
         }

         if ( poi < poi2 )
         {
            break; // current poi is after the second poi (going right to left)
         }
      }

      CComPtr<IVector3d> slope1, slope2;
      GetTendonSlope(prevPoi,ductIdx,&slope1);
      GetTendonSlope(poi,    ductIdx,&slope2);

      Float64 delta_alpha;
      slope2->AngleBetween(slope1,&delta_alpha);

      alpha += delta_alpha;

      prevPoi = poi;
   }

   return alpha;
}

pgsTypes::JackingEndType CBridgeAgentImp::GetJackingEnd(const CGirderKey& girderKey,DuctIndexType ductIdx)
{
   CComPtr<ISuperstructureMember> ssMbr;
   GetSuperstructureMember(girderKey,&ssMbr);

   CComQIPtr<IItemData> itemData(ssMbr);

   CComPtr<IUnknown> unk;
   itemData->GetItemData(CComBSTR(_T("Tendons")),&unk);

   CComQIPtr<ITendonCollection> tendons(unk);
   CComPtr<ITendon> tendon;
   tendons->get_Item(ductIdx,&tendon);

   JackingEndType tj;
   tendon->get_JackingEnd(&tj);

   return (pgsTypes::JackingEndType)tj;
}

Float64 CBridgeAgentImp::GetAptTopHalf(const pgsPointOfInterest& poi)
{
   return GetAptTensionSide(poi,true);
}

Float64 CBridgeAgentImp::GetAptBottomHalf(const pgsPointOfInterest& poi)
{
   return GetAptTensionSide(poi,false);
}

//////////////////////////////////////////////////////////////////////////
// IIntervals
IntervalIndexType CBridgeAgentImp::GetIntervalCount()
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetIntervalCount();
}

EventIndexType CBridgeAgentImp::GetStartEvent(IntervalIndexType idx)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetStartEvent(idx);
}

EventIndexType CBridgeAgentImp::GetEndEvent(IntervalIndexType idx)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetEndEvent(idx);
}

Float64 CBridgeAgentImp::GetTime(IntervalIndexType idx,pgsTypes::IntervalTimeType timeType)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetTime(idx,timeType);
}

Float64 CBridgeAgentImp::GetDuration(IntervalIndexType idx)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetDuration(idx);
}

LPCTSTR CBridgeAgentImp::GetDescription(IntervalIndexType idx)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetDescription(idx);
}

IntervalIndexType CBridgeAgentImp::GetInterval(EventIndexType eventIdx)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetInterval(eventIdx);
}

IntervalIndexType CBridgeAgentImp::GetErectPierInterval(PierIndexType pierIdx)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetErectPierInterval(pierIdx);
}

IntervalIndexType CBridgeAgentImp::GetCastClosureJointInterval(const CClosureKey& closureKey)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetCastClosureInterval(closureKey);
}

IntervalIndexType CBridgeAgentImp::GetCompositeClosureJointInterval(const CClosureKey& closureKey)
{
   SegmentIndexType nSegments = GetSegmentCount(closureKey);
   if ( closureKey.segmentIndex == nSegments-1 )
   {
      return INVALID_INDEX; // asking for closure joint after the last segment.. it doesn't exist... return INVALID_INDEX
   }

   // closure joints are composite with the girder two intervals after they are cast
   // cast interval (+0), curing interval (+1), composite interval (+2)
   return GetCastClosureJointInterval(closureKey)+2;
}

void CBridgeAgentImp::GetContinuityInterval(const CGirderKey& girderKey,PierIndexType pierIdx,IntervalIndexType* pBack,IntervalIndexType* pAhead)
{
   // Get the events when continuity is achieved
   EventIndexType backContinuityEventIdx, aheadContinuityEventIdx;
   GetContinuityEventIndex(pierIdx,&backContinuityEventIdx,&aheadContinuityEventIdx);

   // Get the interval that corresponds to those events
   *pBack  = GetInterval(backContinuityEventIdx);
   *pAhead = GetInterval(aheadContinuityEventIdx);
}

IntervalIndexType CBridgeAgentImp::GetCastDeckInterval()
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetCastDeckInterval();
}

IntervalIndexType CBridgeAgentImp::GetCompositeDeckInterval()
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetCompositeDeckInterval();
}

IntervalIndexType CBridgeAgentImp::GetFirstStressStrandInterval(const CGirderKey& girderKey)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetFirstStressStrandInterval(girderKey);
}

IntervalIndexType CBridgeAgentImp::GetLastStressStrandInterval(const CGirderKey& girderKey)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetLastStressStrandInterval(girderKey);
}

IntervalIndexType CBridgeAgentImp::GetStressStrandInterval(const CSegmentKey& segmentKey)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetStressStrandInterval(segmentKey);
}

IntervalIndexType CBridgeAgentImp::GetFirstPrestressReleaseInterval(const CGirderKey& girderKey)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetFirstPrestressReleaseInterval(girderKey);
}

IntervalIndexType CBridgeAgentImp::GetLastPrestressReleaseInterval(const CGirderKey& girderKey)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetLastPrestressReleaseInterval(girderKey);
}

IntervalIndexType CBridgeAgentImp::GetPrestressReleaseInterval(const CSegmentKey& segmentKey)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetPrestressReleaseInterval(segmentKey);
}

IntervalIndexType CBridgeAgentImp::GetLiftSegmentInterval(const CSegmentKey& segmentKey)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetLiftingInterval(segmentKey);
}

IntervalIndexType CBridgeAgentImp::GetFirstLiftSegmentInterval(const CGirderKey& girderKey)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetFirstLiftingInterval(girderKey);
}

IntervalIndexType CBridgeAgentImp::GetLastLiftSegmentInterval(const CGirderKey& girderKey)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetLastLiftingInterval(girderKey);
}

IntervalIndexType CBridgeAgentImp::GetStorageInterval(const CSegmentKey& segmentKey)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetStorageInterval(segmentKey);
}

IntervalIndexType CBridgeAgentImp::GetFirstStorageInterval(const CGirderKey& girderKey)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetFirstStorageInterval(girderKey);
}

IntervalIndexType CBridgeAgentImp::GetLastStorageInterval(const CGirderKey& girderKey)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetLastStorageInterval(girderKey);
}

IntervalIndexType CBridgeAgentImp::GetHaulSegmentInterval(const CSegmentKey& segmentKey)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetHaulingInterval(segmentKey);
}

IntervalIndexType CBridgeAgentImp::GetFirstSegmentErectionInterval(const CGirderKey& girderKey)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetFirstSegmentErectionInterval(girderKey);
}

IntervalIndexType CBridgeAgentImp::GetLastSegmentErectionInterval(const CGirderKey& girderKey)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetLastSegmentErectionInterval(girderKey);
}

IntervalIndexType CBridgeAgentImp::GetErectSegmentInterval(const CSegmentKey& segmentKey)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetErectSegmentInterval(segmentKey);
}

bool CBridgeAgentImp::IsSegmentErectionInterval(IntervalIndexType intervalIdx)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.IsSegmentErectionInterval(intervalIdx);
}

bool CBridgeAgentImp::IsSegmentErectionInterval(const CGirderKey& girderKey,IntervalIndexType intervalIdx)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.IsSegmentErectionInterval(girderKey,intervalIdx);
}

IntervalIndexType CBridgeAgentImp::GetTemporaryStrandStressingInterval(const CSegmentKey& segmentKey)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   if ( pSegment->Strands.GetTemporaryStrandUsage() == pgsTypes::ttsPretensioned )
   {
      return GetStressStrandInterval(segmentKey);
   }
   else
   {
      // temp strands are post-tensioned.. they are stressed in the same interval they are installed
      return GetTemporaryStrandInstallationInterval(segmentKey);
   }
}

IntervalIndexType CBridgeAgentImp::GetTemporaryStrandInstallationInterval(const CSegmentKey& segmentKey)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);

   IntervalIndexType intervalIdx;
   switch( pSegment->Strands.GetTemporaryStrandUsage() )
   {
   case pgsTypes::ttsPretensioned:
      intervalIdx = GetPrestressReleaseInterval(segmentKey);
      break;

   case pgsTypes::ttsPTBeforeLifting:
   case pgsTypes::ttsPTAfterLifting:
      intervalIdx = GetLiftSegmentInterval(segmentKey);
      break;
      
   case pgsTypes::ttsPTBeforeShipping:
      intervalIdx = GetHaulSegmentInterval(segmentKey);
      break;
   }

   return intervalIdx;
}

IntervalIndexType CBridgeAgentImp::GetTemporaryStrandRemovalInterval(const CSegmentKey& segmentKey)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetTemporaryStrandRemovalInterval(segmentKey);
}

IntervalIndexType CBridgeAgentImp::GetLiveLoadInterval()
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetLiveLoadInterval();
}

IntervalIndexType CBridgeAgentImp::GetLoadRatingInterval()
{
   VALIDATE(BRIDGE);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();
   EventIndexType eventIdx = pTimelineMgr->GetLoadRatingEventIndex();
   if ( eventIdx == INVALID_INDEX )
   {
      // load rating event wasn't specifically defined... assum last interval
      return m_IntervalManager.GetIntervalCount()-1;
   }
   else
   {
      return m_IntervalManager.GetInterval(eventIdx);
   }
}

IntervalIndexType CBridgeAgentImp::GetOverlayInterval()
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetOverlayInterval();
}

IntervalIndexType CBridgeAgentImp::GetInstallRailingSystemInterval()
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetInstallRailingSystemInterval();
}

IntervalIndexType CBridgeAgentImp::GetFirstTendonStressingInterval(const CGirderKey& girderKey)
{
   VALIDATE(BRIDGE);

   return m_IntervalManager.GetFirstTendonStressingInterval(girderKey);
}

IntervalIndexType CBridgeAgentImp::GetLastTendonStressingInterval(const CGirderKey& girderKey)
{
   VALIDATE(BRIDGE);

   return m_IntervalManager.GetLastTendonStressingInterval(girderKey);
}

IntervalIndexType CBridgeAgentImp::GetStressTendonInterval(const CGirderKey& girderKey,DuctIndexType ductIdx)
{
   VALIDATE(BRIDGE);
   ATLASSERT(ductIdx != INVALID_INDEX);

   // Convert the actual number of ducts to the number of raw input ducts.
   // There is one duct per web. If there is multiple webs, such as with U-Beams,
   // ducts 0 and 1, 2 and 3, 4 and 5, etc. are stressed together. The input for
   // the pairs of ducts is stored as index = 0 for duct 0 and 1, index = 1 for duct 2 and 3, etc.
   WebIndexType nWebs = GetWebCount(girderKey);
   ductIdx /= nWebs;

   return m_IntervalManager.GetStressTendonInterval(girderKey,ductIdx);
}

bool CBridgeAgentImp::IsTendonStressingInterval(const CGirderKey& girderKey,IntervalIndexType intervalIdx)
{
   VALIDATE(BRIDGE);
   ATLASSERT(intervalIdx != INVALID_INDEX);

   DuctIndexType nDucts = GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      IntervalIndexType stressingIntervalIdx = GetStressTendonInterval(girderKey,ductIdx);
      if ( stressingIntervalIdx == intervalIdx )
      {
         return true;
      }
   }

   return false;
}

IntervalIndexType CBridgeAgentImp::GetTemporarySupportErectionInterval(SupportIndexType tsIdx)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetTemporarySupportErectionInterval(tsIdx);
}

IntervalIndexType CBridgeAgentImp::GetTemporarySupportRemovalInterval(SupportIndexType tsIdx)
{
   VALIDATE(BRIDGE);
   return m_IntervalManager.GetTemporarySupportRemovalInterval(tsIdx);
}

std::vector<IntervalIndexType> CBridgeAgentImp::GetTemporarySupportRemovalIntervals(GroupIndexType groupIdx)
{
   ATLASSERT(groupIdx != ALL_GROUPS);
   std::vector<IntervalIndexType> tsrIntervals;

   // Get the intervals when temporary supports are removed for this group
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(groupIdx);
   const CSpanData2* pSpan        = pGroup->GetPier(pgsTypes::metStart)->GetNextSpan();
   const CSpanData2* pEndSpan     = pGroup->GetPier(pgsTypes::metEnd)->GetNextSpan();

   while ( pSpan != pEndSpan )
   {
      std::vector<const CTemporarySupportData*> vTS( pSpan->GetTemporarySupports() );
      std::vector<const CTemporarySupportData*>::iterator tsIter(vTS.begin());
      std::vector<const CTemporarySupportData*>::iterator tsIterEnd(vTS.end());
      for ( ; tsIter != tsIterEnd; tsIter++ )
      {
         const CTemporarySupportData* pTS = *tsIter;
         SupportIndexType tsIdx = pTS->GetIndex();
         ATLASSERT(tsIdx != INVALID_INDEX);
         
         IntervalIndexType intervalIdx = GetTemporarySupportRemovalInterval(tsIdx);
         ATLASSERT(intervalIdx != INVALID_INDEX);

         tsrIntervals.push_back( intervalIdx );
      }

      pSpan = pSpan->GetNextPier()->GetNextSpan();
   }

   // sort and remove any duplicates
   std::sort(tsrIntervals.begin(),tsrIntervals.end());
   tsrIntervals.erase( std::unique(tsrIntervals.begin(),tsrIntervals.end()), tsrIntervals.end() );

   return tsrIntervals;
}

std::vector<IntervalIndexType> CBridgeAgentImp::GetUserDefinedLoadIntervals(const CSpanKey& spanKey)
{
   std::vector<IntervalIndexType> vIntervals;

   SpanIndexType nSpans = GetSpanCount();
   SpanIndexType startSpanIdx = (spanKey.spanIndex == ALL_SPANS ? 0 : spanKey.spanIndex);
   SpanIndexType endSpanIdx   = (spanKey.spanIndex == ALL_SPANS ? nSpans-1 : startSpanIdx);

   IntervalIndexType nIntervals = GetIntervalCount();

   // loads are defined by span, so loop over all the spans in the range
   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      CSpanKey thisSpanKey(spanIdx,spanKey.girderIndex);

      GirderIndexType nGirders = GetGirderCountBySpan(spanIdx);
      thisSpanKey.girderIndex = Min(spanKey.girderIndex,nGirders-1);

      for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
      {
         if ( DoUserLoadsExist(thisSpanKey,intervalIdx) )
         {
            // user defined loads are defined in this interval
            vIntervals.push_back(intervalIdx);
         }
      } // interval loop
   } // span loop

   return vIntervals;
}

std::vector<IntervalIndexType> CBridgeAgentImp::GetUserDefinedLoadIntervals(const CSpanKey& spanKey,pgsTypes::ProductForceType pfType)
{
   std::vector<IntervalIndexType> vIntervals;

   SpanIndexType nSpans = GetSpanCount();
   SpanIndexType startSpanIdx = (spanKey.spanIndex == ALL_SPANS ? 0 : spanKey.spanIndex);
   SpanIndexType endSpanIdx   = (spanKey.spanIndex == ALL_SPANS ? nSpans-1 : startSpanIdx);

   IntervalIndexType nIntervals = GetIntervalCount();

   ATLASSERT(pfType == pgsTypes::pftUserDC || pfType == pgsTypes::pftUserDW);
   IUserDefinedLoads::UserDefinedLoadCase loadCase = (pfType == pgsTypes::pftUserDC ? IUserDefinedLoads::userDC : IUserDefinedLoads::userDW);

   // loads are defined by span, so loop over all the spans in the range
   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      CSpanKey thisSpanKey(spanIdx,spanKey.girderIndex);

      GirderIndexType nGirders = GetGirderCountBySpan(spanIdx);
      thisSpanKey.girderIndex = Min(spanKey.girderIndex,nGirders-1);

      for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
      {
         if ( DoUserLoadsExist(thisSpanKey,intervalIdx,loadCase) )
         {
            // user defined loads are defined in this interval
            vIntervals.push_back(intervalIdx);
         }
      } // interval loop
   } // span loop

   return vIntervals;
}

std::vector<IntervalIndexType> CBridgeAgentImp::GetSpecCheckIntervals(const CGirderKey& girderKey)
{
   std::vector<IntervalIndexType> vIntervals;

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   GET_IFACE(ILibrary,       pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   // Segment erection intervals only need to be spec checked if there are piers or temporary supports
   // that cause negative moments in the segment. This occurs when the segment is continuous over
   // these support types. Erecting the segment causes a change in loading condition compared to the
   // simple span loading condition before erection.

   // first check  piers
   GroupIndexType nGroups = GetGirderGroupCount();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx);
   bool bIsContinuousOverSupport = false;
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      PierIndexType startPierIdx, endPierIdx;
      GetGirderGroupPiers(grpIdx,&startPierIdx,&endPierIdx);
      for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
      {
         if ( IsInteriorPier(pierIdx) )
         {
            const CPierData2* pPier = pIBridgeDesc->GetPier(pierIdx);
            pgsTypes::PierSegmentConnectionType cType = pPier->GetSegmentConnectionType();
            if ( cType == pgsTypes::psctContinuousSegment || cType == pgsTypes::psctIntegralSegment )
            {
               bIsContinuousOverSupport = true;
               break;
            }
         }

         if ( bIsContinuousOverSupport )
         {
            break;
         }
      }

      // if not continuous over piers, check temporary supports
      if ( !bIsContinuousOverSupport )
      {
         SpanIndexType startSpanIdx, endSpanIdx;
         GetGirderGroupSpans(grpIdx,&startSpanIdx,&endSpanIdx);
         for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
         {
            const CSpanData2* pSpan = pIBridgeDesc->GetSpan(spanIdx);
            std::vector<const CTemporarySupportData*> vTS(pSpan->GetTemporarySupports());
            std::vector<const CTemporarySupportData*>::iterator tsIter(vTS.begin());
            std::vector<const CTemporarySupportData*>::iterator tsIterEnd(vTS.end());
            for ( ; tsIter != tsIterEnd; tsIter++ )
            {
               const CTemporarySupportData* pTS = *tsIter;
               if ( pTS->GetConnectionType() == pgsTypes::tsctContinuousSegment )
               {
                  bIsContinuousOverSupport = true;
                  break;
               }
            }

            if ( bIsContinuousOverSupport )
            {
               break;
            }
         }
      }

      // Spec check intervals for basic segment events
      GirderIndexType nGirders = GetGirderCount(grpIdx);
      GirderIndexType startGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex);
      GirderIndexType endGirderIdx   = (girderKey.girderIndex == ALL_GIRDERS ? nGirders-1 : startGirderIdx);
      for ( GirderIndexType gdrIdx = startGirderIdx; gdrIdx <= endGirderIdx; gdrIdx++ )
      {
         CGirderKey thisGirderKey(grpIdx,gdrIdx);
         SegmentIndexType nSegments = GetSegmentCount(thisGirderKey);
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey thisSegmentKey(thisGirderKey,segIdx);

            vIntervals.push_back(GetPrestressReleaseInterval(thisSegmentKey));

            // Lifting is a different animal. Don't include it here
            //vIntervals.push_back(GetLiftSegmentInterval(thisSegmentKey));

            // If the segment isn't stored at its ends (which would be the same as at release)
            // spec check it during storage. This is only for spliced girders. We've never
            // do this for conventional pre-tensioned girders. However, for spliced girders
            // a stress check is required every time the loading condition changes. Change
            // in support location is a change in loading condition. Assume spliced girder
            // if there is more than one segment.
            if ( 1 < nSegments )
            {
               const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(thisSegmentKey);
               if ( !IsZero(pSegment->HandlingData.LeftStoragePoint) || !IsZero(pSegment->HandlingData.RightStoragePoint) )
               {
                  vIntervals.push_back(GetStorageInterval(thisSegmentKey));
               }
            }


            // Hauling is a different animal. Don't include it here
            //vIntervals.push_back(GetHaulSegmentInterval(thisSegmentKey));

            // Only spec check at segment erection if there is a support mid-segment that causes negative moments in the segment
            if ( bIsContinuousOverSupport )
            {
               vIntervals.push_back(GetErectSegmentInterval(thisSegmentKey));
            }

            // Only check temporary strand interval if there are temporary strands
            if ( 0 < GetStrandCount(thisSegmentKey,pgsTypes::Temporary) && pSpecEntry->CheckTemporaryStresses() )
            {
               vIntervals.push_back(GetTemporaryStrandRemovalInterval(thisSegmentKey));
            }
         } // next segment


         // Spec check whenever a tendon is stressed
         DuctIndexType nDucts = GetDuctCount(thisGirderKey);
         for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
         {
            vIntervals.push_back(GetStressTendonInterval(thisGirderKey,ductIdx));
         }
      } // next girder
   } // next group

   // Important loading intervals for the full bridge
   if ( pSpecEntry->CheckTemporaryStresses() )
   {
      vIntervals.push_back(GetCastDeckInterval());
   }

   IntervalIndexType overlayIntervalIdx = GetOverlayInterval();
   if (overlayIntervalIdx != INVALID_INDEX )
   {
      vIntervals.push_back(overlayIntervalIdx);
   }

   vIntervals.push_back(GetInstallRailingSystemInterval());

   // Need to spec check whenever a user defined load is applied
   SpanIndexType startSpanIdx, endSpanIdx;
   GetGirderGroupSpans(girderKey.groupIndex,&startSpanIdx,&endSpanIdx);
   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      CSpanKey spanKey(spanIdx,girderKey.girderIndex);
      std::vector<IntervalIndexType> vUserLoadIntervals(GetUserDefinedLoadIntervals(spanKey));
      vIntervals.insert(vIntervals.end(),vUserLoadIntervals.begin(),vUserLoadIntervals.end());
   }

   // Always spec check the last interval (this will cover live load)
   vIntervals.push_back(GetIntervalCount()-1); // last interval

   // sort and remove any duplicates
   std::sort(vIntervals.begin(),vIntervals.end());
   vIntervals.erase( std::unique(vIntervals.begin(),vIntervals.end()), vIntervals.end() );

   return vIntervals;
}

////////////////////////////////////
Float64 CBridgeAgentImp::ConvertDuctOffsetToDuctElevation(const CGirderKey& girderKey,Float64 Xg,Float64 offset,CDuctGeometry::OffsetType offsetType)
{
   // Returns the elevation of the duct relative to the top of the girder
   if ( offsetType == CDuctGeometry::TopGirder )
   {
      return -offset;
   }
   else
   {
      ATLASSERT(offsetType == pgsTypes::BottomGirder);
      pgsPointOfInterest poi = ConvertGirderCoordinateToPoi(girderKey,Xg);
      Float64 Hg = GetHeight(poi);
      return offset - Hg;
   }
}

mathCompositeFunction2d CBridgeAgentImp::CreateDuctCenterline(const CGirderKey& girderKey,const CParabolicDuctGeometry& geometry)
{
   mathCompositeFunction2d fnCenterline;

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
   PierIndexType startPierIdx = pGroup->GetPierIndex(pgsTypes::metStart);
   PierIndexType endPierIdx   = pGroup->GetPierIndex(pgsTypes::metEnd);
   SpanIndexType startSpanIdx = (SpanIndexType)startPierIdx;

   Float64 x1 = 0; // x is measured from the start face of the girder

   //
   // Start to first low point
   //
   SpanIndexType nSpans = geometry.GetSpanCount();

   // start point
   Float64 dist,offset;
   CDuctGeometry::OffsetType offsetType;
   geometry.GetStartPoint(&dist,&offset,&offsetType);

   x1 += dist;
   Float64 y1 = ConvertDuctOffsetToDuctElevation(girderKey,x1,offset,offsetType);

   // low point
   geometry.GetLowPoint(startSpanIdx,&dist,&offset,&offsetType);
   if ( dist < 0 ) // fraction of the span length
   {
      dist *= -GetSpanLength(startSpanIdx,girderKey.girderIndex);
   }

   Float64 x2 = x1 + dist;
   Float64 y2 = ConvertDuctOffsetToDuctElevation(girderKey,x2,offset,offsetType);

   mathPolynomial2d leftParabola = GenerateParabola2(x1,y1,x2,y2,0.0);
   fnCenterline.AddFunction(x1,x2,leftParabola);

   x1 = x2; // start next group of parabolas at the low point
   y1 = y2;

   //
   // Low Point to High Point to Low Point
   //
   Float64 x3,y3;
   mathPolynomial2d rightParabola;
   Float64 startStation = GetPierStation(startPierIdx);
   Float64 Ls = 0;
   for ( PierIndexType pierIdx = startPierIdx+1; pierIdx < endPierIdx; pierIdx++ )
   {
      SpanIndexType prevSpanIdx = SpanIndexType(pierIdx-1);
      SpanIndexType nextSpanIdx = prevSpanIdx+1;

      Float64 distLeftIP, highOffset, distRightIP;
      CDuctGeometry::OffsetType highOffsetType;
      geometry.GetHighPoint(pierIdx,&distLeftIP,&highOffset,&highOffsetType,&distRightIP);

      // low to high point
      Ls += GetSpanLength(prevSpanIdx,girderKey.girderIndex);
      x3 = Ls;
      y3 = ConvertDuctOffsetToDuctElevation(girderKey,x3,highOffset,highOffsetType);

      if ( distLeftIP < 0 ) // fraction of distance between low and high point
      {
         distLeftIP *= -(x3-x1);
      }

      x2 = x3 - distLeftIP; // inflection point measured from high point

      GenerateReverseParabolas(x1,y1,x2,x3,y3,&leftParabola,&rightParabola);
      fnCenterline.AddFunction(x1,x2,leftParabola);
      fnCenterline.AddFunction(x2,x3,rightParabola);

      // high to low point
      geometry.GetLowPoint(pierIdx,&dist,&offset,&offsetType);
      x1 = x3; 
      y1 = y3;

      if ( dist < 0 ) // fraction of span length
      {
         dist *= -GetSpanLength(nextSpanIdx,girderKey.girderIndex);
      }

      if ( nextSpanIdx == nSpans-1 )
      {
         // low point in last span is measured from the right end
         // change it around to be measured from the left end
         dist = GetSpanLength(nextSpanIdx,girderKey.girderIndex) - dist;
      }

      x3 = x1 + dist; // low point, measured from previous high point
      y3 = ConvertDuctOffsetToDuctElevation(girderKey,x3,offset,offsetType);

      if ( distRightIP < 0 ) // fraction of distance between high and low point
      {
         distRightIP *= -(x3 - x1);
      }

      x2 = x1 + distRightIP; // inflection point measured from high point

      GenerateReverseParabolas(x1,y1,x2,x3,y3,&leftParabola,&rightParabola);
      fnCenterline.AddFunction(x1,x2,leftParabola);
      fnCenterline.AddFunction(x2,x3,rightParabola);

      x1 = x3;
      y1 = y3;
   }

   //
   // last low point to end
   //
   geometry.GetEndPoint(&dist,&offset,&offsetType);
   if ( dist < 0 ) // fraction of last span length
   {
      dist *= -GetSpanLength(startSpanIdx + nSpans-1,girderKey.girderIndex);
   }

   Float64 girderLength = GetGirderLength(girderKey);
   x2 = girderLength;
   y2 = ConvertDuctOffsetToDuctElevation(girderKey,x1,offset,offsetType);
   rightParabola = GenerateParabola1(x1,y1,x2,y2,0.0);
   fnCenterline.AddFunction(x1,x2,rightParabola);

   return fnCenterline;
}

void CBridgeAgentImp::CreateDuctCenterline(const CGirderKey& girderKey,const CParabolicDuctGeometry& geometry,IPoint2dCollection** ppPoints)
{
   mathCompositeFunction2d fnCenterline = CreateDuctCenterline(girderKey,geometry);
   IndexType nFunctions = fnCenterline.GetFunctionCount();

   CComPtr<IPoint2dCollection> points;
   points.CoCreateInstance(CLSID_Point2dCollection);
   points.CopyTo(ppPoints);

   for ( IndexType fnIdx = 0; fnIdx < nFunctions; fnIdx++ )
   {
      const mathFunction2d* pFN;
      Float64 xMin,xMax;
      fnCenterline.GetFunction(fnIdx,&pFN,&xMin,&xMax);
      for ( int i = 0; i < 11; i++ )
      {
         Float64 x = xMin + i*(xMax-xMin)/10;
         Float64 y = fnCenterline.Evaluate(x);

         CComPtr<IPoint2d> p;
         p.CoCreateInstance(CLSID_Point2d);
         p->Move(x,y);
         points->Add(p);
      }
   }
}

void CBridgeAgentImp::CreateDuctCenterline(const CGirderKey& girderKey,const COffsetDuctGeometry& geometry,IPoint2dCollection** ppPoints)
{
   ATLASSERT(*ppPoints != NULL); // should contain the points from the duct that this duct offsets from

   CollectionIndexType nPoints;
   (*ppPoints)->get_Count(&nPoints);
   for ( CollectionIndexType idx = 0; idx < nPoints; idx++ )
   {
      CComPtr<IPoint2d> point;
      (*ppPoints)->get_Item(idx,&point);

      Float64 x,y;
      point->Location(&x,&y);
      Float64 offset = geometry.GetOffset(x);
      y += offset;

      point->put_Y(y);
   }
}

////////////////////////////////////////////////////////////////////////
// IBridgeDescriptionEventSink
//
HRESULT CBridgeAgentImp::OnBridgeChanged(CBridgeChangedHint* pHint)
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

HRESULT CBridgeAgentImp::OnGirderChanged(const CGirderKey& girderKey,Uint32 lHint)
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

////////////////////////////////////////////////////////////////////////
// ILossParametersEventSink
HRESULT CBridgeAgentImp::OnLossParametersChanged()
{
   // When the loss parameters are changed, we have to invalidate
   // the transformed section properties. For time-step analysis,
   // the transformed section properties are computed with age
   // adjusted modulus which depend on creep. If creep effects
   // are toggled between enabled and disabled, the section properties
   // have to be updated. Since the event model doesn't say which
   // loss parameter changed, we just have to blast the transformed
   // section properties.
   InvalidateSectionProperties(pgsTypes::sptTransformed);
   InvalidateSectionProperties(pgsTypes::sptTransformedNoncomposite);
   return S_OK;
}

////////////////////////////////////////////////////////////////////////
HRESULT CBridgeAgentImp::OnAnalysisTypeChanged()
{
   // Remove critical section POIs.
   // They will move when the analysis type changes
   m_CriticalSectionState[0].clear();
   m_CriticalSectionState[1].clear();

   m_pPoiMgr->RemovePointsOfInterest(POI_CRITSECTSHEAR1);
   m_pPoiMgr->RemovePointsOfInterest(POI_CRITSECTSHEAR2);
   return S_OK;
}

void CBridgeAgentImp::GetAlignment(IAlignment** ppAlignment)
{
   VALIDATE( COGO_MODEL );

   CComPtr<IAlignmentCollection> alignments;
   m_CogoModel->get_Alignments(&alignments);

   HRESULT hr = alignments->get_Item(g_AlignmentID,ppAlignment);
   ATLASSERT(SUCCEEDED(hr));
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
   {
      m_Bridge->get_LeftBarrier(&barrier);
   }
   else
   {
      m_Bridge->get_RightBarrier(&barrier);
   }

   if ( barrier == NULL )
   {
      *props = 0;
      return;
   }

   CComPtr<IShape> shape;
   barrier->get_Shape(&shape);

   shape->get_ShapeProperties(props);
}

CBridgeAgentImp::SectProp CBridgeAgentImp::GetSectionProperties(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::SectionPropertyType sectPropType)
{
   VALIDATE(BRIDGE);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   USES_CONVERSION;

   // Find properties and return... if not found, compute them now
   PoiIntervalKey poiKey(poi,intervalIdx);
   SectPropContainer::iterator found( m_pSectProps[sectPropType]->find(poiKey) );
   if ( found != m_pSectProps[sectPropType]->end() )
   {
      return (*found).second;
   }

   //
   //   ... not found ... compute, store, return stored value
   //
   SectProp props; 

   IntervalIndexType releaseIntervalIdx       = GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType compositeDeckIntervalIdx = GetCompositeDeckInterval();

   GET_IFACE(ILossParameters,pLossParams);
   bool bIsTimeStepAnalysis = (pLossParams->GetLossMethod() == pgsTypes::TIME_STEP ? true : false);


   // figure out where the poi is located and if the concrete is cured enought to make the section act as a structural member
   CClosureKey closureKey;
   bool bIsInClosureJoint          = IsInClosureJoint(poi,&closureKey);
   bool bIsOnSegment               = IsOnSegment(poi);
   bool bIsInIntermediateDiaphragm = IsInIntermediateDiaphragm(poi);

   bool bIsSection = true;
   if ( 
        (bIsInClosureJoint && intervalIdx < GetCompositeClosureJointInterval(closureKey)) // in closure and closure concrete isn't cured yet
        ||                                                             // OR
        (bIsOnSegment && intervalIdx < releaseIntervalIdx ) // on segment and segment concrete isn't cured yet
        ||                                                             // OR
        (bIsInIntermediateDiaphragm && intervalIdx < compositeDeckIntervalIdx) // in intermediate diaphragm and concrete isn't cured yet
      )
   {
      bIsSection = false; // this is not a structural section...
   }

   // if net deck properties are requested and the interval is before the deck is made composite
   // then the deck is not yet structural and its section properties are taken to be zero
   if ( sectPropType == pgsTypes::sptNetDeck && intervalIdx < compositeDeckIntervalIdx )
   {
      bIsSection = false;
   }

   // if we can compute properties do it... otherwise the properties are taken to be zero
   if ( bIsSection )
   {
      Float64 Xs = poi.GetDistFromStart();

      GET_IFACE(ILibrary,       pLib);
      GET_IFACE(ISpecification, pSpec);
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

      if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth )
      {
         CComQIPtr<IPGSuperEffectiveFlangeWidthTool> eff_tool(m_EffFlangeWidthTool);
         ATLASSERT(eff_tool);
         eff_tool->put_UseTributaryWidth(VARIANT_TRUE);
      }

      GirderIDType leftGdrID,gdrID,rightGdrID;
      GetAdjacentSuperstructureMemberIDs(segmentKey,&leftGdrID,&gdrID,&rightGdrID);
      if ( sectPropType == pgsTypes::sptGrossNoncomposite       || 
           sectPropType == pgsTypes::sptGross                   || 
           sectPropType == pgsTypes::sptTransformedNoncomposite || 
           sectPropType == pgsTypes::sptTransformed             || 
           sectPropType == pgsTypes::sptNetGirder )
      {
         CComPtr<ISection> section;
         HRESULT hr = GetSection(intervalIdx,poi,sectPropType,&section);
         ATLASSERT(SUCCEEDED(hr));

         // get elastic properties of section
         CComPtr<IElasticProperties> eprop;
         section->get_ElasticProperties(&eprop);

         // transform section properties
         // this transforms all materials into equivalent girder concrete
         // we always do this because the deck must be transformed, even for net properties
         Float64 Egdr;
         // NOTE: we are checking if this is a time-step analysis and calling the age adjusted ec method
         // We don't actually have to do this, the age adjusted ec returns the same as the regular ec
         // methods for non-time-step methods. It is just more clear that what ec we want by calling
         // each method explicitly
         if ( bIsOnSegment )
         {
            Egdr = (bIsTimeStepAnalysis ? GetSegmentAgeAdjustedEc(segmentKey,intervalIdx) : GetSegmentEc(segmentKey,intervalIdx));
         }
         else if ( bIsInClosureJoint )
         {
            Egdr = (bIsTimeStepAnalysis ? GetClosureJointAgeAdjustedEc(closureKey,intervalIdx) : GetClosureJointEc(closureKey,intervalIdx));
         }
         else if ( bIsInIntermediateDiaphragm )
         {
            Egdr = (bIsTimeStepAnalysis ? GetDeckAgeAdjustedEc(intervalIdx) : GetDeckEc(intervalIdx));
         }

         CComPtr<IShapeProperties> shapeprops;
         eprop->TransformProperties(Egdr,&shapeprops);

         props.ElasticProps = eprop;
         props.ShapeProps   = shapeprops;

         Float64 Ag;
         shapeprops->get_Area(&Ag);

         LOG(_T("Interval = ") << intervalIdx << _T(" Group = ") << LABEL_SPAN(segmentKey.groupIndex) << _T(" Girder = ") << LABEL_GIRDER(segmentKey.girderIndex) << _T(" Segment = ") << LABEL_SEGMENT(segmentKey.segmentIndex) << _T(" x = ") << ::ConvertFromSysUnits(poi.GetDistFromStart(),unitMeasure::Feet) << _T(" ft") << _T(" Ag = ") << ::ConvertFromSysUnits(Ag,unitMeasure::Inch2) << _T(" in2") << _T(" Eg = ") << ::ConvertFromSysUnits(Egdr,unitMeasure::KSI) << _T(" KSI"));

         // Assuming section is a Composite section and beam is exactly the first piece
         CComQIPtr<ICompositeSectionEx> compositeSection(section);
         ATLASSERT(compositeSection != NULL);
         CComPtr<ICompositeSectionItemEx> sectionItem;
         compositeSection->get_Item(0,&sectionItem);
         CComPtr<IShape> shape;
         sectionItem->get_Shape(&shape);

         shape->get_Perimeter(&props.Perimeter);

         Float64 Yb;
         shapeprops->get_Ybottom(&Yb);

         // Q slab (from procedure in WBFL Sections library documentation)
         CComQIPtr<IXYPosition> position(shape);
         CComPtr<IPoint2d> top_center;
         position->get_LocatorPoint(lpTopCenter,&top_center);

         // Ytop Girder
         CComPtr<IShapeProperties> beamprops;
         shape->get_ShapeProperties(&beamprops);
         props.YtopGirder = ComputeYtopGirder(shapeprops,beamprops);

         // Create clipping line through beam/slab interface
         CComPtr<ILine2d> line;
         line.CoCreateInstance(CLSID_Line2d);
         CComPtr<IPoint2d> p;
         CComPtr<IVector2d> v;
         line->GetExplicit(&p,&v);
         Float64 topCenterX,topCenterY;
         top_center->Location(&topCenterX,&topCenterY); // in Bridge Section Coordinates
         p->Move(topCenterX,topCenterY);
         v->put_Direction(M_PI); // direct line to the left so the beam is clipped out
         v->put_Magnitude(1.0);
         line->SetExplicit(p,v);

         CComPtr<ISection> slabSection;
         section->ClipWithLine(line,&slabSection);

         CComPtr<IElasticProperties> epSlab;
         slabSection->get_ElasticProperties(&epSlab);

         Float64 EAslab;
         epSlab->get_EA(&EAslab);
         Float64 Aslab = EAslab / Egdr; // Transformed to equivalent girder material

         CComPtr<IPoint2d> cg_section;
         shapeprops->get_Centroid(&cg_section);

         CComPtr<IPoint2d> cg_slab;
         epSlab->get_Centroid(&cg_slab);

         Float64 Yg, Ys;
         cg_slab->get_Y(&Ys);
         cg_section->get_Y(&Yg);

         Float64 Qslab = Aslab*(Ys - Yg);
         props.Qslab = Qslab;

         // Area on bottom half of composite section for LRFD Fig 5.8.3.4.2-3
         Float64 half_depth_elevation = GetHalfElevation(poi); // measured down from top of non-composite girder
                                                               // Y=0.0 at top of girder

         half_depth_elevation += topCenterY;

         CComPtr<IPoint2d> p1, p2;
         p1.CoCreateInstance(CLSID_Point2d);
         p2.CoCreateInstance(CLSID_Point2d);

         p1->Move(-99999,half_depth_elevation);
         p2->Move( 99999,half_depth_elevation);

         line->ThroughPoints(p1,p2);

         CComPtr<ISection> clipped_section;
         section->ClipWithLine(line,&clipped_section);

         CComPtr<IElasticProperties> bottomHalfElasticProperties;
         clipped_section->get_ElasticProperties(&bottomHalfElasticProperties);

         CComPtr<IShapeProperties> bottomHalfShapeProperties;
         bottomHalfElasticProperties->TransformProperties(Egdr,&bottomHalfShapeProperties);

         Float64 area;
         bottomHalfShapeProperties->get_Area(&area);
         props.AcBottomHalf = area;

         // Area on top half of composite section for LRFD Fig. 5.8.3.4.2-3
         CComPtr<IShapeProperties> fullShapeProperties;
         props.ElasticProps->TransformProperties(Egdr,&fullShapeProperties);
         fullShapeProperties->get_Area(&area);
         props.AcTopHalf = area - props.AcBottomHalf; // top + bottom = full  ==> top = full - botton
      }
      else if ( sectPropType == pgsTypes::sptNetDeck )
      {
         CComPtr<ISection> deckSection;
         HRESULT hr = GetSection(intervalIdx,poi,sectPropType,&deckSection);
         ATLASSERT(SUCCEEDED(hr));

         // get elastic properties of section
         CComPtr<IElasticProperties> eprop;
         deckSection->get_ElasticProperties(&eprop);

         // transform section properties
         // this transforms all materials into equivalent girder concrete
         // we always do this because the deck must be transformed, even for net properties
         Float64 Edeck;
         if ( bIsTimeStepAnalysis )
         {
            Edeck = GetDeckAgeAdjustedEc(intervalIdx);
         }
         else
         {
            Edeck = GetDeckEc(intervalIdx);
         }
         CComPtr<IShapeProperties> shapeprops;
         eprop->TransformProperties(Edeck,&shapeprops);

         props.ElasticProps = eprop;
         props.ShapeProps   = shapeprops;
      }
      else
      {
         ATLASSERT(false); // should never get here
      }
   }
   else
   {
      // prestressing has not been released into segment yet so essentially we don't have a segment 
      // -OR-
      // we are at a closure joint and it hasn't reached strength enough to be effective
      // create default properties (all zero values)
      props.ElasticProps.CoCreateInstance(CLSID_ElasticProperties);
      props.ElasticProps->TransformProperties(1.0,&props.ShapeProps);
   }


   // don't store if not a real POI
   if ( poi.GetID() == INVALID_ID )
   {
      return props;
   }

   // Store the properties
   std::pair<SectPropContainer::iterator,bool> result( m_pSectProps[sectPropType]->insert(std::make_pair(poiKey,props)) );
   ATLASSERT(result.second == true);
   found = result.first; 
   ATLASSERT(found == m_pSectProps[sectPropType]->find(poiKey));
   ATLASSERT(found != m_pSectProps[sectPropType]->end());
   return (*found).second;
}

HRESULT CBridgeAgentImp::GetSection(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::SectionPropertyType sectPropType,ISection** ppSection)
{
   VALIDATE(BRIDGE);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   Float64 Xs = poi.GetDistFromStart();
   GirderIDType leftGdrID,gdrID,rightGdrID;
   GetAdjacentSuperstructureMemberIDs(segmentKey,&leftGdrID,&gdrID,&rightGdrID);
   if ( sectPropType == pgsTypes::sptGrossNoncomposite       || 
        sectPropType == pgsTypes::sptGross                   || 
        sectPropType == pgsTypes::sptTransformedNoncomposite || 
        sectPropType == pgsTypes::sptTransformed             || 
        sectPropType == pgsTypes::sptNetGirder )
   {
      // use tool to create section
      CComPtr<ISection> section;
      HRESULT hr = m_SectCutTool->CreateGirderSectionBySegment(m_Bridge,gdrID,segmentKey.segmentIndex,Xs,leftGdrID,rightGdrID,intervalIdx,(SectionPropertyMethod)sectPropType,&section);
      ATLASSERT(SUCCEEDED(hr));

      return section.CopyTo(ppSection);
   }
   else if ( sectPropType == pgsTypes::sptNetDeck )
   {
      CComPtr<ISection> deckSection;
      HRESULT hr = m_SectCutTool->CreateNetDeckSection(m_Bridge,gdrID,segmentKey.segmentIndex,Xs,leftGdrID,rightGdrID,intervalIdx,&deckSection);
      ATLASSERT(SUCCEEDED(hr));

      return deckSection.CopyTo(ppSection);
   }

   ATLASSERT(false); // should never get here
   return E_FAIL;
}

Float64 CBridgeAgentImp::ComputeY(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,IShapeProperties* sprops)
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   Float64 Xpoi = poi.GetDistFromStart();
   CComPtr<ISuperstructureMemberSegment> segment;
   GetSegment(segmentKey,&segment);
   CComPtr<IShape> shape;
   segment->get_PrimaryShape(Xpoi,&shape);
   CComPtr<IShapeProperties> beamprops;
   shape->get_ShapeProperties(&beamprops);

   Float64 Y;
   switch (location)
   {
   case pgsTypes::TopGirder:
   case pgsTypes::TopDeck:
   case pgsTypes::BottomDeck:
      {
         IntervalIndexType compositeDeckIntervalIdx = GetCompositeDeckInterval();
         if ( location == pgsTypes::TopDeck && compositeDeckIntervalIdx <= intervalIdx && IsCompositeDeck() )
         {
            // top of composite deck that has been installed
            sprops->get_Ytop(&Y);
         }
         else
         {
            // top girder, bottom deck, (and top deck if deck not composite or installed yet)
            // are all at the same location
            Y = ComputeYtopGirder(sprops,beamprops);
         }
      }
      break;

   case pgsTypes::BottomGirder:
      sprops->get_Ybottom(&Y);
      break;

   default:
      ATLASSERT(false);
   }

   return Y;
}

Float64 CBridgeAgentImp::ComputeYtopGirder(IShapeProperties* compositeProps,IShapeProperties* beamProps)
{
   Float64 Ybc;
   compositeProps->get_Ybottom(&Ybc);

   Float64 Yb, Yt;
   beamProps->get_Ytop(&Yt);
   beamProps->get_Ybottom(&Yb);
   Float64 Hg = Yb + Yt;

   Float64 YtopGirder = Hg - Ybc;

   return YtopGirder;
}

Float64 CBridgeAgentImp::GetHalfElevation(const pgsPointOfInterest& poi)
{
   Float64 deck_thickness = GetStructuralSlabDepth(poi);
   Float64 girder_depth = GetHeight(poi);
   // cut line is at 1/2 of the compsite section depth
   Float64 half_depth = (deck_thickness + girder_depth)/2.;
   // half depth elevation is measured down from the top of the non-composite girder
   // this puts it in Girder Section Coordinates
   return -(half_depth - deck_thickness);
}

HRESULT CBridgeAgentImp::GetSlabOverhangs(Float64 Xb,Float64* pLeft,Float64* pRight)
{
   Float64 station = ConvertBridgeLineToRouteCoordinate(Xb);

   SpanIndexType spanIdx = GetSpanIndex(Xb);
   if ( spanIdx == INVALID_INDEX )
   {
      if ( Xb < 0 )
      {
         spanIdx = 0;
      }
      else
      {
         spanIdx = GetSpanCount()-1;
      }
   }

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   GroupIndexType grpIdx = pGroup->GetIndex();
   GirderIndexType nGirders = pGroup->GetGirderCount();

   GirderIDType leftGdrID  = ::GetSuperstructureMemberID(grpIdx,0);
   GirderIDType rightGdrID = ::GetSuperstructureMemberID(grpIdx,nGirders-1);

   m_BridgeGeometryTool->DeckOverhang(m_Bridge,station,leftGdrID, NULL,qcbLeft, pLeft);
   m_BridgeGeometryTool->DeckOverhang(m_Bridge,station,rightGdrID,NULL,qcbRight,pRight);

   return S_OK;
}

HRESULT CBridgeAgentImp::GetGirderSection(const pgsPointOfInterest& poi,pgsTypes::SectionCoordinateType csType,IGirderSection** gdrSection)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CComPtr<ISuperstructureMemberSegment> segment;
   GetSegment(segmentKey,&segment);

   CComPtr<IShape> girder_shape;
   Float64 Xs = poi.GetDistFromStart();
   Float64 Ls = GetSegmentLength(segmentKey);
   Xs = ::ForceIntoRange(0.0,Xs,Ls);
   segment->get_PrimaryShape(Xs,&girder_shape);

   if ( csType == pgsTypes::scGirder )
   {
      // Convert to girder section coordinates....
      // the bottom center point is located at (0,0)
      CComQIPtr<IXYPosition> position(girder_shape);

      CComPtr<IPoint2d> point;
      position->get_LocatorPoint(lpBottomCenter,&point);

      point->Move(0,0);

      position->put_LocatorPoint(lpBottomCenter,point);
   }


   CComQIPtr<IGirderSection> girder_section(girder_shape);

   ASSERT(girder_section != NULL);

   (*gdrSection) = girder_section;
   (*gdrSection)->AddRef();

   return S_OK;
}

HRESULT CBridgeAgentImp::GetSuperstructureMember(const CGirderKey& girderKey,ISuperstructureMember* *ssmbr)
{
   VALIDATE(BRIDGE);
   return ::GetSuperstructureMember(m_Bridge,girderKey,ssmbr);
}

HRESULT CBridgeAgentImp::GetSegment(const CSegmentKey& segmentKey,ISuperstructureMemberSegment** segment)
{
   VALIDATE(BRIDGE);
   return ::GetSegment(m_Bridge,segmentKey,segment);
}

HRESULT CBridgeAgentImp::GetGirder(const CSegmentKey& segmentKey,IPrecastGirder** girder)
{
   VALIDATE(BRIDGE);
   return ::GetGirder(m_Bridge,segmentKey,girder);
}

HRESULT CBridgeAgentImp::GetGirder(const pgsPointOfInterest& poi,IPrecastGirder** girder)
{
   return GetGirder(poi.GetSegmentKey(),girder);
}

Float64 CBridgeAgentImp::GetGrossSlabDepth()
{
   // Private method for getting gross slab depth
   // Gross slab depth is needed during validation of bridge model,
   // but public method causes validation. This causes a recusion problem
   
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
      ATLASSERT(deck == NULL); // no deck
      deck_thickness = 0;
   }

   return deck_thickness;
}

Float64 CBridgeAgentImp::GetCastDepth()
{
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

void CBridgeAgentImp::LayoutDeckRebar(const CDeckDescription2* pDeck,IBridgeDeck* deck)
{
   const CDeckRebarData& rebar_data = pDeck->DeckRebarData;

   CComPtr<IBridgeDeckRebarLayout> rebar_layout;
   rebar_layout.CoCreateInstance(CLSID_BridgeDeckRebarLayout);
   deck->putref_RebarLayout(rebar_layout);
   rebar_layout->putref_Bridge(m_Bridge);
   rebar_layout->putref_EffectiveFlangeWidthTool(m_EffFlangeWidthTool);

   Float64 deck_height;
   deck->get_GrossDepth(&deck_height);

   // Get the stage where the rebar is first introducted into the system.
   // Technically, the rebar is first introduced to the system when the deck
   // concrete is cast, however the rebar isn't performing any structural function
   // until the concrete is cured
   IntervalIndexType compositeDeckIntervalIdx = m_IntervalManager.GetCompositeDeckInterval();

   // Create a rebar factory. This does the work of creating rebar objects
   CComPtr<IRebarFactory> rebar_factory;
   rebar_factory.CoCreateInstance(CLSID_RebarFactory);

   // Rebar factory needs a unit server object for units conversion
   CComPtr<IUnitServer> unitServer;
   unitServer.CoCreateInstance(CLSID_UnitServer);
   HRESULT hr = ConfigureUnitServer(unitServer);
   ATLASSERT(SUCCEEDED(hr));

   CComPtr<IUnitConvert> unit_convert;
   unitServer->get_UnitConvert(&unit_convert);

   // First layout rebar that runs the full length of the deck
   CComPtr<IBridgeDeckRebarLayoutItem> deck_rebar_layout_item;
   deck_rebar_layout_item.CoCreateInstance(CLSID_BridgeDeckRebarLayoutItem);
   deck_rebar_layout_item->putref_Bridge(m_Bridge);
   rebar_layout->Add(deck_rebar_layout_item);

   // Top Mat - Bars
   if ( rebar_data.TopRebarSize != matRebar::bsNone )
   {
      // create the rebar object
      BarSize       matSize  = GetBarSize(rebar_data.TopRebarSize);
      MaterialSpec  matSpec  = GetRebarSpecification(rebar_data.TopRebarType);
      RebarGrade    matGrade = GetRebarGrade(rebar_data.TopRebarGrade);

      CComPtr<IRebar> rebar;
      rebar_factory->CreateRebar(matSpec,matGrade,matSize,unit_convert,compositeDeckIntervalIdx,&rebar);

      Float64 db;
      rebar->get_NominalDiameter(&db);

      // create the rebar pattern (definition of rebar in the cross section)
      CComPtr<IBridgeDeckRebarPattern> rebar_pattern;
      rebar_pattern.CoCreateInstance(CLSID_BridgeDeckRebarPattern);

      // Locate rebar from the bottom of the deck
      Float64 Y = deck_height - rebar_data.TopCover - db/2;

      // create the rebar pattern
      rebar_pattern->putref_Rebar(rebar);
      rebar_pattern->putref_RebarLayoutItem(deck_rebar_layout_item);
      rebar_pattern->put_Spacing( rebar_data.TopSpacing );
      rebar_pattern->put_Location( Y );
      rebar_pattern->put_IsLumped(VARIANT_FALSE);

      // add this pattern to the layout
      deck_rebar_layout_item->AddRebarPattern(rebar_pattern);
   }

   // Top Mat - Lump Sum
   if ( !IsZero(rebar_data.TopLumpSum) )
   {
      MaterialSpec  matSpec  = GetRebarSpecification(rebar_data.TopRebarType);
      RebarGrade    matGrade = GetRebarGrade(rebar_data.TopRebarGrade);

      // create a dummy #3 bar, then change the diameter and bar area to match
      // the lump sum bar
      CComPtr<IRebar> rebar;
      rebar_factory->CreateRebar(matSpec,matGrade,bs3,unit_convert,compositeDeckIntervalIdx,&rebar);
      rebar->put_NominalDiameter(0.0);
      rebar->put_NominalArea(rebar_data.TopLumpSum);

      Float64 Y = deck_height - rebar_data.TopCover;

      // create the rebar pattern (definition of rebar in the cross section)
      CComPtr<IBridgeDeckRebarPattern> rebar_pattern;
      rebar_pattern.CoCreateInstance(CLSID_BridgeDeckRebarPattern);

      // create the rebar pattern
      rebar_pattern->putref_Rebar(rebar);
      rebar_pattern->putref_RebarLayoutItem(deck_rebar_layout_item);
      rebar_pattern->put_Spacing( 0.0 );
      rebar_pattern->put_Location( Y );
      rebar_pattern->put_IsLumped(VARIANT_TRUE);

      // add this pattern to the layout
      deck_rebar_layout_item->AddRebarPattern(rebar_pattern);
   }

   // Bottom Mat - Rebar
   if ( rebar_data.BottomRebarSize != matRebar::bsNone )
   {
      // create the rebar object
      BarSize       matSize  = GetBarSize(rebar_data.BottomRebarSize);
      MaterialSpec  matSpec  = GetRebarSpecification(rebar_data.BottomRebarType);
      RebarGrade    matGrade = GetRebarGrade(rebar_data.BottomRebarGrade);

      CComPtr<IRebar> rebar;
      rebar_factory->CreateRebar(matSpec,matGrade,matSize,unit_convert,compositeDeckIntervalIdx,&rebar);

      Float64 db;
      rebar->get_NominalDiameter(&db);

      // create the rebar pattern (definition of rebar in the cross section)
      CComPtr<IBridgeDeckRebarPattern> rebar_pattern;
      rebar_pattern.CoCreateInstance(CLSID_BridgeDeckRebarPattern);

      // Locate rebar from the bottom of the deck
      Float64 Y = rebar_data.BottomCover + db/2;

      // create the rebar pattern
      rebar_pattern->putref_Rebar(rebar);
      rebar_pattern->putref_RebarLayoutItem(deck_rebar_layout_item);
      rebar_pattern->put_Spacing( rebar_data.BottomSpacing );
      rebar_pattern->put_Location( Y );
      rebar_pattern->put_IsLumped(VARIANT_FALSE);

      // add this pattern to the layout
      deck_rebar_layout_item->AddRebarPattern(rebar_pattern);
   }

   // Bottom Mat - Lump Sum
   if ( !IsZero(rebar_data.BottomLumpSum) )
   {
      MaterialSpec  matSpec  = GetRebarSpecification(rebar_data.BottomRebarType);
      RebarGrade    matGrade = GetRebarGrade(rebar_data.BottomRebarGrade);

      CComPtr<IRebar> rebar;
      rebar_factory->CreateRebar(matSpec,matGrade,bs3,unit_convert,compositeDeckIntervalIdx,&rebar);
      rebar->put_NominalDiameter(0.0);
      rebar->put_NominalArea(rebar_data.BottomLumpSum);

      Float64 Y = rebar_data.BottomCover;

      // create the rebar pattern (definition of rebar in the cross section)
      CComPtr<IBridgeDeckRebarPattern> rebar_pattern;
      rebar_pattern.CoCreateInstance(CLSID_BridgeDeckRebarPattern);

      // create the rebar pattern
      rebar_pattern->putref_Rebar(rebar);
      rebar_pattern->putref_RebarLayoutItem(deck_rebar_layout_item);
      rebar_pattern->put_Spacing( 0.0 );
      rebar_pattern->put_Location( Y );
      rebar_pattern->put_IsLumped(VARIANT_TRUE);

      // add this pattern to the layout
      deck_rebar_layout_item->AddRebarPattern(rebar_pattern);
   }

   // Negative moment rebar over piers
   std::vector<CDeckRebarData::NegMomentRebarData>::const_iterator iter(rebar_data.NegMomentRebar.begin());
   std::vector<CDeckRebarData::NegMomentRebarData>::const_iterator end(rebar_data.NegMomentRebar.end());
   for ( ; iter != end; iter++ )
   {
      const CDeckRebarData::NegMomentRebarData& nm_rebar_data = *iter;

      CComPtr<INegativeMomentBridgeDeckRebarLayoutItem> nm_deck_rebar_layout_item;
      nm_deck_rebar_layout_item.CoCreateInstance(CLSID_NegativeMomentBridgeDeckRebarLayoutItem);
      nm_deck_rebar_layout_item->putref_Bridge(m_Bridge);
      rebar_layout->Add(nm_deck_rebar_layout_item);

      // negative moment rebar needs to be associed with a pier in the generic bridge model
      // association is done by pier ID. The generic bridge model Pier ID is the same as
      // its PierLine ID.
      nm_deck_rebar_layout_item->put_PierID( ::GetPierLineID(nm_rebar_data.PierIdx) );
      nm_deck_rebar_layout_item->put_LeftCutoff(  nm_rebar_data.LeftCutoff );
      nm_deck_rebar_layout_item->put_RightCutoff( nm_rebar_data.RightCutoff );

      if ( nm_rebar_data.RebarSize != matRebar::bsNone )
      {
         // Rebar
         BarSize       matSize  = GetBarSize(nm_rebar_data.RebarSize);
         MaterialSpec  matSpec  = GetRebarSpecification(nm_rebar_data.RebarType);
         RebarGrade    matGrade = GetRebarGrade(nm_rebar_data.RebarGrade);

         CComPtr<IRebar> rebar;
         rebar_factory->CreateRebar(matSpec,matGrade,matSize,unit_convert,compositeDeckIntervalIdx,&rebar);

         Float64 db;
         rebar->get_NominalDiameter(&db);

         // create the rebar pattern (definition of rebar in the cross section)
         CComPtr<IBridgeDeckRebarPattern> rebar_pattern;
         rebar_pattern.CoCreateInstance(CLSID_BridgeDeckRebarPattern);

         // Locate rebar from the bottom of the deck
         Float64 Y;
         if ( nm_rebar_data.Mat == CDeckRebarData::TopMat )
         {
            Y = deck_height - rebar_data.TopCover - db/2;
         }
         else
         {
            Y = rebar_data.BottomCover + db/2;
         }

         // create the rebar pattern
         rebar_pattern->putref_Rebar(rebar);
         rebar_pattern->putref_RebarLayoutItem(nm_deck_rebar_layout_item);
         rebar_pattern->put_Spacing( nm_rebar_data.Spacing );
         rebar_pattern->put_Location( Y );
         rebar_pattern->put_IsLumped(VARIANT_FALSE);

         // add this pattern to the layout
         nm_deck_rebar_layout_item->AddRebarPattern(rebar_pattern);
      }

      // Lump Sum
      if ( !IsZero(nm_rebar_data.LumpSum) )
      {
         MaterialSpec  matSpec  = GetRebarSpecification(nm_rebar_data.RebarType);
         RebarGrade    matGrade = GetRebarGrade(nm_rebar_data.RebarGrade);

         // create a dummy #3 bar, then change the diameter and bar area to match
         // the lump sum bar
         CComPtr<IRebar> rebar;
         rebar_factory->CreateRebar(matSpec,matGrade,bs3,unit_convert,compositeDeckIntervalIdx,&rebar);
         rebar->put_NominalDiameter(0.0);
         rebar->put_NominalArea(nm_rebar_data.LumpSum);

         Float64 Y;
         if ( nm_rebar_data.Mat == CDeckRebarData::TopMat )
         {
            Y = deck_height - rebar_data.TopCover;
         }
         else
         {
            Y = rebar_data.BottomCover;
         }

         // create the rebar pattern (definition of rebar in the cross section)
         CComPtr<IBridgeDeckRebarPattern> rebar_pattern;
         rebar_pattern.CoCreateInstance(CLSID_BridgeDeckRebarPattern);

         // create the rebar pattern
         rebar_pattern->putref_Rebar(rebar);
         rebar_pattern->putref_RebarLayoutItem(nm_deck_rebar_layout_item);
         rebar_pattern->put_Spacing( 0.0 );
         rebar_pattern->put_Location( Y );
         rebar_pattern->put_IsLumped(VARIANT_TRUE);

         // add this pattern to the layout
         nm_deck_rebar_layout_item->AddRebarPattern(rebar_pattern);
      }
   }
}

void CBridgeAgentImp::LayoutSegmentRebar(const CSegmentKey& segmentKey)
{
   // Get the rebar input data
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   const CLongitudinalRebarData& rebar_data = pSegment->LongitudinalRebarData;

   GET_IFACE_NOCHECK(IEAFStatusCenter,pStatusCenter);

   const std::vector<CLongitudinalRebarData::RebarRow>& rebar_rows = rebar_data.RebarRows;
   if ( 0 < rebar_rows.size() )
   {
      // Get the stage where the rebar is first introducted into the system.
      // Technically, the rebar is first introduced to the system when the segment
      // concrete is cast, however the rebar isn't doing anything structural
      // until the concrete is cured and the prestressing is released
      IntervalIndexType releaseIntervalIdx = m_IntervalManager.GetPrestressReleaseInterval(segmentKey);

      // Get the PrecastGirder object that will create the rebar model in
      // Given the basic input, this guy will compute the actual coordinates of
      // rebar at any cross section along the segment
      CComPtr<IPrecastGirder> girder;
      GetGirder(segmentKey,&girder);

      Float64 segment_length = GetSegmentLength(segmentKey);
      Float64 startEndDist = GetSegmentStartEndDistance(segmentKey);
      Float64 endEndDist   = GetSegmentEndEndDistance(segmentKey);

      CComPtr<IRebarLayout> rebar_layout;
      girder->get_RebarLayout(&rebar_layout);

      GET_IFACE(ILongitudinalRebar,pLongRebar);
      std::_tstring strRebarName = pLongRebar->GetSegmentLongitudinalRebarMaterial(segmentKey);
      
      matRebar::Grade grade;
      matRebar::Type type;
      pLongRebar->GetSegmentLongitudinalRebarMaterial(segmentKey,type,grade);

      MaterialSpec matSpec = GetRebarSpecification(type);
      RebarGrade matGrade  = GetRebarGrade(grade);

      CComPtr<IRebarFactory> rebar_factory;
      rebar_factory.CoCreateInstance(CLSID_RebarFactory);

      CComPtr<IUnitServer> unitServer;
      unitServer.CoCreateInstance(CLSID_UnitServer);
      HRESULT hr = ConfigureUnitServer(unitServer);
      ATLASSERT(SUCCEEDED(hr));

      CComPtr<IUnitConvert> unit_convert;
      unitServer->get_UnitConvert(&unit_convert);

      // need this for clear spacing validation below
      Float64 max_aggregate_size = GetSegmentMaxAggrSize(segmentKey);

      pgsTypes::SegmentVariationType segmentVariation = pSegment->GetVariationType();

      for ( RowIndexType idx = 0; idx < rebar_rows.size(); idx++ )
      {
         const CLongitudinalRebarData::RebarRow& info = rebar_rows[idx];

         Float64 start(0), end(0);

         if ( !info.GetRebarStartEnd(segment_length, &start, &end) )
         {
            continue; // this bar doesn't go on this segment
         }


         BarSize bar_size = GetBarSize(info.BarSize);
         CComPtr<IRebar> rebar;
         rebar_factory->CreateRebar(matSpec,matGrade,bar_size,unit_convert,releaseIntervalIdx,&rebar);

         Float64 db;
         rebar->get_NominalDiameter(&db);

         // validate bar spacing within the row
         // LRFD 5.10.3.1.2 - clear distance between parallel bars in a layer shall not be less than:
         // * nominal diameter of bar
         // * 1.33 times the maximum size of the coarse aggregate
         // * 1.0 inch
         // clear spacing is bar spacing minus nominal diameter of bar
         if ( 1 < info.NumberOfBars ) // only have to validate spacing between bars if there is more than one bar
         {
            Float64 clear = info.BarSpacing - db;
            if ( clear < db )
            {
               std::_tostringstream os;
               os << SEGMENT_LABEL(segmentKey)
                  << _T(": Clearance between longitudinal bars in row ") << LABEL_INDEX(idx) << _T(" is less than the nominal diameter of the bar (See LRFD 5.10.3.1.2)") << std::endl;

               pgsGirderDescriptionStatusItem* pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,EGD_LONG_REINF,m_StatusGroupID,m_scidGirderDescriptionWarning,os.str().c_str());

               pStatusCenter->Add(pStatusItem);
            }
            else if ( clear < 1.33*max_aggregate_size )
            {
               std::_tostringstream os;
               os << SEGMENT_LABEL(segmentKey)
                  << _T(": Clearance between longitudinal bars in row ") << LABEL_INDEX(idx) << _T(" is less than 1.33 times the maximum size of the coarse aggregate (See LRFD 5.10.3.1.2)") << std::endl;

               pgsGirderDescriptionStatusItem* pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,EGD_LONG_REINF,m_StatusGroupID,m_scidGirderDescriptionWarning,os.str().c_str());

               pStatusCenter->Add(pStatusItem);
            }
            else if ( clear < ::ConvertToSysUnits(1.0,unitMeasure::Inch) )
            {
               std::_tostringstream os;
               os << SEGMENT_LABEL(segmentKey)
                  << _T(": Clearance between longitudinal bars in row ") << LABEL_INDEX(idx) << _T(" is less than 1.0 inch (See LRFD 5.10.3.1.2)") << std::endl;

               pgsGirderDescriptionStatusItem* pStatusItem = new pgsGirderDescriptionStatusItem(segmentKey,EGD_LONG_REINF,m_StatusGroupID,m_scidGirderDescriptionWarning,os.str().c_str());

               pStatusCenter->Add(pStatusItem);
            }
         }

         // break the bar into segments. Bar is modeled as straight except for bars that are
         // referenced from the bottom of the segment and the segment has a linear or parabolic profile
         std::vector<std::pair<Float64,Float64>> vBarSegments;
         if ( (segmentVariation == pgsTypes::svtLinear || segmentVariation == pgsTypes::svtParabolic) && 
               info.Face == pgsTypes::BottomFace )
         {
            // if the segment has a linear variation and the bar is referenced from the bottom
            // of the girder, make it follow the bottom of the girder.
            Float64 L1 = pSegment->GetVariationLength(pgsTypes::sztLeftPrismatic) - startEndDist;
            L1 = (L1 < 0 ? 0 : L1);
            Float64 L3 = pSegment->GetVariationLength(pgsTypes::sztRightPrismatic) - endEndDist;
            L3 = (L3 < 0 ? 0 : L3);
            Float64 L2 = segment_length - L1 - L3;

            Float64 s1 = (start < L1 ? start : -1);
            Float64 e1 = (end   < L1 ? end   : L1);

            Float64 s2 = (start < L1 ? L1 : (start < (L1+L2) ? start : -1));
            Float64 e2 = (end   < L1 ? -1 : (end   < (L1+L2) ? end   : L1+L2));

            Float64 s3 = (start < (L1+L2) ? L1+L2 : start);
            Float64 e3 = (end   < (L1+L2) ? -1    : end);

            if ( 0 <= s1 && 0 <= e1 && !IsEqual(s1,e1)  )
            {
               vBarSegments.push_back(std::make_pair(s1,e1));
            }

            if ( 0 <= s2 && 0 <= e2 && !IsEqual(s2,e2) )
            {
               vBarSegments.push_back(std::make_pair(s2,e2));
            }

            if ( 0 <= s3 && 0 <= e3 && !IsEqual(s3,e3)  )
            {
               vBarSegments.push_back(std::make_pair(s3,e3));
            }
         }
         else if ( (segmentVariation == pgsTypes::svtDoubleLinear || segmentVariation == pgsTypes::svtDoubleParabolic) && 
                   info.Face == pgsTypes::BottomFace )
         {
            // if the segment has a double linear variation and the bar is referenced from the bottom
            // of the girder, make it follow the bottom of the girder.
            Float64 L1 = pSegment->GetVariationLength(pgsTypes::sztLeftPrismatic) - startEndDist;
            Float64 L2 = pSegment->GetVariationLength(pgsTypes::sztLeftTapered);
            Float64 L4 = pSegment->GetVariationLength(pgsTypes::sztRightTapered);
            Float64 L5 = pSegment->GetVariationLength(pgsTypes::sztRightPrismatic) - endEndDist;
            L1 = (L1 < 0 ? 0 : L1);
            L5 = (L5 < 0 ? 0 : L5);
            if ( IsZero(L1) )
            {
               L2 = Max(0.0,L2-startEndDist);
            }
            if ( IsZero(L5) )
            {
               L4 = Max(0.0,L4-endEndDist);
            }
            Float64 L3 = segment_length - L1 - L2 - L4 - L5;

            Float64 s1 = (start < L1 ? start : -1);
            Float64 e1 = (end   < L1 ? end   : L1);

            Float64 s2 = (start < L1 ? L1 : (start < (L1+L2) ? start : -1));
            Float64 e2 = (end   < L1 ? -1 : (end   < (L1+L2) ? end   : L1+L2));

            Float64 s3 = (start < (L1+L2) ? L1+L2 : start);
            Float64 e3 = (end   < (L1+L2) ? -1    : (L1+L2+L3));

            Float64 s4 = (start < (L1+L2+L3) ? (L1+L2+L3) : (start < (L1+L2+L3+L4) ? start : -1));
            Float64 e4 = (end   < (L1+L2+L3) ? -1         : (end   < (L1+L2+L3+L4) ? end   : (L1+L2+L3+L4)));

            Float64 s5 = (start < (L1+L2+L3+L4) ? (L1+L2+L3+L4) : start);
            Float64 e5 = (end   < (L1+L2+L3+L4) ? -1            : end);

            if ( 0 <= s1 && 0 <= e1 && !IsEqual(s1,e1)  )
            {
               vBarSegments.push_back(std::make_pair(s1,e1));
            }

            if ( 0 <= s2 && 0 <= e2 && !IsEqual(s2,e2)  )
            {
               vBarSegments.push_back(std::make_pair(s2,e2));
            }

            if ( 0 <= s3 && 0 <= e3 && !IsEqual(s3,e3)  )
            {
               vBarSegments.push_back(std::make_pair(s3,e3));
            }

            if ( 0 <= s4 && 0 <= e4 && !IsEqual(s4,e4)  )
            {
               vBarSegments.push_back(std::make_pair(s4,e4));
            }

            if ( 0 <= s5 && 0 <= e5 && !IsEqual(s5,e5)  )
            {
               vBarSegments.push_back(std::make_pair(s5,e5));
            }
         }
         else
         {
#pragma Reminder("UPDATE: bars are straight for parabolic and double parabolic segments")
            // if the segment is parabolic or double parabolic and the bar is measured
            // from the bottom, we get into this else block and the resultant bar is straight.
            // Need to have a way to model the bars as parabolic and following the bottom of the
            // segment. Bars should be flexible enough to take on a curved shape

            // the bar is straight from start to end
            vBarSegments.push_back(std::make_pair(start,end));
         }

         std::vector<std::pair<Float64,Float64>>::iterator barSegmentIter(vBarSegments.begin());
         std::vector<std::pair<Float64,Float64>>::iterator barSegmentIterEnd(vBarSegments.end());
         for ( ; barSegmentIter != barSegmentIterEnd; barSegmentIter++ )
         {
            Float64 startLayout = barSegmentIter->first;
            Float64 endLayout = barSegmentIter->second;

            CComPtr<IFixedLengthRebarLayoutItem> fixedlength_layout_item;
            hr = fixedlength_layout_item.CoCreateInstance(CLSID_FixedLengthRebarLayoutItem);

            fixedlength_layout_item->put_Start(startLayout);
            fixedlength_layout_item->put_End(endLayout);

            // Set pattern for layout
            pgsPointOfInterest startPoi(segmentKey,startLayout);
            pgsPointOfInterest endPoi(segmentKey,endLayout);

            // Get the start and end segment height
            // Since we are in the process of validating the bridge model
            // we can't use the normal section properties interface (recursion would result).
            // The technique that will be used here will be to get the bounding box of
            // the segment shape and compute the height based on its top and bottom elevation

            // (1) get shape
            CComPtr<IShape> startShape, endShape;
            GetSegmentShapeDirect(startPoi,&startShape);
            GetSegmentShapeDirect(endPoi,  &endShape);

            // (2) get bounding box
            CComPtr<IRect2d> bbStart, bbEnd;
            startShape->get_BoundingBox(&bbStart);
            endShape->get_BoundingBox(&bbEnd);

            // (3) compute height of segment
            Float64 HgStart, HgEnd;
            bbStart->get_Height(&HgStart);
            bbEnd->get_Height(&HgEnd);


            /////////////////////////////////////////////////////


            CComPtr<IRebarRowPattern> row_pattern;
            row_pattern.CoCreateInstance(CLSID_RebarRowPattern);

            // Locate the rebar in Girder Section Coordinates
            // (0,0) is at the top of the girder so Y is measured
            // down from the top
            Float64 yStart, yEnd;
            if ( info.Face == pgsTypes::TopFace )
            {
               yStart = -(info.Cover + db/2);
               yEnd   = yStart;
            }
            else
            {
               yStart = -(HgStart - info.Cover - db/2);
               yEnd   = -(HgEnd   - info.Cover - db/2);
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

            fixedlength_layout_item->AddRebarPattern(row_pattern);

            rebar_layout->Add(fixedlength_layout_item);
         }
      }
   }
}

void CBridgeAgentImp::LayoutClosureJointRebar(const CClosureKey& closureKey)
{
   // Get the rebar input data
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CClosureJointData* pClosure = pIBridgeDesc->GetClosureJointData(closureKey);
   const CLongitudinalRebarData& rebar_data = pClosure->GetRebar();

   // If there aren't any rows of rebar data defined for this segment,
   // leave now
   const std::vector<CLongitudinalRebarData::RebarRow>& rebar_rows = rebar_data.RebarRows;
   if ( rebar_rows.size() == 0 )
   {
      return;
   }

   // Get the stage where the rebar is first introducted into the system.
   // Technically, the rebar is first introduced to the system when the closure joint
   // concrete is cast, however the rebar isn't doing anything structural
   // until the concrete is cured
   IntervalIndexType compositeClosureIntervalIdx = GetCompositeClosureJointInterval(closureKey);

   // Get the keys for the segments on either side of the closure
   CSegmentKey prevSegmentKey(pClosure->GetLeftSegment()->GetSegmentKey());
   CSegmentKey nextSegmentKey(pClosure->GetRightSegment()->GetSegmentKey());

   // Get the PrecastGirder object that will create the rebar model in
   // Given the basic input, this guy will compute the actual coordinates of
   // rebar at any cross section along the segment
   CComPtr<IPrecastGirder> girder;
   GetGirder(prevSegmentKey,&girder);

   CComPtr<IRebarLayout> rebar_layout;
   girder->get_ClosureJointRebarLayout(&rebar_layout);

   // Gather some information for layout of the rebar along the length of the
   // girder and within the girder cross section
   Float64 closure_length = GetClosureJointLength(closureKey);
   Float64 segment_length = GetSegmentLength(prevSegmentKey);
   pgsPointOfInterest startPoi(prevSegmentKey,segment_length);
   pgsPointOfInterest endPoi(nextSegmentKey,0,0);

   // Get the start and end closure height
   // Since we are in the process of validating the bridge model
   // we can't use the normal section properties interface (recursion would result).
   // The technique that will be used here will be to get the bounding box of
   // the segment shape and compute the height based on its top and bottom elevation

   // (1) get shape
   CComPtr<IShape> startShape, endShape;
   GetSegmentShapeDirect(startPoi,&startShape);
   GetSegmentShapeDirect(endPoi,  &endShape);

   // (2) get bounding box
   CComPtr<IRect2d> bbStart, bbEnd;
   startShape->get_BoundingBox(&bbStart);
   endShape->get_BoundingBox(&bbEnd);

   // (3) compute height of segment
   Float64 HgStart, HgEnd;
   bbStart->get_Height(&HgStart);
   bbEnd->get_Height(&HgEnd);

   // Get some basic information about the rebar so we can build a rebar material object
   // We need to map PGSuper data into WBFLGenericBridge data
   MaterialSpec matSpec  = GetRebarSpecification(rebar_data.BarType);
   RebarGrade   matGrade = GetRebarGrade(rebar_data.BarGrade);

   // Create a rebar factory. This does the work of creating rebar objects
   CComPtr<IRebarFactory> rebar_factory;
   rebar_factory.CoCreateInstance(CLSID_RebarFactory);

   // Need a unit server object for units conversion
   CComPtr<IUnitServer> unitServer;
   unitServer.CoCreateInstance(CLSID_UnitServer);
   HRESULT hr = ConfigureUnitServer(unitServer);
   ATLASSERT(SUCCEEDED(hr));

   CComPtr<IUnitConvert> unit_convert;
   unitServer->get_UnitConvert(&unit_convert);

   IndexType nRows = rebar_rows.size();
   for ( IndexType idx = 0; idx < nRows; idx++ )
   {
      // get the data for this row
      const CLongitudinalRebarData::RebarRow& info = rebar_rows[idx];

      // if there are bars in this row, and the bars have size, define
      // the rebar pattern for this row. Rebar pattern is the definition of the
      // rebar within the cross section
      if ( 0 < info.BarSize && 0 < info.NumberOfBars )
      {
         CComPtr<IFixedLengthRebarLayoutItem> fixed_layout_item;
         fixed_layout_item.CoCreateInstance(CLSID_FixedLengthRebarLayoutItem);
         ATLASSERT(fixed_layout_item);

         fixed_layout_item->put_Start(segment_length);
         fixed_layout_item->put_End(segment_length + closure_length);

         CComQIPtr<IRebarLayoutItem> rebar_layout_item(fixed_layout_item);

         // Create the rebar object
         BarSize bar_size = GetBarSize(info.BarSize);
         CComPtr<IRebar> rebar;
         rebar_factory->CreateRebar(matSpec,matGrade,bar_size,unit_convert,compositeClosureIntervalIdx,&rebar);

         Float64 db;
         rebar->get_NominalDiameter(&db);

         // create the rebar pattern (definition of rebar in the cross section)
         CComPtr<IRebarRowPattern> row_pattern;
         row_pattern.CoCreateInstance(CLSID_RebarRowPattern);

         // Locate the rebar in Girder Section Coordinates
         // (0,0) is at the top of the girder so Y is measured
         // down from the top
         Float64 yStart, yEnd;
         if ( info.Face == pgsTypes::TopFace )
         {
            yStart = -(info.Cover + db/2);
            yEnd   = yStart;
         }
         else
         {
            yStart = -(HgStart - info.Cover - db/2);
            yEnd   = -(HgEnd   - info.Cover - db/2);
         }

         CComPtr<IPoint2d> startAnchor;
         startAnchor.CoCreateInstance(CLSID_Point2d);
         startAnchor->Move(0,yStart);

         CComPtr<IPoint2d> endAnchor;
         endAnchor.CoCreateInstance(CLSID_Point2d);
         endAnchor->Move(0,yEnd);

         // create the rebar pattern
         row_pattern->putref_Rebar(rebar);
         row_pattern->put_AnchorPoint(etStart, startAnchor);
         row_pattern->put_AnchorPoint(etEnd,   endAnchor);
         row_pattern->put_Count( info.NumberOfBars );
         row_pattern->put_Spacing( info.BarSpacing );
         row_pattern->put_Orientation( rroHCenter );

         // add this pattern to the layout
         rebar_layout_item->AddRebarPattern(row_pattern);
      
         rebar_layout->Add(rebar_layout_item);
      }
   }
}

void CBridgeAgentImp::CheckBridge()
{
   GET_IFACE_NOCHECK(IEAFStatusCenter,pStatusCenter); // only used if there is an issue

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // make sure all girders have positive lengths
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType nGirders = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();
      for( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);
            CComPtr<IPrecastGirder> girder;
            GetGirder(segmentKey, &girder);

            Float64 sLength;
            girder->get_SpanLength(&sLength);

            if ( sLength <= 0 )
            {
               std::_tostringstream os;
               os << SEGMENT_LABEL(segmentKey) << _T(" does not have a positive length.") << std::endl;

               pgsBridgeDescriptionStatusItem* pStatusItem = 
                  new pgsBridgeDescriptionStatusItem(m_StatusGroupID,m_scidBridgeDescriptionError,pgsBridgeDescriptionStatusItem::General,os.str().c_str());

               pStatusCenter->Add(pStatusItem);

               os << _T("See Status Center for Details");
               THROW_UNWIND(os.str().c_str(),XREASON_NEGATIVE_GIRDER_LENGTH);
            }

            // Check location of temporary strands... usually in the top half of the girder
            const GirderLibraryEntry* pGirderEntry = GetGirderLibraryEntry(segmentKey);
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

               if ( Y < -h_start/2 || Y < -h_end/2 )
               {
                  CString strMsg;
                  strMsg.Format(_T("%s, Temporary strands are not in the top half of the girder"),SEGMENT_LABEL(CSegmentKey(grpIdx,gdrIdx,segIdx)));
                  pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID,m_scidInformationalWarning,strMsg);
                  pStatusCenter->Add(pStatusItem);
               }
            }

            if ( segIdx < nSegments-1 )
            {
               // Closure Joints
               CClosureKey closureKey = segmentKey;
               Float64 closure_length = GetClosureJointLength(closureKey);
               if ( ::IsLE(closure_length,0.0) )
               {
                  std::_tostringstream os;
                  os << _T("Length closure joint for Group ") << LABEL_GROUP(grpIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << _T(" at the end of Segment ") << LABEL_SEGMENT(segIdx)
                     << _T(" must be greater than zero.") << std::endl;

                  pgsBridgeDescriptionStatusItem* pStatusItem = 
                     new pgsBridgeDescriptionStatusItem(m_StatusGroupID,m_scidBridgeDescriptionError,pgsBridgeDescriptionStatusItem::Framing,os.str().c_str());

                  pStatusCenter->Add(pStatusItem);

                  os << _T("See Status Center for Details");
                  THROW_UNWIND(os.str().c_str(),XREASON_INVALID_CLOSURE_JOINT_LENGTH);
               }
            }
         } // next segment
      } // next girder
   } // next group

   // COGO model doesn't save correctly... fix this later
//#if defined _DEBUG
//   // Dumps the cogo model to a file so it can be viewed/debugged
//   CComPtr<IStructuredSave2> save;
//   save.CoCreateInstance(CLSID_StructuredSave2);
//   save->Open(CComBSTR(_T("CogoModel.cogo")));
//
//   save->BeginUnit(CComBSTR(_T("Cogo")),1.0);
//
//   CComQIPtr<IStructuredStorage2> ss(m_CogoModel);
//   ss->Save(save);
//
//   save->EndUnit();
//   save->Close();
//#endif // _DEBUG
}

SpanIndexType CBridgeAgentImp::GetSpanCount_Private()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   return pBridgeDesc->GetSpanCount();
}

PierIndexType CBridgeAgentImp::GetPierCount_Private()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   return pBridgeDesc->GetPierCount();
}

bool CBridgeAgentImp::ComputeNumPermanentStrands(StrandIndexType totalPermanent,const CSegmentKey& segmentKey,StrandIndexType* numStraight,StrandIndexType* numHarped)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   HRESULT hr = m_StrandFillers[segmentKey].ComputeNumPermanentStrands(girder, totalPermanent, numStraight, numHarped);
   ATLASSERT(SUCCEEDED(hr));

   return hr==S_OK;
}

bool CBridgeAgentImp::ComputeNumPermanentStrands(StrandIndexType totalPermanent,LPCTSTR strGirderName, StrandIndexType* numStraight, StrandIndexType* numHarped)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGirderEntry = pLib->GetGirderEntry( strGirderName );
   return pGirderEntry->GetPermStrandDistribution(totalPermanent, numStraight, numHarped);
}

StrandIndexType CBridgeAgentImp::GetMaxNumPermanentStrands(const CSegmentKey& segmentKey)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   StrandIndexType maxNum;
   HRESULT hr = m_StrandFillers[segmentKey].GetMaxNumPermanentStrands(girder, &maxNum);
   ATLASSERT(SUCCEEDED(hr));

   return maxNum;
}

StrandIndexType CBridgeAgentImp::GetMaxNumPermanentStrands(LPCTSTR strGirderName)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGirderEntry = pLib->GetGirderEntry( strGirderName );
   std::vector<GirderLibraryEntry::StrandDefinitionType> permStrands = pGirderEntry->GetPermanentStrands();
   return permStrands.size()-1;
}

StrandIndexType CBridgeAgentImp::GetPreviousNumPermanentStrands(const CSegmentKey& segmentKey,StrandIndexType curNum)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   StrandIndexType nextNum;
   HRESULT hr = m_StrandFillers[segmentKey].GetPreviousNumberOfPermanentStrands(girder, curNum, &nextNum);
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

StrandIndexType CBridgeAgentImp::GetNextNumPermanentStrands(const CSegmentKey& segmentKey,StrandIndexType curNum)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   StrandIndexType nextNum;
   HRESULT hr = m_StrandFillers[segmentKey].GetNextNumberOfPermanentStrands(girder, curNum, &nextNum);
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

bool CBridgeAgentImp::IsValidNumStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType curNum)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   VARIANT_BOOL bIsValid;
   HRESULT hr = E_FAIL;
   switch( type )
   {
   case pgsTypes::Straight:
      hr = m_StrandFillers[segmentKey].IsValidNumStraightStrands(girder, curNum, &bIsValid);
      break;

   case pgsTypes::Harped:
      hr = m_StrandFillers[segmentKey].IsValidNumHarpedStrands(girder, curNum, &bIsValid);
      break;

   case pgsTypes::Temporary:
      hr = m_StrandFillers[segmentKey].IsValidNumTemporaryStrands(girder, curNum, &bIsValid);
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

   // The position of the strands in the strand grid doesn't matter
   // for this method. Just use dummy values.
   Float64 Hg = 0;

   VARIANT_BOOL isvalid(VARIANT_FALSE);
   HRESULT hr(E_FAIL);
   switch( type )
   {
   case pgsTypes::Straight:
      {
         CComPtr<IStrandGrid> startGrid, endGrid;
         startGrid.CoCreateInstance(CLSID_StrandGrid);
         endGrid.CoCreateInstance(CLSID_StrandGrid);
         pGdrEntry->ConfigureStraightStrandGrid(Hg,Hg,startGrid,endGrid);

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
         pGdrEntry->ConfigureHarpedStrandGrids(Hg,Hg,Hg,Hg,startGrid,startHPGrid,endHPGrid,endGrid);

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
         pGdrEntry->ConfigureTemporaryStrandGrid(Hg,Hg,startGrid,endGrid);

         CComQIPtr<IStrandGridFiller> gridFiller(startGrid);

         hr = strandFiller.IsValidNumTemporaryStrands(gridFiller,curNum,&isvalid);
      }
      break;
   }
   ATLASSERT(SUCCEEDED(hr));

   return isvalid==VARIANT_TRUE;
}

StrandIndexType CBridgeAgentImp::GetNextNumStraightStrands(const CSegmentKey& segmentKey,StrandIndexType curNum)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   StrandIndexType nextNum;
   HRESULT hr = m_StrandFillers[segmentKey].GetNextNumberOfStraightStrands(girder, curNum, &nextNum);
   ATLASSERT(SUCCEEDED(hr));

   return nextNum;
}

StrandIndexType CBridgeAgentImp::GetNextNumStraightStrands(LPCTSTR strGirderName,StrandIndexType curNum)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   CStrandFiller strandFiller;
   strandFiller.Init(pGdrEntry);

   // The position of the strands in the strand grid doesn't matter
   // for this method. Just use dummy values.
   Float64 Hg = 0;

   CComPtr<IStrandGrid> startGrid, endGrid;
   startGrid.CoCreateInstance(CLSID_StrandGrid);
   endGrid.CoCreateInstance(CLSID_StrandGrid);
   pGdrEntry->ConfigureStraightStrandGrid(Hg,Hg,startGrid,endGrid);

   CComQIPtr<IStrandGridFiller> gridFiller(startGrid);

   StrandIndexType nextNum;
   strandFiller.GetNextNumberOfStraightStrands(gridFiller,curNum,&nextNum);

   return nextNum;
}

StrandIndexType CBridgeAgentImp::GetNextNumHarpedStrands(const CSegmentKey& segmentKey,StrandIndexType curNum)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   StrandIndexType nextNum;
   HRESULT hr = m_StrandFillers[segmentKey].GetNextNumberOfHarpedStrands(girder, curNum, &nextNum);
   ATLASSERT(SUCCEEDED(hr));

   return nextNum;
}

StrandIndexType CBridgeAgentImp::GetNextNumHarpedStrands(LPCTSTR strGirderName,StrandIndexType curNum)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   CStrandFiller strandFiller;
   strandFiller.Init(pGdrEntry);

   // The position of the strands in the strand grid doesn't matter
   // for this method. Just use dummy values.
   Float64 Hg = 0;

   CComPtr<IStrandGrid> startGrid, startHPGrid, endHPGrid, endGrid;
   startGrid.CoCreateInstance(CLSID_StrandGrid);
   startHPGrid.CoCreateInstance(CLSID_StrandGrid);
   endHPGrid.CoCreateInstance(CLSID_StrandGrid);
   endGrid.CoCreateInstance(CLSID_StrandGrid);
   pGdrEntry->ConfigureHarpedStrandGrids(Hg,Hg,Hg,Hg,startGrid,startHPGrid,endHPGrid,endGrid);

   CComQIPtr<IStrandGridFiller> gridFiller(startGrid);

   StrandIndexType nextNum;
   strandFiller.GetNextNumberOfHarpedStrands(pGdrEntry->OddNumberOfHarpedStrands(),gridFiller,curNum,&nextNum);

   return nextNum;
}

StrandIndexType CBridgeAgentImp::GetNextNumTempStrands(const CSegmentKey& segmentKey,StrandIndexType curNum)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   StrandIndexType nextNum;
   HRESULT hr = m_StrandFillers[segmentKey].GetNextNumberOfTemporaryStrands(girder, curNum, &nextNum);
   ATLASSERT(SUCCEEDED(hr));

   return nextNum;
}

StrandIndexType CBridgeAgentImp::GetNextNumTempStrands(LPCTSTR strGirderName,StrandIndexType curNum)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   CStrandFiller strandFiller;
   strandFiller.Init(pGdrEntry);

   // The position of the strands in the strand grid doesn't matter
   // for this method. Just use dummy values.
   Float64 Hg = 0;

   CComPtr<IStrandGrid> startGrid, endGrid;
   startGrid.CoCreateInstance(CLSID_StrandGrid);
   endGrid.CoCreateInstance(CLSID_StrandGrid);
   pGdrEntry->ConfigureTemporaryStrandGrid(Hg,Hg,startGrid,endGrid);

   CComQIPtr<IStrandGridFiller> gridFiller(startGrid);

   StrandIndexType nextNum;
   strandFiller.GetNextNumberOfTemporaryStrands(gridFiller,curNum,&nextNum);

   return nextNum;
}

StrandIndexType CBridgeAgentImp::GetPrevNumStraightStrands(const CSegmentKey& segmentKey,StrandIndexType curNum)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   StrandIndexType nextNum;
   HRESULT hr = m_StrandFillers[segmentKey].GetPreviousNumberOfStraightStrands(girder, curNum, &nextNum);
   ATLASSERT(SUCCEEDED(hr));

   return nextNum;
}

StrandIndexType CBridgeAgentImp::GetPrevNumStraightStrands(LPCTSTR strGirderName,StrandIndexType curNum)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   CStrandFiller strandFiller;
   strandFiller.Init(pGdrEntry);

   // The position of the strands in the strand grid doesn't matter
   // for this method. Just use dummy values.
   Float64 Hg = 0;

   CComPtr<IStrandGrid> startGrid, endGrid;
   startGrid.CoCreateInstance(CLSID_StrandGrid);
   endGrid.CoCreateInstance(CLSID_StrandGrid);
   pGdrEntry->ConfigureStraightStrandGrid(Hg,Hg,startGrid,endGrid);

   CComQIPtr<IStrandGridFiller> gridFiller(startGrid);

   StrandIndexType prevNum;
   strandFiller.GetPrevNumberOfStraightStrands(gridFiller,curNum,&prevNum);

   return prevNum;
}

StrandIndexType CBridgeAgentImp::GetPrevNumHarpedStrands(const CSegmentKey& segmentKey,StrandIndexType curNum)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   StrandIndexType nextNum;
   HRESULT hr = m_StrandFillers[segmentKey].GetPreviousNumberOfHarpedStrands(girder, curNum, &nextNum);
   ATLASSERT(SUCCEEDED(hr));

   return nextNum;
}

StrandIndexType CBridgeAgentImp::GetPrevNumHarpedStrands(LPCTSTR strGirderName,StrandIndexType curNum)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   CStrandFiller strandFiller;
   strandFiller.Init(pGdrEntry);

   // The position of the strands in the strand grid doesn't matter
   // for this method. Just use dummy values.
   Float64 Hg = 0;

   CComPtr<IStrandGrid> startGrid, startHPGrid, endHPGrid, endGrid;
   startGrid.CoCreateInstance(CLSID_StrandGrid);
   startHPGrid.CoCreateInstance(CLSID_StrandGrid);
   endHPGrid.CoCreateInstance(CLSID_StrandGrid);
   endGrid.CoCreateInstance(CLSID_StrandGrid);
   pGdrEntry->ConfigureHarpedStrandGrids(Hg,Hg,Hg,Hg,startGrid,startHPGrid,endHPGrid,endGrid);

   CComQIPtr<IStrandGridFiller> gridFiller(startGrid);

   StrandIndexType prevNum;
   strandFiller.GetPrevNumberOfHarpedStrands(pGdrEntry->OddNumberOfHarpedStrands(),gridFiller,curNum,&prevNum);

   return prevNum;
}

StrandIndexType CBridgeAgentImp::GetPrevNumTempStrands(const CSegmentKey& segmentKey,StrandIndexType curNum)
{
   VALIDATE( GIRDER );

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   StrandIndexType nextNum;
   HRESULT hr = m_StrandFillers[segmentKey].GetPreviousNumberOfTemporaryStrands(girder, curNum, &nextNum);
   ATLASSERT(SUCCEEDED(hr));

   return nextNum;
}

StrandIndexType CBridgeAgentImp::GetPrevNumTempStrands(LPCTSTR strGirderName,StrandIndexType curNum)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName);

   CStrandFiller strandFiller;
   strandFiller.Init(pGdrEntry);

   // The position of the strands in the strand grid doesn't matter
   // for this method. Just use dummy values.
   Float64 Hg = 0;

   CComPtr<IStrandGrid> startGrid, endGrid;
   startGrid.CoCreateInstance(CLSID_StrandGrid);
   endGrid.CoCreateInstance(CLSID_StrandGrid);
   pGdrEntry->ConfigureTemporaryStrandGrid(Hg,Hg,startGrid,endGrid);

   CComQIPtr<IStrandGridFiller> gridFiller(startGrid);

   StrandIndexType prevNum;
   strandFiller.GetPrevNumberOfTemporaryStrands(gridFiller,curNum,&prevNum);

   return prevNum;
}

Float64 CBridgeAgentImp::GetCutLocation(const pgsPointOfInterest& poi)
{
   GET_IFACE(IBridge, pBridge);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   Float64 dist_from_start_of_girder = poi.GetDistFromStart();

   Float64 start_brg_offset = pBridge->GetSegmentStartBearingOffset(segmentKey);
   Float64 start_end_size   = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 dist_from_start_pier = dist_from_start_of_girder - start_end_size + start_brg_offset;

   return dist_from_start_pier;
}

void CBridgeAgentImp::GetSegmentShapeDirect(const pgsPointOfInterest& poi,IShape** ppShape)
{
   // The primary shape is in Bridge Section Coordinates
   CComPtr<ISection> section;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   CComPtr<ISuperstructureMemberSegment> segment;
   GetSegment(segmentKey,&segment);

   Float64 Xs = poi.GetDistFromStart();
   segment->get_PrimaryShape(Xs,ppShape);
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

RebarGrade CBridgeAgentImp::GetRebarGrade(matRebar::Grade grade)
{
   RebarGrade matGrade;
   switch(grade)
   {
   case matRebar::Grade40: matGrade = Grade40; break;
   case matRebar::Grade60: matGrade = Grade60; break;
   case matRebar::Grade75: matGrade = Grade75; break;
   case matRebar::Grade80: matGrade = Grade80; break;
   case matRebar::Grade100: matGrade = Grade100; break;
   default:
      ATLASSERT(false);
   }

#if defined _DEBUG
      if ( matGrade == Grade100 )
      {
         ATLASSERT(lrfdVersionMgr::SixthEditionWith2013Interims <= lrfdVersionMgr::GetVersion());
      }
#endif

   return matGrade;
}

MaterialSpec CBridgeAgentImp::GetRebarSpecification(matRebar::Type type)
{
   return (type == matRebar::A615 ? msA615 : (type == matRebar::A706 ? msA706 : msA1035));
}

Float64 CBridgeAgentImp::GetAsTensionSideOfGirder(const pgsPointOfInterest& poi,bool bDevAdjust,bool bTensionTop)
{
   CComPtr<IRebarSection> rebar_section;
   GetRebars(poi,&rebar_section);

   CComPtr<IEnumRebarSectionItem> enum_items;
   rebar_section->get__EnumRebarSectionItem(&enum_items);

   Float64 half_depth_elevation = GetHalfElevation(poi); // y = 0 at top of girder... measured in Girder Section Coordinates

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
      location->get_Y(&y); // measured in Girder Section Coordinates

      // include bar if tension on top and y is greater than centerline or if
      // tension is not top (tension is bottom) and y is less than centerline
      bool bIncludeBar = ( (bTensionTop && half_depth_elevation < y) || (!bTensionTop && y <= half_depth_elevation) ) ? true : false;

      if ( bIncludeBar )
      {
         Float64 fra = 1.0;
         if ( bDevAdjust )
         {
            fra = GetDevLengthFactor(poi,item);
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

   Float64 Aps = 0.0;
   if ( !IsOnSegment(poi) )
   {
      return Aps;
   }
 
   Float64 half_depth_elevation = GetHalfElevation(poi); // y=0 at top of girder... measured in Girder Section Coordinates

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CComPtr<IPrecastGirder> girder;
   GetGirder(segmentKey,&girder);

   const matPsStrand* pStrand = GetStrandMaterial(segmentKey,pgsTypes::Straight);
   Float64 aps = pStrand->GetNominalArea();

   Float64 dist_from_start = poi.GetDistFromStart();
   Float64 segment_length = GetSegmentLength(segmentKey);

   // Only use approximate bond method if poi is in mid-section of beam (within CSS's).
   Float64 min_dist_from_ends = Min(dist_from_start, segment_length-dist_from_start);
   bool use_approximate = devAdjust==dlaApproximate && min_dist_from_ends > fabs(half_depth_elevation)*3.0; // Factor here is balance between performance and accuracy.

   // For approximate development length adjustment, take development length information at mid span and use for entire girder
   // adjusted for distance to ends of strands
   STRANDDEVLENGTHDETAILS dla_det;
   if(use_approximate)
   {
      std::vector<pgsPointOfInterest> vPoi( GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_5L,POIFIND_AND) );
      ATLASSERT(vPoi.size() == 1);
      const pgsPointOfInterest& rpoi = vPoi.front();

      GET_IFACE(IPretensionForce,pPSForce);
      if ( bUseConfig )
      {
         dla_det = pPSForce->GetDevLengthDetails(rpoi, config, false);
      }
      else
      {
         dla_det = pPSForce->GetDevLengthDetails(rpoi, false);
      }
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
      strand_point->get_Y(&y); // measured in Girder Section Coordinates

      // include bar if tension on top and y is greater than centerline or if
      // tension is not top (tension is bottom) and y is less than centerline
      bool bIncludeBar = ( (bTensionTop && half_depth_elevation < y) || (!bTensionTop && y <= half_depth_elevation) ) ? true : false;

      if ( bIncludeBar )
      {
         Float64 debond_factor;
         if(devAdjust==dlaNone)
         {
            debond_factor = 1.0;
         }
         else if(use_approximate)
         {
            // Use mid-span development length details to approximate debond factor
            // determine minimum bonded length from poi
            Float64 bond_start, bond_end;
            bool bDebonded;
            if (bUseConfig)
            {
               bDebonded = IsStrandDebonded(segmentKey,strandIdx,pgsTypes::Straight,config,&bond_start,&bond_end);
            }
            else
            {
               bDebonded = IsStrandDebonded(segmentKey,strandIdx,pgsTypes::Straight,&bond_start,&bond_end);
            }

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
               right_bonded_length = segment_length - dist_from_start;
            }

            Float64 bonded_length = Min(left_bonded_length, right_bonded_length);

            debond_factor = GetDevLengthAdjustment(bonded_length, dla_det.ld, dla_det.ltDetails.lt, dla_det.fps, dla_det.fpe);
         }
         else
         {
            // Full adjustment for development
            GET_IFACE(IPretensionForce,pPSForce);
            if ( bUseConfig )
            {
               debond_factor = pPSForce->GetStrandBondFactor(poi,config,strandIdx,pgsTypes::Straight);
            }
            else
            {
               debond_factor = pPSForce->GetStrandBondFactor(poi,strandIdx,pgsTypes::Straight);
            }
         }

         Aps += debond_factor*aps;
      }
   }

   // harped strands
   pStrand = GetStrandMaterial(segmentKey,pgsTypes::Harped);
   aps = pStrand->GetNominalArea();

   StrandIndexType Nh = 0;
   if ( bUseConfig )
   {
      Nh = config.PrestressConfig.GetStrandCount(pgsTypes::Harped);
   }
   else
   {
      GET_IFACE(IStrandGeometry,pStrandGeom);
      Nh = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Harped);
   }

   strand_points.Release();
   if(bUseConfig)
   {
      CIndexArrayWrapper strand_fill(config.PrestressConfig.GetStrandFill(pgsTypes::Harped));
      // Use CStrandMoverSwapper to temporarily swap out girder's strand mover and harping offset limits
      //  for design
      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      CStrandMoverSwapper swapper(segmentKey, config.PrestressConfig, this, girder, pIBridgeDesc);
      girder->get_HarpedStrandPositionsEx(dist_from_start, &strand_fill, &strand_points);
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
      strand_point->get_Y(&y); // measured in Girder Section Coordinates

      // include bar if tension on top and y is greater than centerline or if
      // tension is not top (tension is bottom) and y is less than centerline
      bool bIncludeBar = ( (bTensionTop && half_depth_elevation < y) || (!bTensionTop && y <= half_depth_elevation) ) ? true : false;

      if ( bIncludeBar )
      {
         Float64 debond_factor = 1.;
         bool use_one = use_approximate || devAdjust==dlaNone;
         if ( use_one )
         {
            debond_factor = 1.0;
         }
         else
         {
            GET_IFACE(IPretensionForce,pPSForce);
   
            if ( bUseConfig )
            {
               debond_factor = pPSForce->GetStrandBondFactor(poi,config,strandIdx,pgsTypes::Harped);
            }
            else
            {
               debond_factor = pPSForce->GetStrandBondFactor(poi,strandIdx,pgsTypes::Harped);
            }
         }

         Aps += debond_factor*aps;
      }
   }

   return Aps;
}

Float64 CBridgeAgentImp::GetAptTensionSide(const pgsPointOfInterest& poi,bool bTensionTop)
{
   VALIDATE( GIRDER );

   Float64 Apt = 0.0;
   if ( !IsOnGirder(poi) )
   {
      return Apt;
   }

   Float64 half_depth_elevation = GetHalfElevation(poi); // y=0 at top of girder... measured in Girder Section Coordinates

   CGirderKey girderKey(poi.GetSegmentKey());

   const matPsStrand* pTendon = GetTendonMaterial(girderKey);
   Float64 apt = pTendon->GetNominalArea();

   DuctIndexType nDucts = GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      CComPtr<IPoint2d> point;
      GetDuctPoint(poi,ductIdx,&point);

      IndexType nStrands = GetTendonStrandCount(girderKey,ductIdx);

      Float64 y;
      point->get_Y(&y);

      if ( (bTensionTop && half_depth_elevation < y) || (!bTensionTop && y <= half_depth_elevation) ) 
      {
         Apt += nStrands*apt;
      }
   }

   return Apt;
}

Float64 CBridgeAgentImp::GetAsDeckMats(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory,bool bTopMat,bool bBottomMat)
{
   Float64 As, Yb;
   GetDeckMatData(poi,barType,barCategory,bTopMat,bBottomMat,true,&As,&Yb);
   return As;
}

Float64 CBridgeAgentImp::GetLocationDeckMats(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory,bool bTopMat,bool bBottomMat)
{
   Float64 As, Yb;
   GetDeckMatData(poi,barType,barCategory,bTopMat,bBottomMat,true,&As,&Yb);
   return Yb;
}

void CBridgeAgentImp::GetDeckMatData(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory,bool bTopMat,bool bBottomMat,bool bAdjForDevLength,Float64* pAs,Float64* pYb)
{
#pragma Reminder("UPDATE: this information should come from the generic bridge model")
   // The location of deck bars are avaiable in the generic bridge model. They should be retreived
   // from that model instead of re-computed here.


   // computes the area of the deck rebar mats and their location (cg) measured from the bottom of the deck.
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();
   const CDeckRebarData& rebarData = pDeck->DeckRebarData;

   Float64 tSlab = GetGrossSlabDepth(poi);

   // The reinforcing in this section is the amount of reinforcing within
   // the rebar section width. The rebar section width is the lessor of
   // the effective flange width and the tributary width.
   // Effective flange width can be greater than the tributary flange width
   // when the traffic barrier is structurally continuous and additional
   // width, based on barrier stiffness, is added to the effective flange width
   Float64 Weff = GetEffectiveFlangeWidth(poi);
   Float64 rebarSectionWidth = Weff;

   if ( lrfdVersionMgr::FourthEditionWith2008Interims <= lrfdVersionMgr::GetVersion() )
   {
      Float64 Wtrib = GetTributaryFlangeWidth(poi);
      rebarSectionWidth = Min(Weff,Wtrib);
   }

   Float64 topCover    = rebarData.TopCover;
   Float64 bottomCover = rebarData.BottomCover;

   //
   // full length reinforcement
   //
   const matRebar* pBar;
   Float64 As_Top    = 0;
   Float64 As_Bottom = 0;
   Float64 YbAs_Top    = 0;
   Float64 YbAs_Bottom = 0;

   if ( barCategory == pgsTypes::drcPrimary || barCategory == pgsTypes::drcAll )
   {
      // top mat
      if ( bTopMat )
      {
         if ( (barType == pgsTypes::drbIndividual || barType == pgsTypes::drbAll) && rebarData.TopRebarSize != matRebar::bsNone )
         {
            pBar = lrfdRebarPool::GetInstance()->GetRebar( rebarData.TopRebarType, rebarData.TopRebarGrade, rebarData.TopRebarSize );
            Float64 db = pBar->GetNominalDimension();
            Float64 Yb = tSlab - topCover - db/2;
            IndexType nBars = IsZero(rebarData.TopSpacing) ? 0 : IndexType(rebarSectionWidth/rebarData.TopSpacing) + 1;
            Float64 As = nBars*pBar->GetNominalArea()/rebarSectionWidth;
            YbAs_Top += Yb*As;
            As_Top   += As;
         }

         if ( barType == pgsTypes::drbLumpSum || barType == pgsTypes::drbAll )
         {
            YbAs_Top += rebarData.TopLumpSum*(tSlab - topCover);
            As_Top   += rebarData.TopLumpSum;
         }
      }

      // bottom mat
      if ( bBottomMat )
      {
         if ( (barType == pgsTypes::drbIndividual || barType == pgsTypes::drbAll) && rebarData.BottomRebarSize != matRebar::bsNone )
         {
            pBar = lrfdRebarPool::GetInstance()->GetRebar( rebarData.BottomRebarType, rebarData.BottomRebarGrade, rebarData.BottomRebarSize );
            Float64 db = pBar->GetNominalDimension();
            Float64 Yb = bottomCover + db/2;
            IndexType nBars = IsZero(rebarData.BottomSpacing) ? 0 : IndexType(rebarSectionWidth/rebarData.BottomSpacing) + 1;
            Float64 As = nBars*pBar->GetNominalArea()/rebarSectionWidth;
            YbAs_Bottom += Yb*As;
            As_Bottom   += As;
         }

         if ( barType == pgsTypes::drbLumpSum || barType == pgsTypes::drbAll )
         {
            YbAs_Bottom += bottomCover*rebarData.BottomLumpSum;
            As_Bottom   += rebarData.BottomLumpSum;
         }
      }
   }

   if ( barCategory == pgsTypes::drcSupplemental || barCategory == pgsTypes::drcAll )
   {
      //
      // negative moment reinforcement
      //

      Float64 station, offset;
      GetStationAndOffset(poi,&station,&offset);
      Float64 firstPierStation = GetPierStation(0);
      std::vector<CDeckRebarData::NegMomentRebarData>::const_iterator iter(rebarData.NegMomentRebar.begin());
      std::vector<CDeckRebarData::NegMomentRebarData>::const_iterator end(rebarData.NegMomentRebar.end());
      for ( ; iter != end; iter++ )
      {
         const CDeckRebarData::NegMomentRebarData& nmRebarData = *iter;

         Float64 pierStation = GetPierStation(nmRebarData.PierIdx);
         
         CComPtr<IAngle> objSkewAngle;
         GetPierSkew(nmRebarData.PierIdx,&objSkewAngle);
         Float64 skewAngle;
         objSkewAngle->get_Value(&skewAngle);

         Float64 stationAdjustment = offset*tan(skewAngle);

         Float64 start = pierStation - nmRebarData.LeftCutoff + stationAdjustment;
         Float64 end = start + (nmRebarData.LeftCutoff + nmRebarData.RightCutoff);
         if ( ::InRange(start,station,end) )
         {
            if ( (bTopMat    && (nmRebarData.Mat == CDeckRebarData::TopMat)) ||
                 (bBottomMat && (nmRebarData.Mat == CDeckRebarData::BottomMat)) )
            {
               const CPierData2* pPier = pBridgeDesc->GetPier(nmRebarData.PierIdx);
               bool bIsContinuous = pPier->IsContinuous();
               bool bIsIntegralLeft, bIsIntegralRight;
               pPier->IsIntegral(&bIsIntegralLeft,&bIsIntegralRight);

               if ( bIsContinuous || (bIsIntegralLeft || bIsIntegralRight) )
               {
                  if ( (barType == pgsTypes::drbIndividual || barType == pgsTypes::drbAll) && nmRebarData.RebarSize != matRebar::bsNone )
                  {
                     // Explicit rebar. Reduce area for development if needed.
                     pBar = lrfdRebarPool::GetInstance()->GetRebar( nmRebarData.RebarType, nmRebarData.RebarGrade, nmRebarData.RebarSize);

                     IndexType nBars = IsZero(nmRebarData.Spacing) ? 0 : IndexType(rebarSectionWidth/nmRebarData.Spacing) + 1;

                     Float64 As = nBars*pBar->GetNominalArea()/rebarSectionWidth;
                     Float64 db = pBar->GetNominalDimension();

                     if ( bAdjForDevLength )
                     {
                        // Get development length of bar

                        // remove metric from bar name
                        std::_tstring barst(pBar->GetName());
                        std::size_t sit = barst.find(_T(" "));
                        if (sit != std::_tstring::npos)
                        {
                           barst.erase(sit, barst.size()-1);
                        }

                        matRebar::Size size = lrfdRebarPool::GetBarSize(barst.c_str());
                        REBARDEVLENGTHDETAILS devdet = lrfdRebar::GetRebarDevelopmentLengthDetails(size, pBar->GetNominalArea(), 
                                                                                        pBar->GetNominalDimension(), pBar->GetYieldStrength(), 
                                                                                        (matConcrete::Type)pDeck->Concrete.Type, pDeck->Concrete.Fc, 
                                                                                        pDeck->Concrete.bHasFct, pDeck->Concrete.Fct);
                        Float64 ld = devdet.ldb;

                        Float64 left_bar_length = station - start; // distance from left end of bar to poi
                        Float64 right_bar_length = end - station; // distance from right end of bar to poi
                        Float64 bar_cutoff = Min(left_bar_length,right_bar_length); // cutoff length from POI

                        bar_cutoff = IsZero(bar_cutoff) ? 0 : bar_cutoff;

                        // Reduce As for development if needed
                        if (ld < bar_cutoff)
                        {
                           ATLASSERT(0 < ld);
                           As *= bar_cutoff / ld;
                        }
                     }

                     if (nmRebarData.Mat == CDeckRebarData::TopMat)
                     {
                        Float64 Yb = tSlab - topCover - db/2;
                        YbAs_Top += Yb*As;
                        As_Top += As;
                     }
                     else
                     {
                        Float64 Yb = bottomCover + db/2;
                        YbAs_Bottom += Yb*As;
                        As_Bottom += As;
                     }
                  }

                  // Lump sum bars are not adjusted for development
                  if ( (barType == pgsTypes::drbLumpSum || barType == pgsTypes::drbAll) )
                  {
                     if (nmRebarData.Mat == CDeckRebarData::TopMat)
                     {
                        Float64 Yb = tSlab - topCover;
                        YbAs_Top += Yb*nmRebarData.LumpSum;
                        As_Top   += nmRebarData.LumpSum;
                     }
                     else
                     {
                        YbAs_Bottom += bottomCover*nmRebarData.LumpSum;
                        As_Bottom   += nmRebarData.LumpSum;
                     }
                  }
               }
            }
         }
      }
   }

   Float64 As = As_Bottom + As_Top;
   *pAs = As*rebarSectionWidth;
   *pYb = (IsZero(As) ? 0 : (YbAs_Bottom + YbAs_Top)/As);
}

void CBridgeAgentImp::GetShapeProperties(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 Ecgdr,IShapeProperties** ppShapeProps)
{
   pgsTypes::SectionPropertyType sectPropType = GetSectionPropertiesType();
   GetShapeProperties(sectPropType,intervalIdx,poi,Ecgdr,ppShapeProps);
}

void CBridgeAgentImp::GetShapeProperties(pgsTypes::SectionPropertyType sectPropType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 Ecgdr,IShapeProperties** ppShapeProps)
{
   CComPtr<ISection> s;
   GetSection(intervalIdx,poi,sectPropType,&s);

   // Assuming section is a Composite section and beam is exactly the first piece
   CComQIPtr<ICompositeSectionEx> cmpsection(s);
   CollectionIndexType nItems;
   cmpsection->get_Count(&nItems);

   if ( 0 < nItems )
   {
      CComPtr<ICompositeSectionItemEx> csi;
      cmpsection->get_Item(0,&csi); // this should be the beam

      Float64 Ec;
      csi->get_Efg(&Ec);

      //Update E for the girder
      csi->put_Efg(Ecgdr);

      // change background materials
      for ( CollectionIndexType i = 1; i < nItems; i++ )
      {
         csi.Release();
         cmpsection->get_Item(i,&csi);

         Float64 E;
         csi->get_Ebg(&E);
         if ( IsEqual(Ec,E) )
         {
            csi->put_Ebg(Ecgdr);
         }
      }
   }

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

void CBridgeAgentImp::GetCurbLinePoint(Float64 station, IDirection* direction,DirectionType side,IPoint2d** point)
{
   VALIDATE(BRIDGE);

   HRESULT hr = m_BridgeGeometryTool->CurbLinePoint(m_Bridge,station,direction,side,point);
   ATLASSERT(SUCCEEDED(hr));
}

void CBridgeAgentImp::GetCurbLinePoint(Float64 station, IDirection* direction,DirectionType side,IPoint3d** point)
{
   VALIDATE(BRIDGE);

   CComPtr<IPoint2d> pnt2d;
   GetCurbLinePoint(station,direction,side,&pnt2d);

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

SegmentIndexType CBridgeAgentImp::GetSegmentIndex(const CSplicedGirderData* pGirder,Float64 Xb)
{
   // convert distance from start of bridge to a station so it can be
   // compared with pier and temp support stations
   const CPierData2* pStartPier = pGirder->GetPier(pgsTypes::metStart);
   Float64 station = Xb + pStartPier->GetStation();

   SegmentIndexType nSegments = pGirder->GetSegmentCount();
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
      Float64 startStation,endStation;
      pSegment->GetStations(&startStation,&endStation);

      if ( ::InRange(startStation,station,endStation) )
      {
         return segIdx;
      }
   }

   return INVALID_INDEX;
}

SegmentIndexType CBridgeAgentImp::GetSegmentIndex(const CGirderKey& girderKey,ILine2d* pLine,IPoint2d** ppIntersection)
{
   // Finds where a line intersections a girder line. Returns the index of the segment that is intersected
   // as well as the intersection point
   
   SegmentIndexType nSegments = GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CComPtr<IGirderLine> girderLine;
      GetGirderLine(CSegmentKey(girderKey,segIdx),&girderLine);

      CComPtr<IPath> path;
      girderLine->get_Path(&path);

      CComPtr<IPoint2d> pntNearest;
      girderLine->get_EndPoint(etStart,&pntNearest);

      VARIANT_BOOL bProjectBack  = (segIdx == 0           ? VARIANT_TRUE : VARIANT_FALSE);
      VARIANT_BOOL bProjectAhead = (segIdx == nSegments-1 ? VARIANT_TRUE : VARIANT_FALSE);

      HRESULT hr = path->IntersectEx(pLine,pntNearest,bProjectBack,bProjectAhead,ppIntersection);
      if ( SUCCEEDED(hr) )
      {
         return segIdx;
      }
   }

   (*ppIntersection) = NULL;
   return INVALID_INDEX;
}

SpanIndexType CBridgeAgentImp::GetSpanIndex(Float64 Xb)
{
   if ( Xb < 0 )
   {
      return INVALID_INDEX;
   }

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   Float64 span_station = pBridgeDesc->GetPier(0)->GetStation() + Xb;

   PierIndexType nPiers = pBridgeDesc->GetPierCount();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      if ( span_station < pBridgeDesc->GetPier(pierIdx)->GetStation() )
      {
         return (SpanIndexType)(pierIdx-1);
      }
   }

   return INVALID_INDEX;
}

void CBridgeAgentImp::GetGirderLine(const CSegmentKey& segmentKey,IGirderLine** ppGirderLine)
{
   VALIDATE(BRIDGE);
   CComPtr<ISuperstructureMemberSegment> segment;
   GetSegment(segmentKey,&segment);
   segment->get_GirderLine(ppGirderLine);
}

void CBridgeAgentImp::GetSegmentAtPier(PierIndexType pierIdx,const CGirderKey& girderKey,ISuperstructureMemberSegment** ppSegment)
{
   CSegmentKey segmentKey( GetSegmentAtPier(pierIdx,girderKey) );
   ATLASSERT(segmentKey.groupIndex != INVALID_INDEX && segmentKey.girderIndex != INVALID_INDEX && segmentKey.segmentIndex != INVALID_INDEX);
   GetSegment(segmentKey,ppSegment);
}

void CBridgeAgentImp::GetTemporarySupportLine(SupportIndexType tsIdx,IPierLine** ppPierLine)
{
   VALIDATE(BRIDGE);

   CComPtr<IBridgeGeometry> bridgeGeometry;
   m_Bridge->get_BridgeGeometry(&bridgeGeometry);

   PierIDType pierID = ::GetTempSupportLineID(tsIdx);

   bridgeGeometry->FindPierLine(pierID,ppPierLine);
}

void CBridgeAgentImp::GetPierLine(PierIndexType pierIdx,IPierLine** ppPierLine)
{
   VALIDATE(BRIDGE);

   CComPtr<IBridgeGeometry> bridgeGeometry;
   m_Bridge->get_BridgeGeometry(&bridgeGeometry);

   PierIDType pierID = ::GetPierLineID(pierIdx);

   bridgeGeometry->FindPierLine(pierID,ppPierLine);
}

PierIndexType CBridgeAgentImp::GetGenericBridgePierIndex(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType)
{
   // Returns the index of a pier object in the generic bridge model
   // The pier index can be for a regular pier or a temporary support
   // ONLY USE THIS PIER INDEX FOR ACCESSING PIER OBJECTS IN THE GENERIC BRIDGE MODEL (m_Bridge)
   CComPtr<IGirderLine> girderLine;
   GetGirderLine(segmentKey,&girderLine);

   CComPtr<IPierLine> pierLine;
   if ( endType == pgsTypes::metStart )
   {
      girderLine->get_StartPier(&pierLine);
   }
   else
   {
      girderLine->get_EndPier(&pierLine);
   }

   PierIndexType pierIdx;
   pierLine->get_Index(&pierIdx);
   return pierIdx;
}

void CBridgeAgentImp::GetGenericBridgePier(PierIndexType pierIdx,IBridgePier** ppPier)
{
   // pierIdx is a permanent pier index
   VALIDATE(BRIDGE);
   PierIDType pierID = ::GetPierLineID(pierIdx); // generic bridge pier ID

   CComPtr<IPierCollection> piers;
   m_Bridge->get_Piers(&piers);

   CComPtr<IBridgeGeometry> bridgeGeometry;
   m_Bridge->get_BridgeGeometry(&bridgeGeometry);
   IndexType nPierLines;
   bridgeGeometry->get_PierLineCount(&nPierLines);
   for (IndexType pierLineIdx = 0; pierLineIdx < nPierLines; pierLineIdx++ )
   {
      CComPtr<IPierLine> pierLine;
      bridgeGeometry->GetPierLine(pierLineIdx,&pierLine);
      PierIDType pierLineID;
      pierLine->get_ID(&pierLineID);

      if ( pierID == pierLineID )
      {
         piers->get_Item(pierLineIdx,ppPier);
         return;
      }
   }

   ATLASSERT(false); // should never get here
}

const GirderLibraryEntry* CBridgeAgentImp::GetGirderLibraryEntry(const CGirderKey& girderKey)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);
   return pGirder->GetGirderLibraryEntry();
}

GroupIndexType CBridgeAgentImp::GetGirderGroupAtPier(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace)
{
   // returns the girder group index for the girder group touching the subject pier
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // Get the pier
   const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);

   // Get the span on the side of the pier that goes with the specified pier face
   const CSpanData2* pSpan = NULL;
   if ( pierFace == pgsTypes::Back )
   {
      pSpan = pPier->GetPrevSpan();
   }
   else
   {
      pSpan = pPier->GetNextSpan();
   }

   // get the girder group that contains this span
   const CGirderGroupData* pGroup = NULL;
   if ( pSpan )
   {
      pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   }

   // get the group index
   if ( pGroup )
   {
      return pGroup->GetIndex();
   }
   else
   {
      ATLASSERT(false); // should never get here
      return INVALID_INDEX;
   }
}

void CBridgeAgentImp::CreateTendons(const CBridgeDescription2* pBridgeDesc,const CGirderKey& girderKey,ISuperstructureMember* pSSMbr,ITendonCollection** ppTendons)
{
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);
   const CPTData* pPTData = pGirder->GetPostTensioning();
   DuctIndexType nDucts = pPTData->GetDuctCount();

   const matPsStrand* pStrand = pPTData->pStrand;

   GirderIDType gdrID = pGirder->GetID();

   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();

   CComPtr<ITendonCollection> tendons;
   tendons.CoCreateInstance(CLSID_TendonCollection);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      const CDuctData* pDuctData = pPTData->GetDuct(ductIdx);
      CComPtr<ITendonCollection> webTendons;

      switch (pDuctData->DuctGeometryType)
      {
      case CDuctGeometry::Parabolic:
         CreateParabolicTendon(girderKey,pSSMbr,pDuctData->ParabolicDuctGeometry,&webTendons);
         break;

      case CDuctGeometry::Linear:
         CreateLinearTendon(girderKey,pSSMbr,pDuctData->LinearDuctGeometry,&webTendons);
         break;

      case CDuctGeometry::Offset:
         CreateOffsetTendon(girderKey,pSSMbr,pDuctData->OffsetDuctGeometry,tendons,&webTendons);
         break;
      }

      // the tendons aren't part of the cross section when then are installed because
      // they are not yet grouted. assume the tendons are grouted into the section
      // in the interval that follows the interval when they are installed
      // that is why there is a +1 
      IntervalIndexType stressTendonIntervalIdx = m_IntervalManager.GetStressTendonInterval(girderKey,ductIdx) + 1;

      CComPtr<IPrestressingStrand> tendonMaterial;
      tendonMaterial.CoCreateInstance(CLSID_PrestressingStrand);
      tendonMaterial->put_Name( CComBSTR(pStrand->GetName().c_str()) );
      tendonMaterial->put_Grade((StrandGrade)pStrand->GetGrade());
      tendonMaterial->put_Type((StrandType)pStrand->GetType());
      tendonMaterial->put_Size((StrandSize)pStrand->GetSize());

      tendonMaterial->put_InstallationStage(stressTendonIntervalIdx);

      DuctIndexType nTendons; 
      webTendons->get_Count(&nTendons);
      ATLASSERT(nTendons != 0); // should equal number of webs
      for (DuctIndexType tendonIdx = 0; tendonIdx < nTendons; tendonIdx++ )
      {
         CComPtr<ITendon> tendon;
         webTendons->get_Item(tendonIdx,&tendon);

         tendon->put_OutsideDiameter( pDuctData->pDuctLibEntry->GetOD() );
         tendon->put_InsideDiameter( pDuctData->pDuctLibEntry->GetID() );
         tendon->put_StrandCount(pDuctData->nStrands);
         tendon->putref_Material(tendonMaterial);

         tendon->put_JackingEnd( (JackingEndType)pDuctData->JackingEnd );

#pragma Reminder("UPDATE: need to model duct CG to strand CG offset")
         // need offset in X and Y direction as ducts can be 3D

         tendon->putref_SuperstructureMember(pSSMbr);

         tendons->Add(tendon);
      }
   }

   tendons.CopyTo(ppTendons);
}

void CBridgeAgentImp::CreateParabolicTendon(const CGirderKey& girderKey,ISuperstructureMember* pSSMbr,const CParabolicDuctGeometry& ductGeometry,ITendonCollection** ppTendons)
{
   CComPtr<ITendonCollection> tendons;
   tendons.CoCreateInstance(CLSID_TendonCollection);

   PierIndexType startPierIdx = ductGeometry.GetGirder()->GetPierIndex(pgsTypes::metStart);
   PierIndexType endPierIdx   = ductGeometry.GetGirder()->GetPierIndex(pgsTypes::metEnd);
   SpanIndexType startSpanIdx = (SpanIndexType)startPierIdx;
   SpanIndexType endSpanIdx   = (SpanIndexType)(endPierIdx-1);

   // x = left/right position in the girder cross section
   // y = elevation in the girder cross section (0 is at the top of the girder, positive is up so expect all values to be negative)
   // z = distance along girder (in Girder Coordinates)

   SegmentIndexType nSegments;
   pSSMbr->get_SegmentCount(&nSegments);

   CComPtr<ISuperstructureMemberSegment> segment;
   pSSMbr->get_Segment(0,&segment);

   pgsPointOfInterest poi = GetPointOfInterest(CSegmentKey(girderKey,0),0.0);
   Float64 Xs = poi.GetDistFromStart();
   CComPtr<IShape> shape;
   segment->get_PrimaryShape(Xs,&shape);

   CComQIPtr<IGirderSection> section(shape);
   WebIndexType nWebs;
   section->get_WebCount(&nWebs);

   for ( WebIndexType webIdx = 0; webIdx < nWebs; webIdx++ )
   {
      Float64 Ls = 0;

      CComPtr<ITendon> tendon;
      tendon.CoCreateInstance(CLSID_Tendon);

      tendons->Add(tendon);

      Float64 x1 = 0;
      Float64 z1 = 0;


      //
      // Start to first low point
      //

      // start point
      Float64 dist,offset;
      CDuctGeometry::OffsetType offsetType;
      ductGeometry.GetStartPoint(&dist,&offset,&offsetType);

      z1 += dist;
      Float64 y1 = ConvertDuctOffsetToDuctElevation(girderKey,z1,offset,offsetType);

      // low point
      ductGeometry.GetLowPoint(startSpanIdx,&dist,&offset,&offsetType);
      // distance is measured from the left end of the girder

      if ( dist < 0 ) // fraction of the distance between start and high point at first interior pier
      {
         Float64 L = GetSpanLength(startSpanIdx,girderKey.girderIndex);

         // adjust for the end distance at the start of the girder
         Float64 end_dist = GetSegmentStartEndDistance(CSegmentKey(girderKey.groupIndex,girderKey.girderIndex,0));
         L += end_dist;

         if ( startSpanIdx == endSpanIdx )
         {
            // there is only one span for this tendon... adjust for the end distance at the end of the girder
            end_dist = GetSegmentEndEndDistance(CSegmentKey(girderKey.groupIndex,girderKey.girderIndex,nSegments-1));
            L += end_dist;
         }

         // get the end of girder to low point distance
         dist *= -L;
      }

      Float64 x2 = x1; // dummy (x1 and x2 are computed from the web plane below)
      Float64 z2 = z1 + dist; // this is the location of the low point
      Float64 y2 = ConvertDuctOffsetToDuctElevation(girderKey,z2,offset,offsetType);

      // locate the tendon horizontally in the girder cross section
      CComPtr<IPlane3d> webPlane;
      section->get_WebPlane(webIdx,&webPlane);

      webPlane->GetX(y1,z1,&x1);
      webPlane->GetX(y2,z2,&x2);

      // create a tendon segment
      CComPtr<IParabolicTendonSegment> parabolicTendonSegment;
      parabolicTendonSegment.CoCreateInstance(CLSID_ParabolicTendonSegment);

      CComPtr<IPoint3d> pntStart;
      CComPtr<IPoint3d> pntEnd;

      pntStart.CoCreateInstance(CLSID_Point3d);
      pntStart->Move(x1,y1,z1);

      pntEnd.CoCreateInstance(CLSID_Point3d);
      pntEnd->Move(x2,y2,z2);

      ATLASSERT(z1 < z2);
      parabolicTendonSegment->put_Start(pntStart);
      parabolicTendonSegment->put_End(pntEnd);
      parabolicTendonSegment->put_Slope(0.0);
      parabolicTendonSegment->put_SlopeEnd(qcbRight);
      tendon->AddSegment(parabolicTendonSegment);

      // x1,y1,z1 become coordinates at the low point
      x1 = x2;
      y1 = y2;
      z1 = z2;

      //
      // Low Point to High Point to Low Point
      //
      Float64 x3, y3, z3;
      for ( PierIndexType pierIdx = startPierIdx+1; pierIdx < endPierIdx; pierIdx++ )
      {
         SpanIndexType prevSpanIdx = SpanIndexType(pierIdx-1);
         SpanIndexType nextSpanIdx = prevSpanIdx+1;

         Float64 distLeftIP, highOffset, distRightIP;
         CDuctGeometry::OffsetType highOffsetType;
         ductGeometry.GetHighPoint(pierIdx,&distLeftIP,&highOffset,&highOffsetType,&distRightIP);

         // low to inflection point
         Float64 L = GetSpanLength(prevSpanIdx,girderKey.girderIndex);
         if ( prevSpanIdx == startSpanIdx )
         {
            // adjust for the end distance at the start of the girder
            Float64 end_dist = GetSegmentStartEndDistance(CSegmentKey(girderKey.groupIndex,girderKey.girderIndex,0));
            L += end_dist;
         }
         Ls += L;

         x3 = x2; // dummy
         z3 = Ls;
         y3 = ConvertDuctOffsetToDuctElevation(girderKey,z3,highOffset,highOffsetType);

         if ( distLeftIP < 0 ) // fraction of distance between low and high point
         {
            distLeftIP *= -(z3-z1);
         }

         z2 = z3 - distLeftIP; // inflection point measured from high point... want z2 measured from start of span

         // elevation at inflection point (slope at low and high points must be zero)
         Float64 y2 = (y3*(z2-z1) + y1*(z3-z2))/(z3-z1);

         webPlane->GetX(y2,z2,&x2);
         webPlane->GetX(y3,z3,&x3);

         CComPtr<IParabolicTendonSegment> leftParabolicTendonSegment;
         leftParabolicTendonSegment.CoCreateInstance(CLSID_ParabolicTendonSegment);

         CComPtr<IPoint3d> pntStart;
         pntStart.CoCreateInstance(CLSID_Point3d);
         pntStart->Move(x1,y1,z1);

         CComPtr<IPoint3d> pntInflection;
         pntInflection.CoCreateInstance(CLSID_Point3d);
         pntInflection->Move(x2,y2,z2);

         ATLASSERT(z1 < z2);
         leftParabolicTendonSegment->put_Start(pntStart);
         leftParabolicTendonSegment->put_End(pntInflection);
         leftParabolicTendonSegment->put_Slope(0.0);
         leftParabolicTendonSegment->put_SlopeEnd(qcbLeft);
         tendon->AddSegment(leftParabolicTendonSegment);

         // inflection to high point
         CComPtr<IParabolicTendonSegment> rightParabolicTendonSegment;
         rightParabolicTendonSegment.CoCreateInstance(CLSID_ParabolicTendonSegment);

         CComPtr<IPoint3d> pntEnd;
         pntEnd.CoCreateInstance(CLSID_Point3d);
         pntEnd->Move(x3,y3,z3);

         ATLASSERT(z2 < z3);
         rightParabolicTendonSegment->put_Start(pntInflection);
         rightParabolicTendonSegment->put_End(pntEnd);
         rightParabolicTendonSegment->put_Slope(0.0);
         rightParabolicTendonSegment->put_SlopeEnd(qcbRight);
         tendon->AddSegment(rightParabolicTendonSegment);

         // high to inflection point
         ductGeometry.GetLowPoint(pierIdx,&dist,&offset,&offsetType);
         x1 = x3;
         z1 = z3; 
         y1 = y3;

         L = GetSpanLength(nextSpanIdx,girderKey.girderIndex);
         if ( nextSpanIdx == endSpanIdx )
         {
            // adjust for the end distance at the end of the girder
            Float64 end_dist = GetSegmentEndEndDistance(CSegmentKey(girderKey.groupIndex,girderKey.girderIndex,nSegments-1));
            L += end_dist;
         }

         if ( dist < 0 ) // fraction of span length
         {
            dist *= -L;
         }

         if ( nextSpanIdx == endSpanIdx )
         {
            // for the last span, low point is measured from right end... make it 
            // measured from the left end
            dist = L - dist;
         }

         z3 = z1 + dist; // low point, measured from previous high point
         y3 = ConvertDuctOffsetToDuctElevation(girderKey,z3,offset,offsetType);

         if ( distRightIP < 0 ) // fraction of distance between high and low point
         {
            distRightIP *= -(z3 - z1);
         }

         z2 = z1 + distRightIP; // inflection point measured from high point
         y2 = (y3*(z2-z1) + y1*(z3-z2))/(z3-z1);

         webPlane->GetX(y2,z2,&x2);
         webPlane->GetX(y3,z3,&x3);

         leftParabolicTendonSegment.Release();
         leftParabolicTendonSegment.CoCreateInstance(CLSID_ParabolicTendonSegment);

         pntStart.Release();
         pntStart.CoCreateInstance(CLSID_Point3d);
         pntStart->Move(x1,y1,z1);

         pntInflection.Release();
         pntInflection.CoCreateInstance(CLSID_Point3d);
         pntInflection->Move(x2,y2,z2);

         ATLASSERT(z1 < z2);
         leftParabolicTendonSegment->put_Start(pntStart);
         leftParabolicTendonSegment->put_End(pntInflection);
         leftParabolicTendonSegment->put_Slope(0.0);
         leftParabolicTendonSegment->put_SlopeEnd(qcbLeft);
         tendon->AddSegment(leftParabolicTendonSegment);

         // inflection point to low point
         rightParabolicTendonSegment.Release();
         rightParabolicTendonSegment.CoCreateInstance(CLSID_ParabolicTendonSegment);

         pntEnd.Release();
         pntEnd.CoCreateInstance(CLSID_Point3d);
         pntEnd->Move(x3,y3,z3);

         rightParabolicTendonSegment->put_Start(pntInflection);
         rightParabolicTendonSegment->put_End(pntEnd);
         rightParabolicTendonSegment->put_Slope(0.0);
         rightParabolicTendonSegment->put_SlopeEnd(qcbRight);
         tendon->AddSegment(rightParabolicTendonSegment);

         z1 = z3;
         y1 = y3;
         x1 = x3;
      }

      //
      // last low point to end
      //

      // distance is measured from the previous high point
      ductGeometry.GetEndPoint(&dist,&offset,&offsetType);
      if ( dist < 0 ) // fraction of last span length
      {
         // get the cl-brg to cl-brg length for this girder in the end span
         Float64 L = GetSpanLength(endSpanIdx,girderKey.girderIndex);

         // adjust for the end distance at the end of the girder
         Float64 end_dist = GetSegmentEndEndDistance(CSegmentKey(girderKey.groupIndex,girderKey.girderIndex,nSegments-1));
         L += end_dist;

         if ( startSpanIdx == endSpanIdx )
         {
            // this is only one span for this tendon... adjust for end distance at the start of the girder
            end_dist = GetSegmentEndEndDistance(CSegmentKey(girderKey.groupIndex,girderKey.girderIndex,0));
            L += end_dist;
         }

         dist *= -L;
      }

      Float64 Lg = GetGirderLength(girderKey); // End-to-end length of full girder

      x2 = x1; // dummy
      z2 = Lg - dist; // dist is measured from the end of the bridge
      y2 = ConvertDuctOffsetToDuctElevation(girderKey,z2,offset,offsetType);

      webPlane->GetX(y1,z1,&x1);
      webPlane->GetX(y2,z2,&x2);

      parabolicTendonSegment.Release();
      parabolicTendonSegment.CoCreateInstance(CLSID_ParabolicTendonSegment);

      pntStart.Release();
      pntStart.CoCreateInstance(CLSID_Point3d);
      pntStart->Move(x1,y1,z1);

      pntEnd.Release();
      pntEnd.CoCreateInstance(CLSID_Point3d);
      pntEnd->Move(x2,y2,z2);

      ATLASSERT(z1 < z2);
      parabolicTendonSegment->put_Start(pntStart);
      parabolicTendonSegment->put_End(pntEnd);
      parabolicTendonSegment->put_Slope(0.0);
      parabolicTendonSegment->put_SlopeEnd(qcbLeft);
      tendon->AddSegment(parabolicTendonSegment);
   }

   tendons.CopyTo(ppTendons);
}

void CBridgeAgentImp::CreateLinearTendon(const CGirderKey& girderKey,ISuperstructureMember* pSSMbr,const CLinearDuctGeometry& ductGeometry,ITendonCollection** ppTendons)
{
   CComPtr<ITendonCollection> tendons;
   tendons.CoCreateInstance(CLSID_TendonCollection);

   CComPtr<ISuperstructureMemberSegment> segment;
   pSSMbr->get_Segment(0,&segment);

   CComPtr<IShape> shape;
   segment->get_PrimaryShape(0,&shape);

   Float64 Lg = GetGirderLength(girderKey);

   CComQIPtr<IGirderSection> section(shape);
   WebIndexType nWebs;
   section->get_WebCount(&nWebs);

   for ( WebIndexType webIdx = 0; webIdx < nWebs; webIdx++ )
   {
      CComPtr<ITendon> tendon;
      tendon.CoCreateInstance(CLSID_Tendon);

      tendons->Add(tendon);

      Float64 xStart = 0; // horizontal position in Girder Section Coordinates (distance from CL girder)
      Float64 zStart = 0; // distance along girder (Girder Coordinate System... same as Xg)
      Float64 offset;
      CDuctGeometry::OffsetType offsetType;
      ductGeometry.GetPoint(0,&zStart,&offset,&offsetType);
      ATLASSERT(IsZero(zStart)); // must always be zero!

      // vertical position in Girder Section Coordinates (distance from top of girder)
      Float64 yStart = ConvertDuctOffsetToDuctElevation(girderKey,zStart,offset,offsetType);

      // get the location of the web plane in Girder Section Coordinates
      // this is the horizontal position of the web plane measured from the CL of girder
      CComPtr<IPlane3d> webPlane;
      section->get_WebPlane(webIdx,&webPlane);

      webPlane->GetX(yStart,zStart,&xStart);

      CLinearDuctGeometry::MeasurementType measurementType = ductGeometry.GetMeasurementType();

      CComPtr<ILinearTendonSegment> prevSegment;

      CollectionIndexType nPoints = ductGeometry.GetPointCount();
      ATLASSERT( 2 <= nPoints );
      for ( CollectionIndexType pointIdx = 1; pointIdx < nPoints; pointIdx++ )
      {
         Float64 location;
         ductGeometry.GetPoint(pointIdx,&location,&offset,&offsetType);

         // coordinates at end of girder
         Float64 xEnd = xStart; // assume web is constant distance away from CL Girder
         Float64 zEnd; // Xg at end. see below
         if ( measurementType == CLinearDuctGeometry::AlongGirder )
         {
            if ( location < 0 )
            {
               location *= -Lg;
            }

            zEnd = location;
         }
         else
         {
            // location is distance from previous point
            zEnd = zStart + location;
         }

         Float64 yEnd = ConvertDuctOffsetToDuctElevation(girderKey,zEnd,offset,offsetType);

         CComPtr<IPoint3d> pntStart;
         pntStart.CoCreateInstance(CLSID_Point3d);
         pntStart->Move(xStart,yStart,zStart);

         CComPtr<IPoint3d> pntEnd;
         pntEnd.CoCreateInstance(CLSID_Point3d);
         pntEnd->Move(xEnd,yEnd,zEnd);

         CComPtr<ILinearTendonSegment> linearTendonSegment;
         linearTendonSegment.CoCreateInstance(CLSID_LinearTendonSegment);
         linearTendonSegment->put_Start(pntStart);
         linearTendonSegment->put_End(pntEnd);

         CComQIPtr<ITendonSegment> tendonSegment(linearTendonSegment);
         tendon->AddSegment(tendonSegment);

         linearTendonSegment->putref_PrevTendonSegment(prevSegment);
         if ( prevSegment )
         {
            prevSegment->putref_NextTendonSegment(tendonSegment);
         }

         // start of next segment is end of this segment
         xStart = xEnd;
         yStart = yEnd;
         zStart = zEnd;

         prevSegment = tendonSegment;
      }
   }

   tendons.CopyTo(ppTendons);
}

void CBridgeAgentImp::CreateOffsetTendon(const CGirderKey& girderKey,ISuperstructureMember* pSSMbr,const COffsetDuctGeometry& ductGeometry,ITendonCollection* refTendons,ITendonCollection** ppTendons)
{
   CComPtr<ITendonCollection> tendons;
   tendons.CoCreateInstance(CLSID_TendonCollection);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData*    pGroup      = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
   const CSplicedGirderData*  pGirder     = pGroup->GetGirder(girderKey.girderIndex);
   const CPTData*             pPTData     = pGirder->GetPostTensioning();

   CComPtr<ISuperstructureMemberSegment> segment;
   pSSMbr->get_Segment(0,&segment);

   CComPtr<IShape> shape;
   segment->get_PrimaryShape(0,&shape);

   CComQIPtr<IGirderSection> section(shape);
   WebIndexType nWebs;
   section->get_WebCount(&nWebs);

   for ( WebIndexType webIdx = 0; webIdx < nWebs; webIdx++ )
   {
      CComPtr<IOffsetTendon> offsetTendon;
      offsetTendon.CoCreateInstance(CLSID_OffsetTendon);

      CComPtr<IPlane3d> webPlane;
      section->get_WebPlane(webIdx,&webPlane);

      tendons->Add(offsetTendon);

      CComPtr<ITendon> refTendon;
      refTendons->get_Item(ductGeometry.RefDuctIdx+webIdx,&refTendon);
      offsetTendon->putref_RefTendon(refTendon);

      Float64 z = 0;
      std::vector<COffsetDuctGeometry::Point>::const_iterator iter( ductGeometry.Points.begin() );
      std::vector<COffsetDuctGeometry::Point>::const_iterator iterEnd( ductGeometry.Points.end() );
      for ( ; iter != iterEnd; iter++ )
      {
         const COffsetDuctGeometry::Point& point = *iter;
         z += point.distance;

         CComPtr<IPoint3d> refPoint;
         refTendon->get_CG(z,tmPath,&refPoint);

         Float64 x;
         Float64 y;
         refPoint->get_X(&x);
         refPoint->get_Y(&y);
         y += point.offset;

         Float64 x_offset;
         webPlane->GetX(y,z,&x_offset);
         offsetTendon->AddOffset(z,x_offset-x,point.offset);
      }
   }

   tendons.CopyTo(ppTendons);
}

void CBridgeAgentImp::CreateCompositeOverlayEdgePaths(const CBridgeDescription2* pBridgeDesc,IPath** ppLeftPath,IPath** ppRightPath)
{
   CComPtr<IPath> left_path, right_path;
   left_path.CoCreateInstance(CLSID_Path);
   right_path.CoCreateInstance(CLSID_Path);

   // Build  path along exterior girders
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType nGirders = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();
      SegmentIndexType nSegments = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirder(0)->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CComPtr<IPoint2d> point_on_edge;

         // left edge - start of segment
         NoDeckEdgePoint(grpIdx,segIdx,pgsTypes::metStart,qcbLeft,&point_on_edge);
         left_path->AddEx(point_on_edge);

         // left edge - end of segment
         point_on_edge.Release();
         NoDeckEdgePoint(grpIdx,segIdx,pgsTypes::metEnd,qcbLeft,&point_on_edge);
         left_path->AddEx(point_on_edge);

         // right edge - start of segment
         point_on_edge.Release();
         NoDeckEdgePoint(grpIdx,segIdx,pgsTypes::metStart,qcbRight,&point_on_edge);
         right_path->AddEx(point_on_edge);

         // right edge - end of segment
         point_on_edge.Release();
         NoDeckEdgePoint(grpIdx,segIdx,pgsTypes::metEnd,qcbRight,&point_on_edge);
         right_path->AddEx(point_on_edge);
      } // next segment
   } // next group

   left_path.CopyTo(ppLeftPath);
   right_path.CopyTo(ppRightPath);
}

/// Strand filler-related functions
CContinuousStrandFiller* CBridgeAgentImp::GetContinuousStrandFiller(const CSegmentKey& segmentKey)
{
   StrandFillerCollection::iterator its = m_StrandFillers.find(segmentKey);
   if (its != m_StrandFillers.end())
   {
      CStrandFiller& pcfill = its->second;
      CContinuousStrandFiller* pfiller = dynamic_cast<CContinuousStrandFiller*>(&(pcfill));
      return pfiller;
   }
   else
   {
      ATLASSERT(false); // This will go badly. Filler should have been created already
      return NULL;
   }
}

CDirectStrandFiller* CBridgeAgentImp::GetDirectStrandFiller(const CSegmentKey& segmentKey)
{
   StrandFillerCollection::iterator its = m_StrandFillers.find(segmentKey);
   if (its != m_StrandFillers.end())
   {
      CStrandFiller& pcfill = its->second;
      CDirectStrandFiller* pfiller = dynamic_cast<CDirectStrandFiller*>(&(pcfill));
      return pfiller;
   }
   else
   {
      ATLASSERT(false); // This will go badly. Filler should have been created already
      return NULL;
   }
}

void CBridgeAgentImp::InitializeStrandFiller(const GirderLibraryEntry* pGirderEntry, const CSegmentKey& segmentKey)
{
   StrandFillerCollection::iterator its = m_StrandFillers.find(segmentKey);
   if (its != m_StrandFillers.end())
   {
      its->second.Init(pGirderEntry);
   }
   else
   {
      CStrandFiller filler;
      filler.Init(pGirderEntry);
      std::pair<StrandFillerCollection::iterator, bool>  sit = m_StrandFillers.insert(std::make_pair(segmentKey,filler));
      ATLASSERT(sit.second);
   }
}

void CBridgeAgentImp::CreateStrandMover(LPCTSTR strGirderName,pgsTypes::AdjustableStrandType adjType,IStrandMover** ppStrandMover)
{
   GET_IFACE(ILibrary,pLib);
   const GirderLibraryEntry* pGirderEntry = pLib->GetGirderEntry(strGirderName);

   // get adjustable strand adjustment limits
   pgsTypes::FaceType endTopFace, endBottomFace;
   Float64 endTopLimit, endBottomLimit;
   IBeamFactory::BeamFace etf, ebf;
   IBeamFactory::BeamFace htf, hbf;
   pgsTypes::FaceType hpTopFace, hpBottomFace;
   Float64 hpTopLimit, hpBottomLimit;
   Float64 end_increment, hp_increment;

   if (adjType == pgsTypes::asHarped)
   {
      pGirderEntry->GetEndAdjustmentLimits(&endTopFace, &endTopLimit, &endBottomFace, &endBottomLimit);

      etf = endTopFace    == pgsTypes::BottomFace ? IBeamFactory::BeamBottom : IBeamFactory::BeamTop;
      ebf = endBottomFace == pgsTypes::BottomFace ? IBeamFactory::BeamBottom : IBeamFactory::BeamTop;

      pGirderEntry->GetHPAdjustmentLimits(&hpTopFace, &hpTopLimit, &hpBottomFace, &hpBottomLimit);

      htf = hpTopFace    == pgsTypes::BottomFace ? IBeamFactory::BeamBottom : IBeamFactory::BeamTop;
      hbf = hpBottomFace == pgsTypes::BottomFace ? IBeamFactory::BeamBottom : IBeamFactory::BeamTop;

      // only allow end adjustents if increment is non-negative
      end_increment = pGirderEntry->IsVerticalAdjustmentAllowedEnd() ?  pGirderEntry->GetEndStrandIncrement() : -1.0;
      hp_increment  = pGirderEntry->IsVerticalAdjustmentAllowedHP()  ?  pGirderEntry->GetHPStrandIncrement()  : -1.0;
   }
   else
   {
      ATLASSERT(adjType == pgsTypes::asStraight); // Should be only other option

      // Straight strands - ends and harp points are the same
      pGirderEntry->GetStraightAdjustmentLimits(&endTopFace, &endTopLimit, &endBottomFace, &endBottomLimit);

      hpTopLimit = endTopLimit;
      hpBottomLimit = endBottomLimit;

      etf = endTopFace    == pgsTypes::BottomFace ? IBeamFactory::BeamBottom : IBeamFactory::BeamTop;
      ebf = endBottomFace == pgsTypes::BottomFace ? IBeamFactory::BeamBottom : IBeamFactory::BeamTop;

      htf = etf;
      hbf = ebf;

      // only allow end adjustents if increment is non-negative
      end_increment = pGirderEntry->IsVerticalAdjustmentAllowedStraight() ?  pGirderEntry->GetStraightStrandIncrement() : -1.0;
      hp_increment  = end_increment;
   }

   // create the strand mover
   CComPtr<IBeamFactory> beamFactory;
   pGirderEntry->GetBeamFactory(&beamFactory);
   beamFactory->CreateStrandMover(pGirderEntry->GetDimensions(), 
                                  etf, endTopLimit, ebf, endBottomLimit, 
                                  htf, hpTopLimit,  hbf, hpBottomLimit, 
                                  end_increment, hp_increment, ppStrandMover);

   ATLASSERT(*ppStrandMover != NULL);
}

Float64 CBridgeAgentImp::GetRelaxation(Float64 fpi,const matPsStrand* pStrand,Float64 tStart,Float64 tEnd,Float64 tStress)
{
   GET_IFACE(ILossParameters,pLossParameters);
#if defined _DEBUG
   ATLASSERT(pLossParameters->GetLossMethod() == pgsTypes::TIME_STEP);
#endif

   matPsStrand::Type strandType = pStrand->GetType();
   Float64 fpy = pStrand->GetYieldStrength();
   Float64 fpu = pStrand->GetUltimateStrength();

   // This method computes the incremental strand relaxation during the time period tStart-tEnd
   // with the stress in the strand fpi at the beginning of the interval

   Float64 fr = 0;
   pgsTypes::TimeDependentModel model = pLossParameters->GetTimeDependentModel();
   if ( model == pgsTypes::tdmACI209 || model == pgsTypes::tdmAASHTO )
   {
      Float64 initial_stress_ratio = fpi/fpy;
      if ( tStart-tStress < 1./24. )
      {
         // don't want log10 to be less than zero (see "Recommendations for Estimating Prestress Losses",
         // PCI Journal July-August 1975, pg 51)
         tStart = tStress + 1./24.; 
      }

      if ( tEnd-tStress < 1./24. )
      {
         // don't want log10 to be less than zero (see "Recommendations for Estimating Prestress Losses",
         // PCI Journal July-August 1975, pg 51)
         tEnd = tStress + 1./24.; 
      }

      if ( initial_stress_ratio <= 0.55 )
      {
         fr = 0;
      }
      else
      {
         Float64 K = (strandType == matPsStrand::LowRelaxation ? 45 : 10);
         fr = (fpi/K)*(initial_stress_ratio - 0.55)*(log10(24*(tEnd-tStress)) - log10(24*(tStart-tStress))); // t is in days
      }
   }
   else if ( model == pgsTypes::tdmCEBFIP )
   {
      Float64 initial_stress_ratio = fpi/fpu; // See CEB-FIP Figure 2.3.3
      Float64 p; // relaxation ratio
      Float64 k;
      if ( strandType == matPsStrand::StressRelieved )
      {
         // curve fit for Class 1 curve from CEB-FIP Figure 2.3.3
         p = 0.4*initial_stress_ratio + 0.2;
         k = 0.12;
      }
      else
      {
         // curve fit for Class 2 curve from CEB-FIP Figure 2.3.3
         p = initial_stress_ratio*initial_stress_ratio - 1.2*initial_stress_ratio + 0.37;
         k = 0.19;
      }

      // Assume ultimate relaxation to occur at 50 years
      // CEB-FIP 2.3.4.5, relxation at 50 years = 3 times relaxation at day 1000
      // Relaxation rate = 1/3 1000 hour relaxation
      p /= 3;

      Float64 t1 = tStart - tStress;
      Float64 t2 = tEnd   - tStress;
      fr = fpi*p*(pow(24*t2/1000,k) - pow(24*t1/1000,k)); // Equation in CEB-FEB commentary for 2.3.4.5
   }
   else
   {
      ATLASSERT(false); // is there a new type?
   }

   if ( pStrand->GetCoating() != matPsStrand::None )
   {
      // strand is epoxy coated... double the relaxation
      // see PCI "Guidelines for the use of Epoxy-Coated Strand", PCI Journal, July-August 1993
      fr *= 2;
   }

   return fr;
}

bool CBridgeAgentImp::GirderLineIntersect(const CGirderKey& girderKey,ILine2d* pLine,SegmentIndexType segIdxHint,SegmentIndexType* pSegIdx,Float64* pXs)
{
   ASSERT_GIRDER_KEY(girderKey);
   // First try the hint... if it is right, we are done fast
   CSegmentKey segmentKey(girderKey,segIdxHint);
   if ( SegmentLineIntersect(segmentKey,pLine,pXs) )
   {
      *pSegIdx = segIdxHint;
      return true;
   }

   // work our way out from the hint segment
   // Let's say there are 10 segments and the hint is for segment index = 6
   // we want to try set 5, 7, 4, 8, 3, 9, 2, 1, 0... don't want to try 6 because
   // we already did... working out from the hint alternating sides
   SegmentIndexType nSegments = GetSegmentCount(girderKey);
   std::vector<SegmentIndexType> segments;
   segments.reserve(nSegments-1);

   SegmentIndexType e1 = segIdxHint;
   SegmentIndexType e2 = nSegments-1-segIdxHint;
   SegmentIndexType end = segIdxHint == 0 ? nSegments-1 : Max(e1,e2);
   for (SegmentIndexType i = 1; i <= end; i++ )
   {
      if ( i <= e1 )
      {
         segments.push_back(segIdxHint-i);
      }

      if ( i <= e2 )
      {
         segments.push_back(segIdxHint+i);
      }
   }

   BOOST_FOREACH(SegmentIndexType segIdx,segments)
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      if ( SegmentLineIntersect(segmentKey,pLine,pXs) )
      {
         *pSegIdx = segIdx;
         return true;
      }
   }

   return false;
}

bool CBridgeAgentImp::SegmentLineIntersect(const CSegmentKey& segmentKey,ILine2d* pLine,Float64* pXs)
{
   ASSERT_SEGMENT_KEY(segmentKey);

   VALIDATE(BRIDGE);
   CComPtr<IGirderLine> girderLine;
   GetGirderLine(segmentKey,&girderLine);

   CComPtr<IPath> path;
   girderLine->get_Path(&path);

   CComPtr<IPoint2d> pntStartOfSegment;
   girderLine->get_EndPoint(etStart,&pntStartOfSegment);

   CComPtr<IPoint2d> pnt;
   HRESULT hr = path->IntersectEx(pLine,pntStartOfSegment,VARIANT_FALSE,VARIANT_FALSE,&pnt);
   if ( FAILED(hr) )
   {
      ATLASSERT(hr == COGO_E_NOINTERSECTION);
      return false;
   }

   pntStartOfSegment->DistanceEx(pnt,pXs);

   return true;
}

void CBridgeAgentImp::ComputeHpFill(const GirderLibraryEntry* pGdrEntry,IStrandGridFiller* pStrandGridFiller, IIndexArray* pFill, IIndexArray** ppHPfill)
{
   // Fill for harped strands at harping points can be different than at girder ends
   // if the odd number of strands option is activated
   if ( pGdrEntry->OddNumberOfHarpedStrands() )
   {
      StrandIndexType nStrands;
      pStrandGridFiller->GetStrandCountEx(pFill, &nStrands);

      if (1 < nStrands && nStrands%2 != 0) // if there is more than 1 harped strand and the number of strands is odd
      {
         // we allow, and have, an odd number of strands.

         // we are in business, start alternate fill of hp grid
         CComPtr<IIndexArray> oddHpFill;
         oddHpFill.CoCreateInstance(CLSID_IndexArray);
         CollectionIndexType fill_size;
         pFill->get_Count(&fill_size);
         oddHpFill->Reserve(fill_size);

         // put two strands in the first hp location
#if defined _DEBUG
         IndexType first_row;
         pFill->get_Item(0,&first_row);
         ASSERT(first_row == 1); // only one strand at the bottom... but we need it to be 2 for odd fill at top
#endif

         StrandIndexType running_cnt = (pGdrEntry->IsDifferentHarpedGridAtEndsUsed() ? 2 : 1);
         oddHpFill->Add(running_cnt); 

         for (CollectionIndexType is = 1; is < fill_size; is++)
         {
            if (running_cnt < nStrands)
            {
               // there are still strands to fill

               StrandIndexType fill_val;
               pFill->get_Item(is, &fill_val);
               
               running_cnt += fill_val;
               
               if (running_cnt <= nStrands)
               {
                  // not at the end yet, just fill it up
                  oddHpFill->Add(fill_val);
               }
               else
               {
                  // we are at the end... add the odd strand
                  if ( pGdrEntry->IsDifferentHarpedGridAtEndsUsed() )
                  {
                     oddHpFill->Add(fill_val-1);
                  }
                  else
                  {
                     oddHpFill->Add(fill_val);
                  }

                  running_cnt--;
               }
            }
            else
            {
               oddHpFill->Add(0);
            }
         }

         // Return with modified grid
         ATLASSERT(running_cnt==nStrands);
         oddHpFill.CopyTo(ppHPfill);
         return;
      }
   }

   // if we get to here, hp grid is same as end
   *ppHPfill = pFill;
   (*ppHPfill)->AddRef();
}

Float64 CBridgeAgentImp::ComputePierDiaphragmHeight(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace)
{
   // Compute Pier Diaphragm Height as Hgirder + "A" Dimension (slab offset) - tSlab
   // This makes the height of the diaphragm go from the bottom of the girder to the bottom of the slab
   // In the future, we will want to include bearing heights and bearing recess dimensions

   GroupIndexType groupIndicies[2];
   GetGirderGroupIndex(pierIdx,&groupIndicies[0],&groupIndicies[1]);

   // Set the diaphram height equal to the greatest computed height
   Float64 H = 0;
   for ( int i = 0; i < 2; i++ )
   {
      GroupIndexType grpIdx = groupIndicies[i];
      if ( grpIdx == INVALID_INDEX )
      {
         continue;
      }

      GirderIndexType nGirders = GetGirderCount(grpIdx);
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         pgsPointOfInterest poi = GetPierPointOfInterest(CGirderKey(grpIdx,gdrIdx),pierIdx);
         const CSegmentKey& segmentKey(poi.GetSegmentKey());
         IntervalIndexType releaseIntervalIdx = GetPrestressReleaseInterval(segmentKey);

         PoiAttributeType attrib = (i == 0 ? POI_10L : POI_0L);
         std::vector<pgsPointOfInterest> vPoi = GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | attrib,POIFIND_AND);
         ATLASSERT(vPoi.size() == 1);

         Float64 tSlab = GetGrossSlabDepth(poi);
         Float64 Hg = GetHg(releaseIntervalIdx,vPoi.front());
         Float64 A = GetSlabOffset(grpIdx,pierIdx,gdrIdx);
         Float64 h = Hg + A - tSlab;
         H = Max(H,h);
      } // next girder
   } // next group

   return H;
}

Float64 CBridgeAgentImp::ComputePierDiaphragmWidth(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);

   // Compute the maximum bearing offset... this is used by many of the geometry configurations
   GroupIndexType backGrpIdx, aheadGrpIdx;
   GetGirderGroupIndex(pierIdx,&backGrpIdx,&aheadGrpIdx);

   GroupIndexType grpIdx = (pierFace == pgsTypes::Back ? backGrpIdx : aheadGrpIdx);
   ATLASSERT(grpIdx != INVALID_INDEX);

   Float64 Wsupport; // support width measured along CL girder... need to measure it normal
   // to CL Pier. See below for adjusting bearing offset. The same logic is used for adjusting the bearing offset
   // and the support width.
   if ( pPier->IsBoundaryPier() )
   {
      Wsupport = pPier->GetSupportWidth(pierFace);
   }
   else
   {
      ATLASSERT(pPier->IsInteriorPier());
      Wsupport = pPier->GetSupportWidth(pgsTypes::Ahead);
   }

   Float64 maxBrgOffset = 0;
   Float64 maxSupportWidth = 0;
   GirderIndexType nGirders = GetGirderCount(grpIdx);
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      // get segment bearing offset
      CSegmentKey segmentKey = GetSegmentAtPier(pierIdx,CGirderKey(grpIdx, gdrIdx));
      Float64 brgOffset;


      // segment bearing offset is measured along the CL girder.
      // we need it to be measured normal to the CL pier
      // get the angle between the pier and the girder, then deduct 90 degrees
      // so that it is the angle between the pier normal and the girder.
      // multiple the bearing offset by this angle to get the bearing offset
      // normal to the pier.
      // Since the girders need not be parallel, each girder can have a different intersection
      // angle with the pier. For this reason, we have to compute the bearing offset and
      // support width normal to the pier uniquely for each girder.
      CComPtr<IAngle> objSegAngle;
      Float64 segAngle;
      if ( pierFace == pgsTypes::Back)
      {
         brgOffset = GetSegmentEndBearingOffset(segmentKey);
         GetSegmentAngle(segmentKey,pgsTypes::metEnd,&objSegAngle);
         objSegAngle->get_Value(&segAngle);
         segAngle -= PI_OVER_2;
      }
      else
      {
         brgOffset = GetSegmentStartBearingOffset(segmentKey);
         GetSegmentAngle(segmentKey,pgsTypes::metStart,&objSegAngle);
         objSegAngle->get_Value(&segAngle);
         segAngle -= PI_OVER_2;
      }

      brgOffset *= cos(segAngle);
      maxBrgOffset = Max(maxBrgOffset,brgOffset);
      maxSupportWidth = Max(maxSupportWidth,Wsupport*cos(segAngle));
   }

   Float64 W = 0;
   if ( pPier->IsBoundaryPier() )
   {
      pgsTypes::BoundaryConditionType bcType = pPier->GetBoundaryConditionType();
      if ( bcType == pgsTypes::bctHinge || bcType == pgsTypes::bctRoller )
      {
         W = maxSupportWidth;
      }
      else if ( bcType == pgsTypes::bctContinuousBeforeDeck || bcType == pgsTypes::bctContinuousAfterDeck )
      {
         W = maxBrgOffset - maxSupportWidth/2;
      }
      else if ( bcType == pgsTypes::bctIntegralBeforeDeck || bcType == pgsTypes::bctIntegralAfterDeck )
      {
         W = maxBrgOffset + maxSupportWidth/2;

         if ( pPier->GetPierModelType() == pgsTypes::pmtPhysical )
         {
            // The segment/pier connection is integral and there is a physical model of the pier.
            // Use the lower cross beam width as the minimum width of the pier diaphragm
            W = Max(W,pPier->GetXBeamWidth()/2);
         }
      }
      else if ( bcType == pgsTypes::bctIntegralBeforeDeckHingeBack || bcType == pgsTypes::bctIntegralAfterDeckHingeBack )
      {
         if ( pierFace == pgsTypes::Back )
         {
            W = maxSupportWidth;
         }
         else
         {
            W = maxBrgOffset + maxSupportWidth/2;
            if ( pPier->GetPierModelType() == pgsTypes::pmtPhysical )
            {
               W = Max(W,pPier->GetXBeamWidth()/2);
            }
         }
      }
      else if ( bcType == pgsTypes::bctIntegralBeforeDeckHingeAhead || bcType == pgsTypes::bctIntegralAfterDeckHingeAhead )
      {
         if ( pierFace == pgsTypes::Back )
         {
            W = maxBrgOffset + maxSupportWidth/2;
            if ( pPier->GetPierModelType() == pgsTypes::pmtPhysical )
            {
               W = Max(W,pPier->GetXBeamWidth()/2);
            }
         }
         else
         {
            W = maxSupportWidth;
         }
      }
      else
      {
         ATLASSERT(false); // Is there a new type?
         W = 0;
      }
   }
   else
   {
      ATLASSERT(pPier->IsInteriorPier());
      pgsTypes::PierSegmentConnectionType connectionType = pPier->GetSegmentConnectionType();

      if ( connectionType == pgsTypes::psctContinousClosureJoint )
      {
         W = 2*(maxBrgOffset - maxSupportWidth/2);
      }
      else if ( connectionType == pgsTypes::psctIntegralClosureJoint )
      {
         W = 2*(maxBrgOffset + maxSupportWidth/2);
      }
      else if ( connectionType == pgsTypes::psctContinuousSegment )
      {
         W = maxSupportWidth;
      }
      else if ( connectionType == pgsTypes::psctIntegralSegment )
      {
         W = maxSupportWidth;
         if ( pPier->GetPierModelType() == pgsTypes::pmtPhysical )
         {
            // The segment/pier connection is integral and there is a physical model of the pier.
            // Use the lower cross beam width as the minimum width of the pier diaphragm
            W = Max(W,pPier->GetXBeamWidth());
         }
      }
      else
      {
         ATLASSERT(false); // Is there a new type?
         W = 0;
      }
   }

   return W;
}

void CBridgeAgentImp::ValidateDeckParameters()
{
   if ( m_bDeckParametersValidated )
   {
      return;
   }

   HRESULT hr = m_SectCutTool->GetDeckProperties(m_Bridge,10,&m_DeckSurfaceArea,&m_DeckVolume);
   ATLASSERT(SUCCEEDED(hr));
   m_bDeckParametersValidated = true;
}

void CBridgeAgentImp::InvalidateDeckParameters()
{
   m_bDeckParametersValidated = false;
   m_DeckSurfaceArea = -1;
   m_DeckVolume = -1;
}
