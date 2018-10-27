///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

#include "stdafx.h"
#include "BridgeGeometryModelBuilder.h"
#include "BridgeHelpers.h"

#include <PgsExt\ClosureJointData.h>
#include <PsgLib\GirderLibraryEntry.h>

#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CBridgeGeometryModelBuilder::CBridgeGeometryModelBuilder()
{
   m_AlignmentID = 999;
}

bool CBridgeGeometryModelBuilder::BuildBridgeGeometryModel(const CBridgeDescription2* pBridgeDesc,ICogoModel* pCogoModel,IAlignment* pAlignment,IBridgeGeometry* pBridgeGeometry)
{
   //
   // Associate COGO model with bridge
   //
   pBridgeGeometry->putref_CogoModel(pCogoModel);

   // Associate an alignment with the bridge model (bridges can be associated with many alignments)
   pBridgeGeometry->putref_Alignment(m_AlignmentID,pAlignment);

   // Set the alignment offset
   pBridgeGeometry->put_AlignmentOffset(pBridgeDesc->GetAlignmentOffset());

   // Set the ID of the main bridge alignment (all other alignments are secondary)
   pBridgeGeometry->put_BridgeAlignmentID(m_AlignmentID);

   if ( !LayoutPiers(pBridgeDesc,pBridgeGeometry) )
   {
      return false;
   }

   if ( !LayoutTemporarySupports(pBridgeDesc,pBridgeGeometry) )
   {
      return false;
   }

   pBridgeGeometry->UpdateGeometry(); // this makes the pier lines available

   if ( !LayoutGirderLines(pBridgeDesc,pBridgeGeometry) )
   {
      return false;
   }

   if ( !LayoutDiaphragmLines(pBridgeDesc,pBridgeGeometry) )
   {
      return false;
   }

   return true;
}

bool CBridgeGeometryModelBuilder::LayoutPiers(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry)
{
   CComPtr<IAlignment> alignment;
   pBridgeGeometry->get_BridgeAlignment(&alignment);

   const CPierData2* pPier = pBridgeDesc->GetPier(0);
   PierIndexType pierIdx = 0;
   while ( pPier )
   {
      PierIDType pierID = ::GetPierLineID(pierIdx);

      CComPtr<IAngle> skew;
      CComPtr<IDirection> direction;
      GetPierDirection(alignment,pPier,&skew,&direction);

      Float64 skew_angle;
      skew->get_Value(&skew_angle);

      Float64 pier_length;
      Float64 left_end_offset; // offset from the alignment to the left hand-side of the pier, measured along the pier

      if ( pPier->GetPierModelType() == pgsTypes::pmtIdealized )
      {
         // get maximum girder spacing width (back and ahead side of piers)
         // width is to be measured along CL pier
         // also get the offset from the alignment, measured along the CL pier, to the left-most girder
         Float64 back_width  = 0;
         Float64 ahead_width = 0;

         Float64 back_offset  = 0;
         Float64 ahead_offset = 0;

         const CSpanData2* pPrevSpan = pPier->GetPrevSpan();
         const CSpanData2* pNextSpan = pPier->GetNextSpan();

         if ( pPrevSpan )
         {
            const CGirderSpacing2* pSpacing = pPier->GetGirderSpacing(pgsTypes::Back);
            GetPierLineProperties(pBridgeDesc,pSpacing,skew_angle,&back_width,&back_offset);
         }

         if ( pNextSpan )
         {
            const CGirderSpacing2* pSpacing = pPier->GetGirderSpacing(pgsTypes::Ahead);
            GetPierLineProperties(pBridgeDesc,pSpacing,skew_angle,&ahead_width,&ahead_offset);
         }

         if ( back_width < ahead_width )
         {
            pier_length     = ahead_width;
            left_end_offset = ahead_offset;
         }
         else
         {
            pier_length     = back_width;
            left_end_offset = back_offset;
         }
      }
      else
      {
         Float64 w = pPier->GetColumnSpacingWidth();
         Float64 x5,x6;
         pPier->GetXBeamOverhangs(&x5,&x6);
         pier_length = w + x5 + x6;

         // update offset, which is the offset to the left edge of the pier from the alignment
         // measured along the CL Pier
         ColumnIndexType refColIdx;
         Float64 refColOffset;
         pgsTypes::OffsetMeasurementType refColOffsetMeasure;
         pPier->GetTransverseOffset(&refColIdx,&refColOffset,&refColOffsetMeasure);

         if ( refColOffsetMeasure == pgsTypes::omtBridge )
         {
            Float64 blo = pBridgeDesc->GetAlignmentOffset();
            refColOffset += blo;
         }

         left_end_offset = refColOffset - x5 - pPier->GetColumnSpacingWidthToColumn(refColIdx);
      }

      Float64 station = pPier->GetStation();
      std::_tstring strOrientation = pPier->GetOrientation();
      CComPtr<IPierLine> pierline;
      pBridgeGeometry->CreatePierLine(pierID,m_AlignmentID,CComVariant(station),CComBSTR(strOrientation.c_str()),pier_length,left_end_offset,&pierline);

      // Layout connection geometry on back side of pier
      if ( pPier->GetPrevSpan() )
      {
         Float64 offset;
         ConnectionLibraryEntry::BearingOffsetMeasurementType offsetMeasureType;
         pPier->GetBearingOffset(pgsTypes::Back,&offset,&offsetMeasureType);
         pierline->put_BearingOffset( pfBack, offset );
         pierline->put_BearingOffsetMeasurementType( pfBack, offsetMeasureType == ConnectionLibraryEntry::AlongGirder ? mtAlongItem : mtNormal );

         Float64 endDist;
         ConnectionLibraryEntry::EndDistanceMeasurementType endDistMeasureType;
         pPier->GetGirderEndDistance(pgsTypes::Back,&endDist,&endDistMeasureType);
         pierline->put_EndDistance( pfBack, endDist );
         if ( endDistMeasureType == ConnectionLibraryEntry::FromBearingAlongGirder )
         {
            pierline->put_EndDistanceMeasurementLocation( pfBack, mlCenterlineBearing );
            pierline->put_EndDistanceMeasurementType(     pfBack, mtAlongItem );
         }
         else if ( endDistMeasureType == ConnectionLibraryEntry::FromBearingNormalToPier )
         {
            pierline->put_EndDistanceMeasurementLocation( pfBack, mlCenterlineBearing );
            pierline->put_EndDistanceMeasurementType(     pfBack, mtNormal );
         }
         else if ( endDistMeasureType == ConnectionLibraryEntry::FromPierAlongGirder )
         {
            pierline->put_EndDistanceMeasurementLocation( pfBack, mlPierLine );
            pierline->put_EndDistanceMeasurementType(     pfBack, mtAlongItem );
         }
         else if ( endDistMeasureType == ConnectionLibraryEntry::FromPierNormalToPier )
         {
            pierline->put_EndDistanceMeasurementLocation( pfBack, mlPierLine );
            pierline->put_EndDistanceMeasurementType(     pfBack, mtNormal );
         }
      }


      // Layout connection geometry on ahead side of pier
      if ( pPier->GetNextSpan() )
      {
         Float64 offset;
         ConnectionLibraryEntry::BearingOffsetMeasurementType offsetMeasureType;
         pPier->GetBearingOffset(pgsTypes::Ahead,&offset,&offsetMeasureType);
         pierline->put_BearingOffset( pfAhead, offset );
         pierline->put_BearingOffsetMeasurementType( pfAhead, offsetMeasureType == ConnectionLibraryEntry::AlongGirder ? mtAlongItem : mtNormal );

         Float64 endDist;
         ConnectionLibraryEntry::EndDistanceMeasurementType endDistMeasureType;
         pPier->GetGirderEndDistance(pgsTypes::Ahead,&endDist,&endDistMeasureType);
         pierline->put_EndDistance( pfAhead, endDist );
         if ( endDistMeasureType == ConnectionLibraryEntry::FromBearingAlongGirder )
         {
            pierline->put_EndDistanceMeasurementLocation( pfAhead, mlCenterlineBearing );
            pierline->put_EndDistanceMeasurementType(     pfAhead, mtAlongItem );
         }
         else if ( endDistMeasureType == ConnectionLibraryEntry::FromBearingNormalToPier )
         {
            pierline->put_EndDistanceMeasurementLocation( pfAhead, mlCenterlineBearing );
            pierline->put_EndDistanceMeasurementType(     pfAhead, mtNormal );
         }
         else if ( endDistMeasureType == ConnectionLibraryEntry::FromPierAlongGirder )
         {
            pierline->put_EndDistanceMeasurementLocation( pfAhead, mlPierLine );
            pierline->put_EndDistanceMeasurementType(     pfAhead, mtAlongItem );
         }
         else if ( endDistMeasureType == ConnectionLibraryEntry::FromPierNormalToPier )
         {
            pierline->put_EndDistanceMeasurementLocation( pfAhead, mlPierLine );
            pierline->put_EndDistanceMeasurementType(     pfAhead, mtNormal );
         }
      }

      const CSpanData2* pSpan = pPier->GetNextSpan();
      if ( pSpan )
      {
         pPier = pSpan->GetNextPier();
      }
      else
      {
         pPier = nullptr;
      }

      pierIdx++;
   }

   return true;
}

