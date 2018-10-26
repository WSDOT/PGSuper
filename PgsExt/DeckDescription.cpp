///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\DeckDescription.h>
#include <PgsExt\BridgeDescription.h>
#include <Units\SysUnits.h>
#include <StdIo.h>

#include <IFace\Tools.h>
#include <IFace\Project.h>

#include <WbflAtlExt.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CDeckDescription
****************************************************************************/


CDeckDescription::CDeckDescription()
{
   DeckType               = pgsTypes::sdtCompositeCIP;
   TransverseConnectivity = pgsTypes::atcConnectedAsUnit; // only applicable if girder spacing is adjacent

   GrossDepth       = ::ConvertToSysUnits(  8.5, unitMeasure::Inch );
   Fillet           = ::ConvertToSysUnits( 0.75, unitMeasure::Inch );

   SlabConcreteType     = pgsTypes::Normal;
   SlabFc               = ::ConvertToSysUnits(4.,unitMeasure::KSI);
   SlabStrengthDensity  = ::ConvertToSysUnits(160.,unitMeasure::LbfPerFeet3);
   SlabWeightDensity    = ::ConvertToSysUnits(160.,unitMeasure::LbfPerFeet3);
   SlabMaxAggregateSize = ::ConvertToSysUnits(0.75,unitMeasure::Inch);
   SlabEcK1             = 1.0;
   SlabEcK2             = 1.0;
   SlabCreepK1          = 1.0;
   SlabCreepK2          = 1.0;
   SlabShrinkageK1      = 1.0;
   SlabShrinkageK2      = 1.0;
   SlabUserEc           = false;
   SlabEc               = ::ConvertToSysUnits(4200.,unitMeasure::KSI);
   SlabHasFct           = false;
   SlabFct              = 0.0;

   WearingSurface = pgsTypes::wstSacrificialDepth;
   bInputAsDepthAndDensity = false;
   OverlayWeight  = ::ConvertToSysUnits( 25.0, unitMeasure::PSF );
   OverlayDensity = 0;
   OverlayDepth = 0;
   SacrificialDepth = ::ConvertToSysUnits(  0.5, unitMeasure::Inch );
   PanelDepth       = ::ConvertToSysUnits(  0.0, unitMeasure::Inch );
   PanelSupport     = ::ConvertToSysUnits(  4.0, unitMeasure::Inch );
                         // for horizontal shear capacity)

   OverhangTaper = pgsTypes::dotTopTopFlange;
   OverhangEdgeDepth = ::ConvertToSysUnits( 7.0, unitMeasure::Inch );

   Condition = pgsTypes::cfGood;
   ConditionFactor = 1.0;

   m_pBridgeDesc = nullptr;
}

CDeckDescription::CDeckDescription(const CDeckDescription& rOther)
{
   MakeCopy(rOther);
}

CDeckDescription::~CDeckDescription()
{
}

