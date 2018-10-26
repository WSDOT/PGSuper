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

#include "stdafx.h"
#include "BridgeGeometryModelBuilder.h"
#include "BridgeHelpers.h"

#include <PgsExt\ClosureJointData.h>
#include <PsgLib\GirderLibraryEntry.h>

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
   pBridgeGeometry->put_AlignmentOffset(-pBridgeDesc->GetAlignmentOffset());

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
   // use pier index as the pier ID
   const CPierData2* pPier = pBridgeDesc->GetPier(0);
   PierIndexType pierIdx = 0;
   while ( pPier )
   {
      PierIDType pierID = ::GetPierLineID(pierIdx);

      Float64 station = pPier->GetStation();
      std::_tstring strOrientation = pPier->GetOrientation();

      CComPtr<IAlignment> alignment;
      pBridgeGeometry->get_BridgeAlignment(&alignment);

      CComPtr<IAngle> skew;
      GetPierSkewAngle(alignment,pPier,&skew);

      Float64 skew_angle;
      skew->get_Value(&skew_angle);

      Float64 cosine_skew_angle = cos(skew_angle);

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
         back_width = Max(back_width,pSpacing->GetSpacingWidth());
   
         GirderIndexType refGirderIdx = pSpacing->GetRefGirder();
         Float64 refGirderOffset = pSpacing->GetRefGirderOffset();
         pgsTypes::OffsetMeasurementType refGirderOffsetType = pSpacing->GetRefGirderOffsetType();

         // convert ref girder offset into an offset to the left-most girder
         if ( refGirderIdx == INVALID_INDEX )
         {
            // reference girder is the center of the spacing group
            refGirderOffset -= back_width/2;
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
            back_width      /= cosine_skew_angle;
            refGirderOffset /= cosine_skew_angle;
         }

         back_offset = -refGirderOffset;
      }

      if ( pNextSpan )
      {
         const CGirderSpacing2* pSpacing = pPier->GetGirderSpacing(pgsTypes::Ahead);
         ahead_width = Max(ahead_width,pSpacing->GetSpacingWidth());
   
         GirderIndexType refGirderIdx = pSpacing->GetRefGirder();
         Float64 refGirderOffset = pSpacing->GetRefGirderOffset();
         pgsTypes::OffsetMeasurementType refGirderOffsetType = pSpacing->GetRefGirderOffsetType();

         // convert ref girder offset into an offset to the left-most girder
         if ( refGirderIdx == INVALID_INDEX )
         {
            // reference girder is the center of the spacing group
            refGirderOffset -= ahead_width/2;
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
            ahead_width     /= cosine_skew_angle;
            refGirderOffset /= cosine_skew_angle;
         }

         ahead_offset = -refGirderOffset;
      }

      Float64 width,offset;
      if ( back_width < ahead_width )
      {
         width  = ahead_width;
         offset = ahead_offset;
      }
      else
      {
         width  = back_width;
         offset = back_offset;
      }

      CComPtr<IPierLine> pierline;
      pBridgeGeometry->CreatePierLine(pierID,m_AlignmentID,CComVariant(station),CComBSTR(strOrientation.c_str()),width,offset,&pierline);

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
         pPier = NULL;
      }

      pierIdx++;
   }

   return true;
}