bool CBridgeGeometryModelBuilder::LayoutTemporarySupports(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry)
{
   CComPtr<IAlignment> alignment;
   pBridgeGeometry->get_BridgeAlignment(&alignment);

   SupportIndexType nTS = pBridgeDesc->GetTemporarySupportCount();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
#pragma Reminder("UPDATE: Find a way to use the same Temporary Support ID for the bridge and geometry models")
      SupportIDType tsID = ::GetTempSupportLineID(tsIdx); // this is the ID of the line that represents the temporary
                                                          // support in the geometry model, not the ID of the temporary support
                                                          // in the bridge data model

      const CTemporarySupportData* pTS = pBridgeDesc->GetTemporarySupport(tsIdx);

      Float64 station = pTS->GetStation();

      Float64 pier_width;
      Float64 left_end_offset;
      if ( pTS->GetConnectionType() == pgsTypes::tsctContinuousSegment )
      {
         ATLASSERT(!pTS->HasSpacing());
         // no spacing at the TS... need to base width on spacing at ends of segment
         // also need to account for the fact that the ref girder can be different at each end
         // once ref girder is based on girder index 0, the ref girder offset at this TS can be computed
         CSegmentKey segmentKey, rightSegmentKey;
         pBridgeDesc->GetSegmentsAtTemporarySupport(pTS->GetIndex(),&segmentKey,&rightSegmentKey);
         ATLASSERT(segmentKey == rightSegmentKey);

         // we need a specific segment key... this segment key is for all girders
         ATLASSERT(segmentKey.girderIndex == ALL_GIRDERS);
         segmentKey.girderIndex = 0;
         const CPrecastSegmentData* pSegment = pBridgeDesc->GetSegment(segmentKey);

         const CPierData2* pStartPier;
         const CTemporarySupportData* pStartTS;
         pSegment->GetSupport(pgsTypes::metStart,&pStartPier,&pStartTS);

         const CGirderSpacing2* pStartSpacing;
         Float64 start_station, start_skew_angle;
         if ( pStartPier )
         {
            start_station = pStartPier->GetStation();

            CComPtr<IAngle> skew;
            CComPtr<IDirection> direction;
            GetPierDirection(alignment,pStartPier,&skew,&direction);
            skew->get_Value(&start_skew_angle);

            pStartSpacing = pStartPier->GetGirderSpacing(pgsTypes::Ahead);
         }
         else
         {
            ATLASSERT(pStartTS);
            start_station = pStartTS->GetStation();

            CComPtr<IAngle> skew;
            CComPtr<IDirection> direction;
            GetTempSupportDirection(alignment,pStartTS,&skew,&direction);
            skew->get_Value(&start_skew_angle);

            pStartSpacing = pStartTS->GetSegmentSpacing();
         }

         const CPierData2* pEndPier;
         const CTemporarySupportData* pEndTS;
         pSegment->GetSupport(pgsTypes::metEnd,&pEndPier,&pEndTS);

         const CGirderSpacing2* pEndSpacing;
         Float64 end_station, end_skew_angle;
         if ( pEndPier )
         {
            end_station = pEndPier->GetStation();

            CComPtr<IAngle> skew;
            CComPtr<IDirection> direction;
            GetPierDirection(alignment,pEndPier,&skew,&direction);
            skew->get_Value(&end_skew_angle);

            pEndSpacing = pEndPier->GetGirderSpacing(pgsTypes::Back);
         }
         else
         {
            ATLASSERT(pEndTS);
            end_station = pEndTS->GetStation();

            CComPtr<IAngle> skew;
            CComPtr<IDirection> direction;
            GetTempSupportDirection(alignment,pEndTS,&skew,&direction);
            skew->get_Value(&end_skew_angle);

            pEndSpacing = pEndTS->GetSegmentSpacing();
         }

         Float64 start_width, start_offset;
         Float64 end_width, end_offset;
         GetPierLineProperties(pBridgeDesc,pStartSpacing,start_skew_angle,&start_width,&start_offset);
         GetPierLineProperties(pBridgeDesc,pEndSpacing,  end_skew_angle,  &end_width,  &end_offset);

         pier_width      = ::LinInterp(station - start_station,start_width, end_width, end_station-start_station);
         left_end_offset = ::LinInterp(station - start_station,start_offset,end_offset,end_station-start_station);
      }
      else
      {
         // spacing is measured at TS
         ATLASSERT(pTS->HasSpacing());

         CComPtr<IAngle> skew;
         CComPtr<IDirection> direction;
         GetTempSupportDirection(alignment,pTS,&skew,&direction);

         Float64 skew_angle;
         skew->get_Value(&skew_angle);

         const CGirderSpacing2* pSpacing = pTS->GetSegmentSpacing();
         GetPierLineProperties(pBridgeDesc,pSpacing,skew_angle,&pier_width,&left_end_offset);
      }

      std::_tstring strOrientation = pTS->GetOrientation();
      CComPtr<IPierLine> tsLine;
      pBridgeGeometry->CreatePierLine(tsID,m_AlignmentID,CComVariant(station),CComBSTR(strOrientation.c_str()),pier_width,left_end_offset,&tsLine);

      Float64 brgOffset;
      ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetMeasure;
      pTS->GetBearingOffset(&brgOffset,&brgOffsetMeasure);
      tsLine->put_BearingOffset( pfBack, brgOffset);
      tsLine->put_BearingOffsetMeasurementType( pfBack, (MeasurementType)brgOffsetMeasure );
      tsLine->put_BearingOffset( pfAhead, brgOffset);
      tsLine->put_BearingOffsetMeasurementType( pfAhead, (MeasurementType)brgOffsetMeasure );

      // layout connection geometry on both sides of temporary support
      Float64 endDistance;
      ConnectionLibraryEntry::EndDistanceMeasurementType measureType;
      pTS->GetGirderEndDistance(&endDistance,&measureType);
      tsLine->put_EndDistance( pfBack, endDistance );
      tsLine->put_EndDistance( pfAhead, endDistance );
      if ( measureType == ConnectionLibraryEntry::FromBearingAlongGirder )
      {
         tsLine->put_EndDistanceMeasurementLocation( pfBack, mlCenterlineBearing );
         tsLine->put_EndDistanceMeasurementType(     pfBack, mtAlongItem );
         tsLine->put_EndDistanceMeasurementLocation( pfAhead, mlCenterlineBearing );
         tsLine->put_EndDistanceMeasurementType(     pfAhead, mtAlongItem );
      }
      else if ( measureType == ConnectionLibraryEntry::FromBearingNormalToPier )
      {
         tsLine->put_EndDistanceMeasurementLocation( pfBack, mlCenterlineBearing );
         tsLine->put_EndDistanceMeasurementType(     pfBack, mtNormal );
         tsLine->put_EndDistanceMeasurementLocation( pfAhead, mlCenterlineBearing );
         tsLine->put_EndDistanceMeasurementType(     pfAhead, mtNormal );
      }
      else if ( measureType == ConnectionLibraryEntry::FromPierAlongGirder )
      {
         tsLine->put_EndDistanceMeasurementLocation( pfBack, mlPierLine );
         tsLine->put_EndDistanceMeasurementType(     pfBack, mtAlongItem );
         tsLine->put_EndDistanceMeasurementLocation( pfAhead, mlPierLine );
         tsLine->put_EndDistanceMeasurementType(     pfAhead, mtAlongItem );
      }
      else if ( measureType == ConnectionLibraryEntry::FromPierNormalToPier )
      {
         tsLine->put_EndDistanceMeasurementLocation( pfBack, mlPierLine );
         tsLine->put_EndDistanceMeasurementType(     pfBack, mtNormal );
         tsLine->put_EndDistanceMeasurementLocation( pfAhead, mlPierLine );
         tsLine->put_EndDistanceMeasurementType(     pfAhead, mtNormal );
      }
   }

   return true;
}