//======================== OPERATORS  =======================================
CDeckDescription& CDeckDescription::operator= (const CDeckDescription& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CDeckDescription::operator == (const CDeckDescription& rOther) const
{
   if ( DeckType != rOther.DeckType )
      return false;

   if ( TransverseConnectivity != rOther.TransverseConnectivity )
      return false;

   if ( !IsEqual( GrossDepth, rOther.GrossDepth ) )
      return false;

   if ( OverhangTaper != rOther.OverhangTaper )
      return false;

   if ( !IsEqual( OverhangEdgeDepth, rOther.OverhangEdgeDepth ) )
      return false;

   if ( !IsEqual( Fillet, rOther.Fillet ) )
      return false;

   if ( SlabFc != rOther.SlabFc )
      return false;

   if ( !IsEqual( SlabWeightDensity, rOther.SlabWeightDensity ) )
      return false;

   if ( SlabStrengthDensity != rOther.SlabStrengthDensity )
      return false;

   if ( SlabMaxAggregateSize != rOther.SlabMaxAggregateSize )
      return false;

   if ( SlabEcK1 != rOther.SlabEcK1 )
      return false;

   if ( SlabEcK2 != rOther.SlabEcK2 )
      return false;

   if ( SlabCreepK1 != rOther.SlabCreepK1 )
      return false;

   if ( SlabCreepK2 != rOther.SlabCreepK2 )
      return false;

   if ( SlabShrinkageK1 != rOther.SlabShrinkageK1 )
      return false;

   if ( SlabShrinkageK2 != rOther.SlabShrinkageK2 )
      return false;

   if ( SlabUserEc != rOther.SlabUserEc )
      return false;

   if ( !IsEqual(SlabEc,rOther.SlabEc) )
      return false;

   if ( SlabHasFct != rOther.SlabHasFct )
      return false;

   if ( SlabHasFct && !IsEqual(SlabFct,rOther.SlabFct) )
      return false;

   if (bInputAsDepthAndDensity != rOther.bInputAsDepthAndDensity)
   {
      return false;
   }

   if (bInputAsDepthAndDensity)
   {
      if ( !IsEqual( OverlayDepth, rOther.OverlayDepth ) )
         return false;

      if ( !IsEqual( OverlayDensity, rOther.OverlayDensity ) )
         return false;

      ATLASSERT(IsEqual(OverlayWeight, rOther.OverlayWeight )); // Sanity check. This should have been computed and synched elsewhere
   }
   else
   {
      if ( !IsEqual( OverlayWeight, rOther.OverlayWeight ) )
         return false;
   }

	if ( !IsEqual( SacrificialDepth, rOther.SacrificialDepth ) )
      return false;

   if ( PanelDepth != rOther.PanelDepth )
      return false;

   if ( PanelSupport != rOther.PanelSupport )
      return false;

   if ( DeckRebarData != rOther.DeckRebarData )
      return false;

   if ( WearingSurface != rOther.WearingSurface )
      return false;

   if ( DeckEdgePoints != rOther.DeckEdgePoints )
      return false;

   if ( Condition != rOther.Condition )
      return false;

   if ( !IsEqual(ConditionFactor,rOther.ConditionFactor) )
      return false;

   return true;
}

bool CDeckDescription::operator != (const CDeckDescription& rOther) const
{
   return !operator==( rOther );
}

//======================== OPERATIONS =======================================

// this global and free function are used to clean up neg moment rebar data
// during load. See information at bottom of Load method
PierIndexType g_NumPiers;
bool MaxPierIdx(CDeckRebarData::NegMomentRebarData& rebarData)
{
   return g_NumPiers <= rebarData.PierIdx;
}

HRESULT CDeckDescription::Load(IStructuredLoad* pStrLoad,IProgress* pProgress,pgsTypes::SlabOffsetType* pSlabOffsetType,Float64* pSlabOffset)
{
   USES_CONVERSION;

  CHRException hr;

   try
   {
      hr = pStrLoad->BeginUnit(_T("Deck"));
      Float64 version;
      hr = pStrLoad->get_Version(&version);

      CComVariant var;

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("SlabType"), &var );
      DeckType = (pgsTypes::SupportedDeckType)(int)var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("TransverseConnectivity"), &var );
      TransverseConnectivity = (pgsTypes::AdjacentTransverseConnectivity)(int)var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("GrossDepth"), &var );
      GrossDepth = var.dblVal;

      if ( version < 2 )
      {
         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("LeftOverhang"), &var );
         Float64 LeftOverhang = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("RightOverhang"), &var );
         Float64 RightOverhang = var.dblVal;

         // make this into a deck edge point
         GirderIndexType nGirders  = m_pBridgeDesc->GetSpan(0)->GetGirderCount();
         Float64 spacing = m_pBridgeDesc->GetSpan(0)->GetGirderSpacing(pgsTypes::metStart)->GetGirderSpacing(0);
         CDeckPoint point;
         point.LeftEdge  = LeftOverhang  + (nGirders-1)*spacing/2;
         point.RightEdge = RightOverhang + (nGirders-1)*spacing/2;
         point.Station = 0;
         point.MeasurementType      = pgsTypes::omtBridge;
         point.LeftTransitionType   = pgsTypes::dptLinear;
         point.RightTransitionType  = pgsTypes::dptLinear;
         DeckEdgePoints.push_back(point);
      }
      else
      {
         var.vt = VT_INDEX;
         pStrLoad->get_Property(_T("DeckEdgePointCount"),&var);
         IndexType nPoints = VARIANT2INDEX(var);

         if ( 0 < nPoints && nPoints != INVALID_INDEX )
         {
            pStrLoad->BeginUnit(_T("DeckEdgePoints"));
            for ( IndexType i = 0; i < nPoints; i++ )
            {
               CDeckPoint point;
               point.Load(pStrLoad,pProgress);
               DeckEdgePoints.push_back(point);
            }
            pStrLoad->EndUnit();
         }
      }

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("OverhangEdgeDepth"),&var);
      OverhangEdgeDepth = var.dblVal;

      var.Clear();
      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("OverhangTaperType"),&var);
      OverhangTaper = (pgsTypes::DeckOverhangTaper)(var.lVal);

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("Fillet"), &var );
      Fillet = var.dblVal;

      if ( version < 4 )
      {
         // slab offset was moved up to the bridge level with version 4 of this data block
         // if this version is less than 4, read the slab offset and pass it upstream with
         // the supplied pointer. Slab offset is for the entire bridge
         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("SlabOffset"), &var );
         *pSlabOffset = var.dblVal;
         *pSlabOffsetType = pgsTypes::sotBridge;
      }

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("PanelDepth"),&var );
      PanelDepth = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("PanelSupport"),&var );
      PanelSupport = var.dblVal;

      var.Clear();
      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("WearingSurfaceType"),&var);
      WearingSurface = (pgsTypes::WearingSurfaceType)(var.lVal);

      if ( version < 3.0 )
      {
         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("OverlayDepth"), &var );
         OverlayDepth = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("OverlayDensity"), &var );
         OverlayDensity = var.dblVal;

         Float64 g = unitSysUnitsMgr::GetGravitationalAcceleration();
         OverlayWeight = OverlayDepth*OverlayDensity*g;

         bInputAsDepthAndDensity = true;
      }
      else
      {
         var.Clear();
         var.vt = VT_BOOL;
         pStrLoad->get_Property(_T("InputAsDepthAndDensity"),&var);
         bInputAsDepthAndDensity = (var.boolVal == VARIANT_TRUE ? true : false);

         if ( bInputAsDepthAndDensity )
         {
            var.Clear();
            var.vt = VT_R8;
            hr = pStrLoad->get_Property(_T("OverlayDepth"), &var );
            OverlayDepth = var.dblVal;

            var.Clear();
            var.vt = VT_R8;
            hr = pStrLoad->get_Property(_T("OverlayDensity"), &var );
            OverlayDensity = var.dblVal;

            Float64 g = unitSysUnitsMgr::GetGravitationalAcceleration();
            OverlayWeight = OverlayDepth*OverlayDensity*g;
         }
         else
         {
            var.Clear();
            var.vt = VT_R8;
            pStrLoad->get_Property(_T("OverlayWeight"),&var); // added in version 3.0
            OverlayWeight = var.dblVal;
         }

     }

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("SacrificialDepth"), &var );
      SacrificialDepth = var.dblVal;

      // there was a bug in the PGSuper interface that allowed the sacrifical depth to
      // be greater than the gross/cast depth of the slab. Obviously this is incorrect.
      // If this is encountered in the input, fix it.
      if ( DeckType != pgsTypes::sdtNone && GrossDepth <= SacrificialDepth )
         SacrificialDepth = GrossDepth/2;

      if ( version < 6 )
      {
         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("SlabFc"), &var );
         SlabFc = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("SlabWeightDensity"), &var );
         SlabWeightDensity = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("SlabStrengthDensity"), &var );
         SlabStrengthDensity = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("SlabMaxAggregateSize"), &var );
         SlabMaxAggregateSize = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("SlabK1"), &var );
         SlabEcK1 = var.dblVal;

         var.Clear();
         var.vt = VT_BOOL;
         hr = pStrLoad->get_Property(_T("SlabUserEc"),&var);
         SlabUserEc = (var.boolVal == VARIANT_TRUE ? true : false);

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("SlabEc"), &var );
         SlabEc = var.dblVal;
      }
      else
      {
         hr = pStrLoad->BeginUnit(_T("SlabConcrete"));

         var.Clear();
         var.vt = VT_BSTR;
         hr = pStrLoad->get_Property(_T("Type"),&var);
         SlabConcreteType = (pgsTypes::ConcreteType)lrfdConcreteUtil::GetTypeFromName(OLE2T(var.bstrVal));

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("Fc"), &var );
         SlabFc = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("WeightDensity"), &var );
         SlabWeightDensity = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("StrengthDensity"), &var );
         SlabStrengthDensity = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("MaxAggregateSize"), &var );
         SlabMaxAggregateSize = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("EcK1"), &var );
         SlabEcK1 = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("EcK2"), &var );
         SlabEcK2 = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("CreepK1"), &var );
         SlabCreepK1 = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("CreepK2"), &var );
         SlabCreepK2 = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("ShrinkageK1"), &var );
         SlabShrinkageK1 = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("ShrinkageK2"), &var );
         SlabShrinkageK2 = var.dblVal;

         var.Clear();
         var.vt = VT_BOOL;
         hr = pStrLoad->get_Property(_T("UserEc"),&var);
         SlabUserEc = (var.boolVal == VARIANT_TRUE ? true : false);

         if ( SlabUserEc )
         {
            var.Clear();
            var.vt = VT_R8;
            hr = pStrLoad->get_Property(_T("Ec"), &var );
            SlabEc = var.dblVal;
         }

         if ( SlabConcreteType != pgsTypes::Normal )
         {
            var.Clear();
            var.vt = VT_BOOL;
            hr = pStrLoad->get_Property(_T("HasFct"),&var);
            SlabHasFct = (var.boolVal == VARIANT_TRUE ? true : false);

            if ( SlabHasFct )
            {
               var.Clear();
               var.vt = VT_R8;
               hr = pStrLoad->get_Property(_T("Fct"), &var );
               SlabFct = var.dblVal;
            }
         }

         pStrLoad->EndUnit(); // SlabConcrete
      } 

      if ( 4 < version )
      {
         pStrLoad->BeginUnit(_T("Condition"));
         var.vt = VT_I4;
         pStrLoad->get_Property(_T("ConditionFactorType"),&var);
         Condition = (pgsTypes::ConditionFactorType)(var.lVal);

         var.vt = VT_R8;
         pStrLoad->get_Property(_T("ConditionFactor"),&var);
         ConditionFactor = var.dblVal;
      
         pStrLoad->EndUnit();
      }

      DeckRebarData.Load(pStrLoad,pProgress);

      // in some 2.1 Beta versions we stored deck edge points for NoDeck and CompositeOverlay decks
      // these decks should have any points so clear them out right now.
      if ( IsConstantWidthDeck(DeckType) )
         DeckEdgePoints.clear();

      hr = pStrLoad->EndUnit();
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   }

   // Not sure how this happened, but at least on regression test file (UserSelect.pgs) has negative
   // moment rebar data for a pier that doesn't exist. If it happened for that file, it is likely
   // to have happened for other files as well. When this data is encountered in other places in 
   // the software it causes a crash. This code block removes deck rebar data at piers that 
   // don't exist.
   g_NumPiers = m_pBridgeDesc->GetPierCount();
   std::vector<CDeckRebarData::NegMomentRebarData>::iterator new_end = std::remove_if(DeckRebarData.NegMomentRebar.begin(),DeckRebarData.NegMomentRebar.end(),MaxPierIdx);
   DeckRebarData.NegMomentRebar.erase(new_end,DeckRebarData.NegMomentRebar.end());

   return hr;
}