bool CBridgeGeometryModelBuilder::LayoutTemporarySupports(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry)
{
   SupportIndexType nTS = pBridgeDesc->GetTemporarySupportCount();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
#pragma Reminder("UPDATE: Find a way to use the same Temporary Support ID for the bridge and geometry models")
      SupportIDType tsID = ::GetTempSupportLineID(tsIdx); // this is the ID of the line that represents the temporary
                                                          // support in the geometry model, no the ID of the temporary support
                                                          // in the bridge data model

      const CTemporarySupportData* pTS = pBridgeDesc->GetTemporarySupport(tsIdx);
      Float64 station = pTS->GetStation();
      std::_tstring strOrientation = pTS->GetOrientation();

      Float64 width = pTS->GetSegmentSpacing()->GetSpacingWidth();

#pragma Reminder("UPDATE: need to get temporary support girder spacing offset")
#pragma Reminder("UPDATE: need to adjust width and offset for ref girder and spacing measurement type")
      Float64 offset = 0;

      CComPtr<IPierLine> tsLine;
      pBridgeGeometry->CreatePierLine(tsID,m_AlignmentID,CComVariant(station),CComBSTR(strOrientation.c_str()),width,offset,&tsLine);

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
   pgsTypes::MeasurementLocation measureLocation = pBridgeDesc->GetMeasurementLocation();
   pgsTypes::MeasurementType measureType = pBridgeDesc->GetMeasurementType();

   if ( !pBridgeDesc->UseSameNumberOfGirdersInAllGroups() ||
        !pBridgeDesc->UseSameGirderForEntireBridge()      || 
        measureType == mtAlongItem )
   {
      // if there is a different number of girders in each group, but the spacing is uniform, this is the
      // same as general spacing... use the general spacing function and return

      // or
      
      // if a different girder types are used, the girders aren't necessarily the same width so
      // this is the same as general spacing

      // or
      
      // the girder spacing is measured along the CL pier or CL bearing. if the piers are skewed or
      // if the alignment is curved, the girder spacing along the alignment normal will not be uniform
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

   if ( measureType == pgsTypes::AlongItem )
   {
      // girder spacing is measured along CL pier
      // adjust the spacing and the reference girder offset so
      // that it is measured normal to the alignment
#pragma Reminder("IMPLEMENT")
      ATLASSERT(false); // need to implement this
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
   glFactory.CoCreateInstance(CLSID_SingleGirderLineFactory);

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

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
#pragma Reminder("Assuming same number of segments in all girders in a group")
      const CSplicedGirderData* pGirder = pGroup->GetGirder(0);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      std::vector<CComPtr<IPoint2dCollection>> girderPoints(nSegments+1); // spacing is measured at end of every segment

      const CPierData2* pStartPier = pGroup->GetPier(pgsTypes::metStart);
      const CPierData2* pEndPier   = pGroup->GetPier(pgsTypes::metEnd);

      const CGirderSpacing2* pStartSpacing = pStartPier->GetGirderSpacing(pgsTypes::Ahead);
      const CGirderSpacing2* pEndSpacing   = pEndPier->GetGirderSpacing(pgsTypes::Back);

      //
      // Resolve girder spacing at segments ends
      //

      ResolveGirderSpacingAtPier(alignment,alignmentOffset,pStartPier,pgsTypes::Ahead,&girderPoints[0],pBridgeGeometry);

      for ( SegmentIndexType segIdx = 1; segIdx < nSegments; segIdx++ )
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
         const CClosureJointData* pClosure = pSegment->GetLeftClosure();
         const CPierData2* pPier           = pClosure->GetPier();
         const CTemporarySupportData* pTS = pClosure->GetTemporarySupport();
         if ( pPier )
         {
            ResolveGirderSpacingAtPier(alignment,alignmentOffset,pPier,pgsTypes::Back,&girderPoints[segIdx],pBridgeGeometry);
         }
         else
         {
            ResolveGirderSpacingAtTempSupport(alignment,alignmentOffset,pTS,&girderPoints[segIdx],pBridgeGeometry);
         }
      }

      ResolveGirderSpacingAtPier(alignment,alignmentOffset,pEndPier,pgsTypes::Back,&girderPoints[nSegments],pBridgeGeometry);

   #if defined _DEBUG
      // verify all girder offset arrays are the same length
      CollectionIndexType size1;
      girderPoints[0]->get_Count(&size1);
      for ( CollectionIndexType i = 1; i < nSegments; i++ )
      {
         CollectionIndexType size2;
         girderPoints[i]->get_Count(&size2);
         ATLASSERT(size1 == size2);
         size1 = size2;
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
            girderPoints[segIdx]->get_Item(gdrIdx,&pntStart);
            girderPoints[segIdx+1]->get_Item(gdrIdx,&pntEnd);

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
   const CClosureJointData* pLeftClosure  = pSegment->GetLeftClosure();
   const CClosureJointData* pRightClosure = pSegment->GetRightClosure();

   if ( pLeftClosure == NULL )
   {
      *pStartID = ::GetPierLineID( pSegment->GetGirder()->GetPier(pgsTypes::metStart)->GetIndex() );
   }
   else
   {
      if ( pLeftClosure->GetPier() )
      {
         *pStartID = ::GetPierLineID( pLeftClosure->GetPier()->GetIndex() );
      }
      else
      {
         *pStartID = ::GetTempSupportLineID( pLeftClosure->GetTemporarySupport()->GetIndex() );
      }
   }


   if ( pRightClosure == NULL )
   {
      *pEndID = ::GetPierLineID( pSegment->GetGirder()->GetPier(pgsTypes::metEnd)->GetIndex() );
   }
   else
   {
      if ( pRightClosure->GetPier() )
      {
         *pEndID = ::GetPierLineID( pRightClosure->GetPier()->GetIndex() );
      }
      else
      {
         *pEndID = ::GetTempSupportLineID( pRightClosure->GetTemporarySupport()->GetIndex() );
      }
   }
}

void CBridgeGeometryModelBuilder::GetPierSkewAngle(IAlignment* pAlignment,const CPierData2* pPier,IAngle** ppSkew)
{
   Float64 station = pPier->GetStation();
   std::_tstring strOrientation = pPier->GetOrientation();

   // get pier direction and skew angle
   // this is needed to determine the pier width and offset measured along CL pier
   CComPtr<IDirection> direction;
   pAlignment->GetDirection(CComVariant(station),CComBSTR(strOrientation.c_str()),&direction);

   CComPtr<IDirection> normal; // normal to the alignment
   pAlignment->Normal(CComVariant(station),&normal);
   normal->IncrementBy(CComVariant(M_PI)); // We want the normal to the left... Increment by 180 degrees

   direction->AngleBetween(normal,ppSkew);
}

bool CBridgeGeometryModelBuilder::LayoutDiaphragmLines(const CBridgeDescription2* pBridgeDesc,IBridgeGeometry* pBridgeGeometry)
{
#pragma Reminder("TODO: Implement")
   // need to add diaphragms to the bridge geometry model
   // though there is no point in doing this unless the generic bridge model or
   // the bridge agent make use of the diaphragm geometry.
   //
   // also, the software can be updated to accomodate more general diaphragm layouts including
   // staggered diaphragm lines.
   return true;
}

void CBridgeGeometryModelBuilder::ResolveGirderSpacingAtPier(IAlignment* pAlignment,Float64 alignmentOffset,const CPierData2* pPier,pgsTypes::PierFaceType pierFace,IPoint2dCollection** ppPoints,IBridgeGeometry* pBridgeGeometry)
{
   const CGirderSpacing2* pSpacing = pPier->GetGirderSpacing(pierFace);
   Float64 measureStation;
   Float64 pierStation = pPier->GetStation();
   if ( pSpacing->GetMeasurementLocation() == pgsTypes::AtPierLine )
   {
      // Spacing measured at centerline of pier
      measureStation = pierStation;
   }
   else
   {
      // Spacing measured at centerline of bearing
      // Support station is the pier station +/- bearing offset. 
      // Bearing offset must be adjusted for how it is measured
      Float64 brgOffset;
      ConnectionLibraryEntry::BearingOffsetMeasurementType measureType;
      pPier->GetBearingOffset(pierFace,&brgOffset,&measureType);

      if ( measureType == ConnectionLibraryEntry::NormalToPier )
      {
         // Bearing offset measured normal to the pier
         CComPtr<IAngle> skew;
         GetPierSkewAngle(pAlignment,pPier,&skew);

         Float64 skew_angle;
         skew->get_Value(&skew_angle);

         brgOffset /= cos(skew_angle);
      }

      measureStation = pierStation + (pierFace == pgsTypes::Back ? -1 : 1)*brgOffset;
   }

   CComPtr<IDirection> measureDirection;
   if ( pSpacing->GetMeasurementType() == pgsTypes::NormalToItem )
   {
      // spacing is measured normal to the alignment
      pAlignment->Normal(CComVariant(measureStation),&measureDirection);
   }
   else
   {
      // spacing is measured along CL Pier or CL Bearing
      // CL Bearing is parallel to CL Pier so always measure direction at the CL Pier station
      pAlignment->GetDirection(CComVariant(pierStation),CComBSTR(pPier->GetOrientation()),&measureDirection);
   }

   ResolveGirderSpacing(pAlignment,alignmentOffset,measureStation,measureDirection,pSpacing,ppPoints,pBridgeGeometry);
}

void CBridgeGeometryModelBuilder::ResolveGirderSpacingAtTempSupport(IAlignment* pAlignment,Float64 alignmentOffset,const CTemporarySupportData* pTS,IPoint2dCollection** ppPoints,IBridgeGeometry* pBridgeGeometry)
{
   const CGirderSpacing2* pSpacing = pTS->GetSegmentSpacing();

   // spacing is always measured at CL temporary support
   ATLASSERT(pSpacing->GetMeasurementLocation() == pgsTypes::AtPierLine);

   Float64 measureStation = pTS->GetStation();
   CComPtr<IDirection> measureDirection;
   if ( pSpacing->GetMeasurementType() == pgsTypes::NormalToItem )
   {
      // spacing is measured normal to the alignment
      pAlignment->Normal(CComVariant(measureStation),&measureDirection);
   }
   else
   {
      // spacing is measured along CL Pier or CL Bearing
      pAlignment->GetDirection(CComVariant(measureStation),CComBSTR(pTS->GetOrientation()),&measureDirection);
   }

   ResolveGirderSpacing(pAlignment,alignmentOffset,measureStation,measureDirection,pSpacing,ppPoints,pBridgeGeometry);
}

void CBridgeGeometryModelBuilder::ResolveGirderSpacing(IAlignment* pAlignment,Float64 alignmentOffset,Float64 measureStation,IDirection* pMeasureDirection,const CGirderSpacing2* pSpacing,IPoint2dCollection** ppPoints,IBridgeGeometry* pBridgeGeometry)
{
   // NOTE: supportStation is the station where the girder spacing is measured. It could be at the
   // center of the support or at the centerline of bearing. You must check the girder spacing object
   // and use the appropreate value

   if ( *ppPoints == NULL )
   {
      // if array is not given, create it
      CComPtr<IPoint2dCollection> points;
      points.CoCreateInstance(CLSID_Point2dCollection);
      points.CopyTo(ppPoints);
   }
   else
   {
      // otherwise clear it
      (*ppPoints)->Clear();
   }

   //
   // Get distance from alignment to left-most girder
   //

   // get ref girder information
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
   Float64 skew = 0;
   Float64 cosine_skew = 1.0;
   if ( measureType == pgsTypes::NormalToItem )
   {
      // spacing is normal to alignment
      // get skew angle
      CComPtr<IDirection> normal;
      pAlignment->Normal(CComVariant(measureStation),&normal);
      normal->IncrementBy(CComVariant(M_PI));

      CComPtr<IAngle> angle;
      normal->AngleBetween(pMeasureDirection,&angle);

      angle->get_Value(&skew);

      // adjust ref girder offset
      cosine_skew = cos(skew);
      refGirderOffset /= cosine_skew;
   }

   // if the ref girder is measured from the bridge, convert it to being measured from the alignment
   if ( refGirderOffsetType == pgsTypes::omtBridge )
   {
      refGirderOffset -= alignmentOffset;
   }
   
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

   CComPtr<IPoint2d> alignmentPnt;
   pAlignment->LocatePoint(CComVariant(measureStation),omtNormal,0.0,CComVariant(0),&alignmentPnt);

   CComPtr<IPoint2d> pnt;
   locate->ByDistDir(alignmentPnt,-refGirderOffset,CComVariant(pMeasureDirection),0.0,&pnt);
   (*ppPoints)->Add(pnt);

   const CBridgeDescription2* pBridgeDesc = pSpacing->GetBridgeDescription();
   pgsTypes::SupportedBeamSpacing spacingType = pBridgeDesc->GetGirderSpacingType();

   Float64 offset = refGirderOffset;
   SpacingIndexType nSpaces = pSpacing->GetSpacingCount();
   for ( SpacingIndexType spaceIdx = 0; spaceIdx < nSpaces; spaceIdx++ )
   {
      Float64 space = pSpacing->GetGirderSpacing(spaceIdx);
      space /= cosine_skew;
      offset += space;

      if ( ::IsJointSpacing(spacingType) )
      {
         // spacing is a joint spacing
         // need to add half of the width of the girder on each side of the joint
         GirderIndexType leftGdrIdx  = spaceIdx;
         GirderIndexType rightGdrIdx = leftGdrIdx + 1;

         const CGirderGroupData* pGroup = GetGirderGroup(pSpacing);

         const CSplicedGirderData* pLeftGirder  = pGroup->GetGirder(leftGdrIdx);
         const CSplicedGirderData* pRightGirder = pGroup->GetGirder(rightGdrIdx);

         Float64 left_width  = GetGirderWidth(pLeftGirder);
         Float64 right_width = GetGirderWidth(pRightGirder);

         Float64 width = (left_width + right_width)/2;
         offset += width/cosine_skew;
      }

      pnt.Release();
      locate->ByDistDir(alignmentPnt,-offset,CComVariant(pMeasureDirection),0.0,&pnt);
      (*ppPoints)->Add(pnt);
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