bool CBridgeGeometryModelBuilder::LayoutGirderLines(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry)
{
   // girder layout lines are geometric construction lines that are used
   // to create the actual girder line geometry

   bool bRetVal = false;
   switch( pBridgeDesc->GetGirderSpacingType() )
   {
   case pgsTypes::sbsUniform:
   case pgsTypes::sbsUniformAdjacent:
   case pgsTypes::sbsConstantAdjacent:
      bRetVal = LayoutUniformGirderLines(pBridgeDesc,pBridgeGeometry);
      break;

   case pgsTypes::sbsGeneral:
   case pgsTypes::sbsGeneralAdjacent:
      bRetVal = LayoutGeneralGirderLines(pBridgeDesc,pBridgeGeometry);
      break;

   default:
      ATLASSERT(false); // is there a new spacing type?
   }

   return bRetVal;
}

bool CBridgeGeometryModelBuilder::LayoutUniformGirderLines(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IRoadwayData, pIAlignment);
   bool bAnglePointInAlignment = false;
   AlignmentData2 alignment_data = pIAlignment->GetAlignmentData2();
   for (const auto& hc : alignment_data.HorzCurves)
   {
      if (IsZero(hc.Radius))
      {
         bAnglePointInAlignment = true;
         break;
      }
   }

   pgsTypes::MeasurementLocation measureLocation = pBridgeDesc->GetMeasurementLocation();
   pgsTypes::MeasurementType measureType = pBridgeDesc->GetMeasurementType();

   if ( !pBridgeDesc->UseSameNumberOfGirdersInAllGroups() ||
        !pBridgeDesc->UseSameGirderForEntireBridge()      || 
        measureType == pgsTypes::AlongItem                ||
        bAnglePointInAlignment
      )
   {
      // if there is a different number of girders in each group, but the spacing is uniform, this is the
      // same as general spacing... use the general spacing function and return

      // OR
      
      // if a different girder types are used, the girders aren't necessarily the same width so
      // this is the same as general spacing

      // OR
      
      // the girder spacing is measured along the CL pier or CL bearing. if the piers are skewed or
      // if the alignment is curved, the girder spacing along the alignment normal will not be uniform

      // OR
      
      // this is an angle point in the alignment. the simple layout doesn't work because of how parallel
      // alignment paths get created. use the more general layout. Note, we only really need to do this
      // if there is an angle point within the limits of the bridge, but it is just easier to do the more
      // general layout if any angle points exist
      return LayoutGeneralGirderLines(pBridgeDesc,pBridgeGeometry);
   }