HRESULT CDeckDescription::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit(_T("Deck"),6.0);

   pStrSave->put_Property(_T("SlabType"),         CComVariant(DeckType));
   pStrSave->put_Property(_T("TransverseConnectivity"), CComVariant(TransverseConnectivity)); // added for version 14.0
   pStrSave->put_Property(_T("GrossDepth"),       CComVariant(GrossDepth));

   pStrSave->put_Property(_T("DeckEdgePointCount"),CComVariant((long)DeckEdgePoints.size()));

   if ( 0 < DeckEdgePoints.size() )
   {
      pStrSave->BeginUnit(_T("DeckEdgePoints"),1.0);
      std::vector<CDeckPoint>::iterator iter(DeckEdgePoints.begin());
      std::vector<CDeckPoint>::iterator end(DeckEdgePoints.end());
      for ( ; iter != end; iter++ )
      {
         CDeckPoint& point = *iter;
         point.Save(pStrSave,pProgress);
      }
      pStrSave->EndUnit();
   }

   pStrSave->put_Property(_T("OverhangEdgeDepth"),CComVariant(OverhangEdgeDepth));
   pStrSave->put_Property(_T("OverhangTaperType"),CComVariant(OverhangTaper));
   pStrSave->put_Property(_T("Fillet"),           CComVariant(Fillet));
   //pStrSave->put_Property(_T("SlabOffset"),       CComVariant(SlabOffset)); // removed in version 4 (moved up to bridge level)
   pStrSave->put_Property(_T("PanelDepth"),       CComVariant(PanelDepth));
   pStrSave->put_Property(_T("PanelSupport"),     CComVariant(PanelSupport));
   pStrSave->put_Property(_T("WearingSurfaceType"), CComVariant(WearingSurface));

   pStrSave->put_Property(_T("InputAsDepthAndDensity"),CComVariant(bInputAsDepthAndDensity)); // added in version 3.0
   if ( bInputAsDepthAndDensity )
   {
      pStrSave->put_Property(_T("OverlayDepth"),     CComVariant(OverlayDepth));
      pStrSave->put_Property(_T("OverlayDensity"),   CComVariant(OverlayDensity));
   }
   else
   {
      pStrSave->put_Property(_T("OverlayWeight"),CComVariant(OverlayWeight)); // added in version 3.0
   }

   pStrSave->put_Property(_T("SacrificialDepth"), CComVariant(SacrificialDepth));

   // Added in version 6
   // new parameters are Unit, SlabConcreteType, SlabHasFct, and SlabFct
   pStrSave->BeginUnit(_T("SlabConcrete"),1.0);

      pStrSave->put_Property(_T("Type"),CComVariant( lrfdConcreteUtil::GetTypeName((matConcrete::Type)SlabConcreteType,false).c_str() ));
      pStrSave->put_Property(_T("Fc"),               CComVariant(SlabFc));
      pStrSave->put_Property(_T("WeightDensity"),    CComVariant(SlabWeightDensity));
      pStrSave->put_Property(_T("StrengthDensity"),  CComVariant(SlabStrengthDensity));
      pStrSave->put_Property(_T("MaxAggregateSize"), CComVariant(SlabMaxAggregateSize));
      pStrSave->put_Property(_T("EcK1"),             CComVariant(SlabEcK1));
      pStrSave->put_Property(_T("EcK2"),             CComVariant(SlabEcK2));
      pStrSave->put_Property(_T("CreepK1"),          CComVariant(SlabCreepK1)); 
      pStrSave->put_Property(_T("CreepK2"),          CComVariant(SlabCreepK2));
      pStrSave->put_Property(_T("ShrinkageK1"),      CComVariant(SlabShrinkageK1)); 
      pStrSave->put_Property(_T("ShrinkageK2"),      CComVariant(SlabShrinkageK2));
      pStrSave->put_Property(_T("UserEc"), CComVariant(SlabUserEc));

      if ( SlabUserEc )
         pStrSave->put_Property(_T("Ec"),     CComVariant(SlabEc));

      if ( SlabConcreteType != pgsTypes::Normal )
      {
         pStrSave->put_Property(_T("HasFct"),CComVariant(SlabHasFct));
         
         if ( SlabHasFct )
            pStrSave->put_Property(_T("Fct"),CComVariant(SlabFct));
      }

   pStrSave->EndUnit();

   // Added in version 5
   pStrSave->BeginUnit(_T("Condition"),1.0);
   pStrSave->put_Property(_T("ConditionFactorType"),CComVariant(Condition));
   pStrSave->put_Property(_T("ConditionFactor"),CComVariant(ConditionFactor));
   pStrSave->EndUnit(); // Condition

   DeckRebarData.Save(pStrSave,pProgress);

   pStrSave->EndUnit();

   return hr;
}