#pragma Reminder("UPDATE: adjust ref girder offset for spacing measurement type")
#pragma Reminder("UPDATE: adjust girder spacing for spacing measurement type and location")

   pgsTypes::SupportedBeamSpacing spacingType = pBridgeDesc->GetGirderSpacingType();
   Float64 spacing = pBridgeDesc->GetGirderSpacing();

   if ( ::IsJointSpacing(spacingType) )
   {
      // the spacing is a joint spacing, need to add beam width

      ATLASSERT(pBridgeDesc->UseSameGirderForEntireBridge() == true);
      const CSplicedGirderData* pGirder = pBridgeDesc->GetGirderGroup(GroupIndexType(0))->GetGirder(0);
      const GirderLibraryEntry* pGdrLibEntry = pGirder->GetGirderLibraryEntry();
      Float64 w1 = pGdrLibEntry->GetBeamWidth(pgsTypes::metStart);
      Float64 w2 = pGdrLibEntry->GetBeamWidth(pgsTypes::metEnd);
      Float64 w  = Max(w1,w2);

      spacing += w;
   }

   GirderIndexType nGirders = pBridgeDesc->GetGirderCount();

   GirderIndexType refGirderIdx = pBridgeDesc->GetRefGirder();
   Float64 refGirderOffset = pBridgeDesc->GetRefGirderOffset();
   pgsTypes::OffsetMeasurementType refGirderOffsetType = pBridgeDesc->GetRefGirderOffsetType();

   // convert ref girder offset into an offset to the left-most girder
   if ( refGirderIdx == INVALID_INDEX )
   {
      // reference girder is the center of the spacing group
      refGirderOffset -= spacing*(nGirders-1)/2;
      refGirderIdx = 0;
   }
   else
   {
      // a specific girder is the reference girder
      refGirderOffset -= spacing*refGirderIdx;
      refGirderIdx = 0;
   }

   // if the ref girder is measured from the bridge, convert it to being measured from the alignment
   if ( refGirderOffsetType == pgsTypes::omtBridge )
   {
      // alignment offset is always measured normal to the alignment
      refGirderOffset += pBridgeDesc->GetAlignmentOffset();
   }


   // Create a girder layout line
   // Layout line is parallel to and offset from the alignment
   // Layout line IDs are the girder index
   CComPtr<IAlignmentOffsetLayoutLineFactory> factory;
   factory.CoCreateInstance(CLSID_AlignmentOffsetLayoutLineFactory);
   factory->put_LayoutLineID(0); // ID of first layout line
   factory->put_LayoutLineIDInc(1);
   factory->put_LayoutLineCount(nGirders);
   factory->put_Offset(-refGirderOffset);  // refGirderOffset < 0 = left of alignment; WBFL Bridge Geometry: positive value is to the left of the alignment; Change sign
   factory->put_OffsetIncrement(-spacing); // positive value places lines to the left of the previous line (want to place the lines to the right)
   factory->put_AlignmentID(m_AlignmentID);
   pBridgeGeometry->CreateLayoutLines(factory);

   //
   // Build Girder Lines
   //

   CComPtr<ISingleGirderLineFactory> glFactory;
   HRESULT hr = glFactory.CoCreateInstance(CLSID_SingleGirderLineFactory);
   ATLASSERT(SUCCEEDED(hr));

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
            LineIDType segID = ::GetGirderSegmentLineID(grpIdx,gdrIdx,segIdx);
            glFactory->put_GirderLineID( segID );   // girder line ID
            glFactory->put_LayoutLineID( gdrIdx );  // layout line used to layout this girder line
            glFactory->put_Type(glChord);           // layout as a chord
            glFactory->put_StartMeasurementType( measureType == pgsTypes::AlongItem ? mtAlongItem : mtNormal );
            glFactory->put_StartMeasurementLocation( measureLocation == pgsTypes::AtPierLine ? mlPierLine : mlCenterlineBearing );
            glFactory->put_EndMeasurementType( measureType == pgsTypes::AlongItem ? mtAlongItem : mtNormal );
            glFactory->put_EndMeasurementLocation( measureLocation == pgsTypes::AtPierLine ? mlPierLine : mlCenterlineBearing );

            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

            PierIDType startID, endID;
            GetPierID(pSegment,&startID,&endID);

            glFactory->put_StartPierID(startID);
            glFactory->put_EndPierID(endID);

            pBridgeGeometry->CreateGirderLines(glFactory);
         } // segment loop
      } // girder loop
   } // group loop

   return true;
}

bool CBridgeGeometryModelBuilder::LayoutGeneralGirderLines(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry)
{
   //
   // Do some preliminary setup for the function
   //
   CComPtr<ISimpleLayoutLineFactory> factory;
   factory.CoCreateInstance(CLSID_SimpleLayoutLineFactory);

   CComPtr<IAlignment> alignment;
   pBridgeGeometry->get_Alignment(m_AlignmentID,&alignment);

   Float64 alignmentOffset = pBridgeDesc->GetAlignmentOffset();

   pgsTypes::SupportedBeamSpacing spacingType = pBridgeDesc->GetGirderSpacingType();

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(0); // all girders in a group have the same number of segments... girder 0 is just as good as any other girder in this group
      SegmentIndexType nSegments = pGirder->GetSegmentCount();

      std::vector<CComPtr<IPoint2dCollection>> girderPoints(2*nSegments);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

         ResolveSegmentSpacing(pBridgeGeometry,alignmentOffset,pSegment,&girderPoints[2*segIdx],&girderPoints[2*segIdx+1]);
      }

   #if defined _DEBUG
      // verify all girder offset arrays are the same length
      for ( CollectionIndexType i = 0; i < nSegments; i++ )
      {
         CollectionIndexType size1;
         girderPoints[2*i]->get_Count(&size1);

         CollectionIndexType size2;
         girderPoints[2*i+1]->get_Count(&size2);

         ATLASSERT(size1 == size2);
      }
   #endif // _DEBUG

      // 
      // Create paths for layout lines
      //
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();

         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CComPtr<IPoint2d> pntStart, pntEnd;
            girderPoints[2*segIdx]->get_Item(gdrIdx,&pntStart);
            girderPoints[2*segIdx+1]->get_Item(gdrIdx,&pntEnd);

            CComPtr<ILineSegment2d> lineSegment;
            lineSegment.CoCreateInstance(CLSID_LineSegment2d);
            lineSegment->ThroughPoints(pntStart,pntEnd);

            CComPtr<IPath> path;
            path.CoCreateInstance(CLSID_Path);
            path->AddEx(lineSegment);

            LineIDType ID = ::GetGirderSegmentLineID(grpIdx,gdrIdx,segIdx);
            factory->AddPath(ID,path);
         } // end of segment loop
      } // end of girder loop

      // create the layout lines
      // there is a layout line for each segment. The layout line ID is derived from grpIdx/gdrIdx/segIdx
      pBridgeGeometry->CreateLayoutLines(factory);
      factory->Reset();

      //
      // Create the girder lines
      //
      CComPtr<ISingleGirderLineFactory> glFactory;
      glFactory.CoCreateInstance(CLSID_SingleGirderLineFactory);
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();

         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            LineIDType ID = ::GetGirderSegmentLineID(grpIdx,gdrIdx,segIdx);

            glFactory->put_GirderLineID( ID ); // girder line ID
            glFactory->put_LayoutLineID( ID ); // layout line used to layout this girder line

            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
            PierIDType startID, endID;
            GetPierID(pSegment,&startID,&endID);

            const CPierData2* pStartPier = nullptr;
            const CTemporarySupportData* pStartTS = nullptr;
            pSegment->GetSupport(pgsTypes::metStart,&pStartPier,&pStartTS);

            const CPierData2* pEndPier = nullptr;
            const CTemporarySupportData* pEndTS = nullptr;
            pSegment->GetSupport(pgsTypes::metEnd,&pEndPier,&pEndTS);

            const CGirderSpacing2* pStartSpacing = nullptr;
            const CGirderSpacing2* pEndSpacing   = nullptr;
            if ( pStartPier )
            {
               pStartSpacing = pStartPier->GetGirderSpacing(pgsTypes::Ahead);
            }
            else
            {
               pStartSpacing = pStartTS->GetSegmentSpacing();
            }

            if ( pEndPier )
            {
               pEndSpacing = pEndPier->GetGirderSpacing(pgsTypes::Back);
            }
            else
            {
               pEndSpacing = pEndTS->GetSegmentSpacing();
            }


            glFactory->put_StartPierID( startID );
            glFactory->put_EndPierID(   endID   );
            glFactory->put_Type(glChord);
            glFactory->put_StartMeasurementType( pStartSpacing->GetMeasurementType() == pgsTypes::AlongItem ? mtAlongItem : mtNormal );
            glFactory->put_StartMeasurementLocation( pStartSpacing->GetMeasurementLocation() == pgsTypes::AtPierLine ? mlPierLine : mlCenterlineBearing );
            glFactory->put_EndMeasurementType( pEndSpacing->GetMeasurementType() == pgsTypes::AlongItem ? mtAlongItem : mtNormal );
            glFactory->put_EndMeasurementLocation( pEndSpacing->GetMeasurementLocation() == pgsTypes::AtPierLine ? mlPierLine : mlCenterlineBearing );
            pBridgeGeometry->CreateGirderLines(glFactory);
            glFactory->Reset();
         }
      }
   } // end of group loop

   return true;
}

void CBridgeGeometryModelBuilder::GetPierID(const CPrecastSegmentData* pSegment,PierIDType* pStartID,PierIDType* pEndID)
{
   const CClosureJointData* pStartClosure  = pSegment->GetStartClosure();
   const CClosureJointData* pEndClosure = pSegment->GetEndClosure();

   if ( pStartClosure == nullptr )
   {
      *pStartID = ::GetPierLineID( pSegment->GetGirder()->GetPier(pgsTypes::metStart)->GetIndex() );
   }
   else
   {
      if ( pStartClosure->GetPier() )
      {
         *pStartID = ::GetPierLineID( pStartClosure->GetPier()->GetIndex() );
      }
      else
      {
         *pStartID = ::GetTempSupportLineID( pStartClosure->GetTemporarySupport()->GetIndex() );
      }
   }


   if ( pEndClosure == nullptr )
   {
      *pEndID = ::GetPierLineID( pSegment->GetGirder()->GetPier(pgsTypes::metEnd)->GetIndex() );
   }
   else
   {
      if ( pEndClosure->GetPier() )
      {
         *pEndID = ::GetPierLineID( pEndClosure->GetPier()->GetIndex() );
      }
      else
      {
         *pEndID = ::GetTempSupportLineID( pEndClosure->GetTemporarySupport()->GetIndex() );
      }
   }
}

void CBridgeGeometryModelBuilder::GetPierDirection(IAlignment* pAlignment,const CPierData2* pPier,IAngle** ppSkew,IDirection** ppDirection)
{
   Float64 station = pPier->GetStation();
   LPCTSTR strOrientation = pPier->GetOrientation();

   GetSkewAndDirection(pAlignment,station,strOrientation,ppSkew,ppDirection);
}

void CBridgeGeometryModelBuilder::GetTempSupportDirection(IAlignment* pAlignment,const CTemporarySupportData* pTS,IAngle** ppSkew,IDirection** ppDirection)
{
   Float64 station = pTS->GetStation();
   LPCTSTR strOrientation = pTS->GetOrientation();
   GetSkewAndDirection(pAlignment,station,strOrientation,ppSkew,ppDirection);
}

void CBridgeGeometryModelBuilder::GetSkewAndDirection(IAlignment* pAlignment,Float64 station,LPCTSTR strOrientation,IAngle** ppSkew,IDirection** ppDirection)
{
   // get pier direction and skew angle
   // this is needed to determine the pier width and offset measured along CL pier
   pAlignment->GetDirection(CComVariant(station),CComBSTR(strOrientation),ppDirection);

   CComPtr<IDirection> normal; // normal to the alignment
   pAlignment->Normal(CComVariant(station),&normal);
   normal->IncrementBy(CComVariant(M_PI)); // We want the normal to the left... Increment by 180 degrees

   (*ppDirection)->AngleBetween(normal,ppSkew);
}

bool CBridgeGeometryModelBuilder::LayoutDiaphragmLines(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry)
{
#pragma Reminder("IMPLEMENT")
   // need to add diaphragms to the bridge geometry model
   // though there is no point in doing this unless the generic bridge model or
   // the bridge agent make use of the diaphragm geometry.
   //
   // also, the software can be updated to accomodate more general diaphragm layouts including
   // staggered diaphragm lines.
   return true;
}

void CBridgeGeometryModelBuilder::GetSpacingDataAtPier(IBridgeGeometry* pBridgeGeometry,Float64 alignmentOffset,const CPierData2* pPier,pgsTypes::PierFaceType pierFace,
                                                       Float64* pMeasureStation,IDirection** ppMeasureDirection,IDirection** ppSupportDirection,const CGirderSpacing2** ppSpacing)
{
   const CGirderSpacing2* pSpacing = pPier->GetGirderSpacing(pierFace);
   Float64 measureStation;
   Float64 pierStation = pPier->GetStation();

   CComPtr<IAlignment> alignment;
   pBridgeGeometry->get_BridgeAlignment(&alignment);

   CComPtr<IAngle> skew;
   CComPtr<IDirection> pierDirection;
   GetPierDirection(alignment,pPier,&skew,&pierDirection);

   if ( pSpacing->GetMeasurementLocation() == pgsTypes::AtPierLine )
   {
      // Spacing measured at centerline of pier
      measureStation = pierStation;
   }
   else
   {
      // Spacing measured at centerline of bearing
      // Support station is where a line parallel to the CL Pier, offset by +/- bearing offset, 
      // intersects the alignment

      // Bearing offset must be adjusted for how it is measured
      Float64 brgOffset;
      ConnectionLibraryEntry::BearingOffsetMeasurementType measureType;
      pPier->GetBearingOffset(pierFace,&brgOffset,&measureType);

      if ( measureType == ConnectionLibraryEntry::AlongGirder)
      {
         // assumes CL girder is parallel to the alignment
         Float64 skew_angle;
         skew->get_Value(&skew_angle);
         brgOffset *= cos(skew_angle);
      }

      PierIDType pierID = ::GetPierLineID(pPier->GetIndex());
      CComPtr<IPierLine> pierLine;
      pBridgeGeometry->FindPierLine( pierID, &pierLine );

      CComPtr<ILine2d> centerlinePier;
      pierLine->get_Centerline(&centerlinePier);

      CComPtr<IGeomUtil2d> geomUtil;
      geomUtil.CoCreateInstance(CLSID_GeomUtil);

      Float64 sign = (pierFace == pgsTypes::Back ? -1 : 1);

      CComPtr<ILine2d> brgLine;
      geomUtil->CreateParallelLine(centerlinePier,sign*brgOffset,&brgLine);

      CComPtr<IPoint2d> pntAlignment;
      pierLine->get_AlignmentPoint(&pntAlignment);

      CComPtr<IPoint2d> pnt;
      alignment->Intersect(brgLine,pntAlignment,&pnt);

      CComPtr<IStation> station;
      Float64 offset;
      alignment->Offset(pnt,&station,&offset);

      ATLASSERT(IsZero(offset));

      station->get_NormalizedValue(alignment,&measureStation);
   }

   CComPtr<IDirection> measureDirection;
   if ( pSpacing->GetMeasurementType() == pgsTypes::NormalToItem )
   {
      // spacing is measured normal to the alignment
      alignment->Normal(CComVariant(measureStation),&measureDirection);
   }
   else
   {
      // spacing is measured along CL Pier or CL Bearing
      // CL Bearing is parallel to CL Pier so always measure direction at the CL Pier station
      alignment->GetDirection(CComVariant(pierStation),CComBSTR(pPier->GetOrientation()),&measureDirection);
   }

   *pMeasureStation = measureStation;
   measureDirection.CopyTo(ppMeasureDirection);
   pierDirection.CopyTo(ppSupportDirection);
   *ppSpacing = pSpacing;
}