void CDeckDescription::SetBridgeDescription(const CBridgeDescription* pBridge)
{
   m_pBridgeDesc = pBridge;
}

const CBridgeDescription* CDeckDescription::GetBridgeDescription() const
{
   return m_pBridgeDesc;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CDeckDescription::MakeCopy(const CDeckDescription& rOther)
{
   // Add copy code here...
   DeckType       = rOther.DeckType;
   TransverseConnectivity = rOther.TransverseConnectivity;

   GrossDepth       = rOther.GrossDepth;
   OverhangTaper    = rOther.OverhangTaper;
   OverhangEdgeDepth = rOther.OverhangEdgeDepth;
   Fillet           = rOther.Fillet;
	OverlayWeight    = rOther.OverlayWeight;
   OverlayDensity   = rOther.OverlayDensity;
   OverlayDepth     = rOther.OverlayDepth;
   bInputAsDepthAndDensity = rOther.bInputAsDepthAndDensity;
	SacrificialDepth = rOther.SacrificialDepth;
   WearingSurface   = rOther.WearingSurface;

   SlabConcreteType      = rOther.SlabConcreteType;
   SlabFc                = rOther.SlabFc;
   SlabWeightDensity     = rOther.SlabWeightDensity;
   SlabStrengthDensity   = rOther.SlabStrengthDensity;
   SlabMaxAggregateSize  = rOther.SlabMaxAggregateSize;
   SlabEcK1              = rOther.SlabEcK1;
   SlabEcK2              = rOther.SlabEcK2;
   SlabCreepK1           = rOther.SlabCreepK1;
   SlabCreepK2           = rOther.SlabCreepK2;
   SlabShrinkageK1       = rOther.SlabShrinkageK1;
   SlabShrinkageK2       = rOther.SlabShrinkageK2;
   SlabUserEc            = rOther.SlabUserEc;
   SlabEc                = rOther.SlabEc;
   SlabHasFct            = rOther.SlabHasFct;
   SlabFct               = rOther.SlabFct;

   PanelDepth       = rOther.PanelDepth;
   PanelSupport     = rOther.PanelSupport;

   Condition = rOther.Condition;
   ConditionFactor = rOther.ConditionFactor;

   DeckRebarData = rOther.DeckRebarData;

   DeckEdgePoints = rOther.DeckEdgePoints;
}

void CDeckDescription::MakeAssignment(const CDeckDescription& rOther)
{
   MakeCopy( rOther );
}