void CBridgeGeometryModelBuilder::GetSpacingDataAtTempSupport(IBridgeGeometry* pBridgeGeometry,Float64 alignmentOffset,const CTemporarySupportData* pTS,
                                                       Float64* pMeasureStation,IDirection** ppMeasureDirection,IDirection** ppSupportDirection,const CGirderSpacing2** ppSpacing)
{
   const CGirderSpacing2* pSpacing = pTS->GetSegmentSpacing();

   // spacing is always measured at CL temporary support
   ATLASSERT(pSpacing->GetMeasurementLocation() == pgsTypes::AtPierLine);

   CComPtr<IAlignment> alignment;
   pBridgeGeometry->get_BridgeAlignment(&alignment);

   CComPtr<IAngle> skew;
   CComPtr<IDirection> tsDirection;
   GetTempSupportDirection(alignment,pTS,&skew,&tsDirection);

   Float64 measureStation = pTS->GetStation();
   CComPtr<IDirection> measureDirection;
   if ( pSpacing->GetMeasurementType() == pgsTypes::NormalToItem )
   {
      // spacing is measured normal to the alignment
      alignment->Normal(CComVariant(measureStation),&measureDirection);
   }
   else
   {
      // spacing is measured along CL Temp Support
      measureDirection = tsDirection;
   }

   *pMeasureStation = measureStation;
   measureDirection.CopyTo(ppMeasureDirection);
   tsDirection.CopyTo(ppSupportDirection);
   *ppSpacing = pSpacing;
}

void CBridgeGeometryModelBuilder::ResolveSegmentSpacing(IBridgeGeometry* pBridgeGeometry,Float64 alignmentOffset,const CPrecastSegmentData* pSegment,IPoint2dCollection** ppStartPoints,IPoint2dCollection** ppEndPoints)
{
   const CGirderSpacing2* pStartSpacing;
   const CGirderSpacing2* pEndSpacing;

   Float64 startMeasureStation, endMeasureStation;
   CComPtr<IDirection> startMeasureDirection, endMeasureDirection;
   CComPtr<IDirection> startSupportDirection, endSupportDirection;


   const CPierData2* pStartPier = nullptr;
   const CTemporarySupportData* pStartTS = nullptr;
   pSegment->GetSupport(pgsTypes::metStart,&pStartPier,&pStartTS);
   if ( pStartPier )
   {
      GetSpacingDataAtPier(pBridgeGeometry,alignmentOffset,pStartPier,pgsTypes::Ahead,&startMeasureStation,&startMeasureDirection,&startSupportDirection,&pStartSpacing);
   }
   else
   {
      GetSpacingDataAtTempSupport(pBridgeGeometry,alignmentOffset,pStartTS,&startMeasureStation,&startMeasureDirection,&startSupportDirection,&pStartSpacing);
   }

   const CPierData2* pEndPier = nullptr;
   const CTemporarySupportData* pEndTS = nullptr;
   pSegment->GetSupport(pgsTypes::metEnd,&pEndPier,&pEndTS);
   if ( pEndPier )
   {
      GetSpacingDataAtPier(pBridgeGeometry,alignmentOffset,pEndPier,pgsTypes::Back,&endMeasureStation,&endMeasureDirection,&endSupportDirection,&pEndSpacing);
   }
   else
   {
      GetSpacingDataAtTempSupport(pBridgeGeometry,alignmentOffset,pEndTS,&endMeasureStation,&endMeasureDirection,&endSupportDirection,&pEndSpacing);
   }

   ResolveSegmentSpacing(pBridgeGeometry,alignmentOffset,
                         startMeasureStation,
                         startMeasureDirection,
                         startSupportDirection,
                         pStartSpacing,
                         endMeasureStation,
                         endMeasureDirection,
                         endSupportDirection,
                         pEndSpacing,
                         ppStartPoints,
                         ppEndPoints);
}

Float64 CBridgeGeometryModelBuilder::GetSkewAngle(IAlignment* pAlignment,Float64 measureStation,IDirection* pMeasureDirection)
{
   CComPtr<IDirection> normal;
   pAlignment->Normal(CComVariant(measureStation),&normal);
   normal->IncrementBy(CComVariant(M_PI));

   CComPtr<IAngle> angle;
   normal->AngleBetween(pMeasureDirection,&angle);

   Float64 skew;
   angle->get_Value(&skew);
   return skew;
}

Float64 CBridgeGeometryModelBuilder::GetLeftGirderOffset(IAlignment* pAlignment,
                                                         Float64 alignmentOffset,
                                                         Float64 measureStation,
                                                         IDirection* pMeasureDirection,
                                                         const CGirderSpacing2* pSpacing)
{
   // returns the offset from the alignment to the left-most girder

   GirderIndexType refGirderIdx = pSpacing->GetRefGirder();
   Float64 refGirderOffset = pSpacing->GetRefGirderOffset();
   pgsTypes::OffsetMeasurementType refGirderOffsetType = pSpacing->GetRefGirderOffsetType();

   // convert ref girder offset into an offset to the left-most girder
   if ( refGirderIdx == INVALID_INDEX )
   {
      // reference girder is the center of the spacing group
      Float64 width = pSpacing->GetSpacingWidth();
      refGirderOffset -= width/2;
      refGirderIdx = 0;
   }
   else
   {
      // a specific girder is the reference girder
      Float64 width = pSpacing->GetSpacingWidthToGirder(refGirderIdx);
      refGirderOffset -= width;
      refGirderIdx = 0;
   }

   // convert offset so that it is measured along the centerline of the support
   pgsTypes::MeasurementType measureType = pSpacing->GetMeasurementType();
   if ( measureType == pgsTypes::NormalToItem )
   {
      // spacing is normal to alignment
      // get skew angle and adjust the ref girder offset
      Float64 skew = GetSkewAngle(pAlignment,measureStation,pMeasureDirection);
      refGirderOffset /= cos(skew);
   }

   // if the ref girder is measured from the bridge, convert it to being measured from the alignment
   if ( refGirderOffsetType == pgsTypes::omtBridge )
   {
      refGirderOffset += alignmentOffset;
   }

   return refGirderOffset;
}

void CBridgeGeometryModelBuilder::ResolveSegmentSpacing(IBridgeGeometry* pBridgeGeometry,
                                                        Float64 alignmentOffset,
                                                        Float64 startMeasureStation,
                                                        IDirection* pStartMeasureDirection,
                                                        IDirection* pStartSupportDirection,
                                                        const CGirderSpacing2* pStartSpacing,
                                                        Float64 endMeasureStation,
                                                        IDirection* pEndMeasureDirection,
                                                        IDirection* pEndSupportDirection,
                                                        const CGirderSpacing2* pEndSpacing,
                                                        IPoint2dCollection** ppStartPoints,
                                                        IPoint2dCollection** ppEndPoints)
{
   // NOTE: supportStation is the station where the girder spacing is measured. It could be at the
   // center of the support or at the centerline of bearing. You must check the girder spacing object
   // and use the appropreate value

   if ( *ppStartPoints == nullptr )
   {
      // if array is not given, create it
      CComPtr<IPoint2dCollection> points;
      points.CoCreateInstance(CLSID_Point2dCollection);
      points.CopyTo(ppStartPoints);
   }
   else
   {
      // otherwise clear it
      (*ppStartPoints)->Clear();
   }

   if ( *ppEndPoints == nullptr )
   {
      // if array is not given, create it
      CComPtr<IPoint2dCollection> points;
      points.CoCreateInstance(CLSID_Point2dCollection);
      points.CopyTo(ppEndPoints);
   }
   else
   {
      // otherwise clear it
      (*ppEndPoints)->Clear();
   }

   CComPtr<IAlignment> alignment;
   pBridgeGeometry->get_BridgeAlignment(&alignment);

   //
   // Get distance from alignment to left-most girder
   //

   Float64 startRefGirderOffset = GetLeftGirderOffset(alignment, alignmentOffset, startMeasureStation, pStartMeasureDirection, pStartSpacing);
   Float64 endRefGirderOffset   = GetLeftGirderOffset(alignment, alignmentOffset, endMeasureStation,   pEndMeasureDirection,   pEndSpacing);


   //
   // Using distance from the alignment to each girder and the support direction, locate
   // the intersection of the centerline of the support and girder
   //

   CComPtr<ICogoModel> cogoModel;
   pBridgeGeometry->get_CogoModel(&cogoModel);

   CComPtr<ICogoEngine> cogoEngine;
   cogoModel->get_Engine(&cogoEngine);

   CComPtr<ILocate2> locate;
   cogoEngine->get_Locate(&locate);

   CComPtr<IPoint2d> startAlignmentPnt;
   alignment->LocatePoint(CComVariant(startMeasureStation),omtNormal,0.0,CComVariant(0),&startAlignmentPnt);

   CComPtr<IPoint2d> pntOnStartMeasurementLine;
   locate->ByDistDir(startAlignmentPnt,-startRefGirderOffset,CComVariant(pStartMeasureDirection),0.0,&pntOnStartMeasurementLine);

   //CComPtr<IDirection> normalToStartMeasurementLine;
   //pStartMeasureDirection->Clone(&normalToStartMeasurementLine);
   //normalToStartMeasurementLine->IncrementBy(CComVariant(-PI_OVER_2));


   CComPtr<IPoint2d> endAlignmentPnt;
   alignment->LocatePoint(CComVariant(endMeasureStation),omtNormal,0.0,CComVariant(0),&endAlignmentPnt);

   CComPtr<IPoint2d> pntOnEndMeasurementLine;
   locate->ByDistDir(endAlignmentPnt,-endRefGirderOffset,CComVariant(pEndMeasureDirection),0.0,&pntOnEndMeasurementLine);

   //CComPtr<IDirection> normalToEndMeasurementLine;
   //pEndMeasureDirection->Clone(&normalToEndMeasurementLine);
   //normalToEndMeasurementLine->IncrementBy(CComVariant(-PI_OVER_2));

   CComPtr<IMeasure2> measure;
   cogoEngine->get_Measure(&measure);
   CComPtr<IDirection> dirGirder;
   measure->Direction(pntOnStartMeasurementLine,pntOnEndMeasurementLine,&dirGirder);

   CComPtr<IIntersect2> intersect;
   cogoEngine->get_Intersect(&intersect);

   //// intersect a line passing through the point on the measurement line that is perpendicular to the measurement line
   //// with a line passing through the alignment at the direction of the bearing line... the intersection point
   //// is where the girder line touches the bearing line
   //CComPtr<IPoint2d> startPnt;
   //intersect->Bearings(pntOnStartMeasurementLine,CComVariant(normalToStartMeasurementLine),0.0,startAlignmentPnt,CComVariant(pStartSupportDirection),0.0,&startPnt);
   //(*ppStartPoints)->Add(startPnt);

   //CComPtr<IPoint2d> endPnt;
   //intersect->Bearings(pntOnEndMeasurementLine,CComVariant(normalToEndMeasurementLine),0.0,endAlignmentPnt,CComVariant(pEndSupportDirection),0.0,&endPnt);
   //(*ppEndPoints)->Add(endPnt);

   // intersectio a line passing through the two points on the girder line with the measurement lines
   CComPtr<IPoint2d> startPnt;
   intersect->Bearings(pntOnStartMeasurementLine,CComVariant(dirGirder),0.0,startAlignmentPnt,CComVariant(pStartSupportDirection),0.0,&startPnt);
   (*ppStartPoints)->Add(startPnt);

   CComPtr<IPoint2d> endPnt;
   intersect->Bearings(pntOnEndMeasurementLine,CComVariant(dirGirder),0.0,endAlignmentPnt,CComVariant(pEndSupportDirection),0.0,&endPnt);
   (*ppEndPoints)->Add(endPnt);

   const CBridgeDescription2* pBridgeDesc = pStartSpacing->GetBridgeDescription();
   pgsTypes::SupportedBeamSpacing spacingType = pBridgeDesc->GetGirderSpacingType();

   Float64 start_offset = startRefGirderOffset;
   Float64 end_offset   = endRefGirderOffset;
   Float64 start_skew = (pStartSpacing->GetMeasurementType() == pgsTypes::NormalToItem ? GetSkewAngle(alignment,startMeasureStation,pStartMeasureDirection) : 0);
   Float64 end_skew   = (pEndSpacing->GetMeasurementType()   == pgsTypes::NormalToItem ? GetSkewAngle(alignment,endMeasureStation,  pEndMeasureDirection) : 0);
   Float64 start_cosine_skew = cos(start_skew);
   Float64 end_cosine_skew   = cos(end_skew);
   SpacingIndexType nSpaces = pStartSpacing->GetSpacingCount();
   ATLASSERT(nSpaces == pEndSpacing->GetSpacingCount());
   for ( SpacingIndexType spaceIdx = 0; spaceIdx < nSpaces; spaceIdx++ )
   {
      Float64 start_space = pStartSpacing->GetGirderSpacing(spaceIdx);
      start_space /= start_cosine_skew;
      start_offset += start_space;

      Float64 end_space = pEndSpacing->GetGirderSpacing(spaceIdx);
      end_space /= end_cosine_skew;
      end_offset += end_space;

      if ( ::IsJointSpacing(spacingType) )
      {
         // spacing is a joint spacing
         // need to add half of the width of the girder on each side of the joint
         GirderIndexType leftGdrIdx  = spaceIdx;
         GirderIndexType rightGdrIdx = leftGdrIdx + 1;

         const CGirderGroupData* pGroup = GetGirderGroup(pStartSpacing);

         const CSplicedGirderData* pLeftGirder  = pGroup->GetGirder(leftGdrIdx);
         const CSplicedGirderData* pRightGirder = pGroup->GetGirder(rightGdrIdx);

         Float64 left_width  = GetGirderWidth(pLeftGirder);
         Float64 right_width = GetGirderWidth(pRightGirder);

         Float64 width = (left_width + right_width)/2;


         // this is not the skew angle we want... width is measured normal to the CL segment
         // the skew angle we need is the angle between the support line and the CL segment
         // this skew angle is the one the measure line makes with the alignment

         // use startPnt and endPnt to get the direction of the girder
         // then get the angle between the girder line direction and the pStartMeasureDirection and pEndMeasureDirection
         // this is the skew angle we want
         //CComPtr<IMeasure2> measure;
         //cogoEngine->get_Measure(&measure);

         CComPtr<IDirection> gdrDirection;
         measure->Direction(startPnt,endPnt,&gdrDirection);
         
         // we want the angle between the normal to the measurement line and the CL Girder.
         // It is easier to rotate the CL girder by 90 degrees, than to rotate the two
         // measurement lines... additionally, the measurement lines are used for other purposes
         // so we would have to rotate them back as well.
         gdrDirection->IncrementBy(CComVariant(PI_OVER_2));

         CComPtr<IAngle> startAngle, endAngle;
         pStartMeasureDirection->AngleBetween(gdrDirection,&startAngle);
         pEndMeasureDirection->AngleBetween(gdrDirection,&endAngle);

         Float64 start_girder_skew;
         startAngle->get_Value(&start_girder_skew);

         Float64 end_girder_skew;
         endAngle->get_Value(&end_girder_skew);

         start_offset += width/cos(start_girder_skew);
         end_offset   += width/cos(end_girder_skew);
      }

      pntOnStartMeasurementLine.Release();
      locate->ByDistDir(startAlignmentPnt,-start_offset,CComVariant(pStartMeasureDirection),0.0,&pntOnStartMeasurementLine);

      //startPnt.Release();
      //intersect->Bearings(pntOnStartMeasurementLine,CComVariant(normalToStartMeasurementLine),0.0,startAlignmentPnt,CComVariant(pStartSupportDirection),0.0,&startPnt);

      //(*ppStartPoints)->Add(startPnt);


      pntOnEndMeasurementLine.Release();
      locate->ByDistDir(endAlignmentPnt,-end_offset,CComVariant(pEndMeasureDirection),0.0,&pntOnEndMeasurementLine);

      //endPnt.Release();
      //intersect->Bearings(pntOnEndMeasurementLine,CComVariant(normalToEndMeasurementLine),0.0,endAlignmentPnt,CComVariant(pEndSupportDirection),0.0,&endPnt);

      //(*ppEndPoints)->Add(endPnt);

      dirGirder.Release();
      measure->Direction(pntOnStartMeasurementLine,pntOnEndMeasurementLine,&dirGirder);

      startPnt.Release();
      intersect->Bearings(pntOnStartMeasurementLine,CComVariant(dirGirder),0.0,startAlignmentPnt,CComVariant(pStartSupportDirection),0.0,&startPnt);

      (*ppStartPoints)->Add(startPnt);


      pntOnEndMeasurementLine.Release();
      locate->ByDistDir(endAlignmentPnt,-end_offset,CComVariant(pEndMeasureDirection),0.0,&pntOnEndMeasurementLine);

      endPnt.Release();
      intersect->Bearings(pntOnEndMeasurementLine,CComVariant(dirGirder),0.0,endAlignmentPnt,CComVariant(pEndSupportDirection),0.0,&endPnt);

      (*ppEndPoints)->Add(endPnt);
   }
}

const CGirderGroupData* CBridgeGeometryModelBuilder::GetGirderGroup(const CGirderSpacing2* pSpacing)
{
   const CSpanData2* pSpan;
   const CPierData2* pPier = pSpacing->GetPier();
   if ( pPier )
   {
      if ( pPier->GetGirderSpacing(pgsTypes::Back) == pSpacing )
      {
         pSpan = pPier->GetPrevSpan();
      }
      else
      {
         pSpan = pPier->GetNextSpan();
      }
   }
   else
   {
      const CTemporarySupportData* pTS = pSpacing->GetTemporarySupport();
      pSpan = pTS->GetSpan();
   }

   const CGirderGroupData* pGroup = pSpacing->GetBridgeDescription()->GetGirderGroup(pSpan);
   return pGroup;
}

Float64 CBridgeGeometryModelBuilder::GetGirderWidth(const CSplicedGirderData* pGirder)
{
   const GirderLibraryEntry* pLibEntry = pGirder->GetGirderLibraryEntry();
   return pLibEntry->GetBeamWidth(pgsTypes::metStart);
}

void CBridgeGeometryModelBuilder::GetPierLineProperties(const CBridgeDescription2* pBridgeDesc,const CGirderSpacing2* pSpacing,Float64 skewAngle,Float64* pWidth,Float64* pOffset)
{
   Float64 width = pSpacing->GetSpacingWidth();

   Float64 cosine_skew_angle = cos(skewAngle);

   GirderIndexType refGirderIdx = pSpacing->GetRefGirder();
   Float64 refGirderOffset = pSpacing->GetRefGirderOffset();
   pgsTypes::OffsetMeasurementType refGirderOffsetType = pSpacing->GetRefGirderOffsetType();

   // convert ref girder offset into an offset to the left-most girder
   if ( refGirderIdx == INVALID_INDEX )
   {
      // reference girder is the center of the spacing group
      refGirderOffset -= width/2;
      refGirderIdx = 0;
   }
   else
   {
      // a specific girder is the reference girder
      refGirderOffset -= pSpacing->GetSpacingWidthToGirder(refGirderIdx);
      refGirderIdx = 0;
   }

   // if the ref girder is measured from the bridge, convert it to being measured from the alignment
   if ( refGirderOffsetType == pgsTypes::omtBridge )
   {
      Float64 alignment_offset = pBridgeDesc->GetAlignmentOffset();

      if ( pSpacing->GetMeasurementType() == pgsTypes::AlongItem )
      {
         alignment_offset /= cosine_skew_angle;
      }

      refGirderOffset += alignment_offset;
   }

   if ( pSpacing->GetMeasurementType() == pgsTypes::NormalToItem )
   {
      // convert spacing to along pier
      width           /= cosine_skew_angle;
      refGirderOffset /= cosine_skew_angle;
   }

   *pWidth = width;
   *pOffset = refGirderOffset;
}
