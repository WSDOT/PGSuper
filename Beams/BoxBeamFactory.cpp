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

// BoxBeamFactory.cpp : Implementation of CBoxBeamFactory
#include "stdafx.h"
#include <Plugins\Beams.h>

#include <Plugins\BeamFamilyCLSID.h>

#include "BoxBeamFactory.h"
#include "BoxBeamDistFactorEngineer.h"
#include "PsBeamLossEngineer.h"
#include "StrandMoverImpl.h"
#include <GeomModel\PrecastBeam.h>
#include <MathEx.h>
#include <sstream>
#include <algorithm>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <PgsExt\BridgeDescription2.h>

#include <IFace\StatusCenter.h>
#include <PgsExt\StatusItem.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CBoxBeamFactory
HRESULT CBoxBeamFactory::FinalConstruct()
{
   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.push_back(_T("H1"));
   m_DimNames.push_back(_T("H2"));
   m_DimNames.push_back(_T("H3"));
   m_DimNames.push_back(_T("H4"));
   m_DimNames.push_back(_T("H5"));
   m_DimNames.push_back(_T("H6"));
   m_DimNames.push_back(_T("H7"));
   m_DimNames.push_back(_T("W1"));
   m_DimNames.push_back(_T("W2"));
   m_DimNames.push_back(_T("W3"));
   m_DimNames.push_back(_T("W4"));
   m_DimNames.push_back(_T("F1"));
   m_DimNames.push_back(_T("F2"));
   m_DimNames.push_back(_T("C1"));
   m_DimNames.push_back(_T("Jmax"));
   m_DimNames.push_back(_T("ShearKeyDepth"));
   m_DimNames.push_back(_T("EndBlockLength"));

   m_DefaultDims.push_back(::ConvertToSysUnits( 5.0,unitMeasure::Inch)); // H1
   m_DefaultDims.push_back(::ConvertToSysUnits(29.5,unitMeasure::Inch)); // H2
   m_DefaultDims.push_back(::ConvertToSysUnits( 5.0,unitMeasure::Inch)); // H3
   m_DefaultDims.push_back(::ConvertToSysUnits( 4.0,unitMeasure::Inch)); // H4
   m_DefaultDims.push_back(::ConvertToSysUnits( 3.0,unitMeasure::Inch)); // H5
   m_DefaultDims.push_back(::ConvertToSysUnits( 5.0,unitMeasure::Inch)); // H6
   m_DefaultDims.push_back(::ConvertToSysUnits(17.0,unitMeasure::Inch)); // H7
   m_DefaultDims.push_back(::ConvertToSysUnits( 3.0,unitMeasure::Inch)); // W1
   m_DefaultDims.push_back(::ConvertToSysUnits( 5.0,unitMeasure::Inch)); // W2
   m_DefaultDims.push_back(::ConvertToSysUnits(27.75,unitMeasure::Inch));// W3
   m_DefaultDims.push_back(::ConvertToSysUnits( 5.0,unitMeasure::Inch)); // W4
   m_DefaultDims.push_back(::ConvertToSysUnits( 5.0,unitMeasure::Inch)); // F1
   m_DefaultDims.push_back(::ConvertToSysUnits( 5.0,unitMeasure::Inch)); // F2
   m_DefaultDims.push_back(::ConvertToSysUnits( 0.75,unitMeasure::Inch)); // C1
   m_DefaultDims.push_back(::ConvertToSysUnits( 1.0,unitMeasure::Inch)); // Jmax
   m_DefaultDims.push_back(::ConvertToSysUnits( 0.0,unitMeasure::Inch)); // shear key
   m_DefaultDims.push_back(::ConvertToSysUnits( 12.0,unitMeasure::Inch)); // end block

   // SI Units
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H4
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H5
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H6
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H7
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W4
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // F1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // F2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // C1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // Jmax
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // shear key
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // end block

   // US Units
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H4
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H5
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H6
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H7
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W4
   m_DimUnits[1].push_back(&unitMeasure::Inch); // F1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // F2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // C1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // Jmax
   m_DimUnits[1].push_back(&unitMeasure::Inch); // shear key
   m_DimUnits[1].push_back(&unitMeasure::Inch); // end block

   return S_OK;
}

void CBoxBeamFactory::CreateGirderSection(IBroker* pBroker,StatusGroupIDType statusGroupID,const IBeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection)
{
   CComPtr<IBoxBeamSection> gdrsection;
   gdrsection.CoCreateInstance(CLSID_BoxBeamSection);
   CComPtr<IBoxBeam> beam;
   gdrsection->get_Beam(&beam);

   Float64 H1, H2, H3, H4, H5, H6, H7, W1, W2, W3, W4, F1, F2, C1, J, shearKeyDepth, endBlockLength;
   GetDimensions(dimensions,H1, H2, H3, H4, H5, H6, H7, W1, W2, W3, W4, F1, F2, C1, J, shearKeyDepth, endBlockLength);

   beam->put_H1(H1);
   beam->put_H2(H2);
   beam->put_H3(H3);
   beam->put_H4(H4);
   beam->put_H5(H5);
   beam->put_H6(H6);
   beam->put_H7(H7);
   beam->put_W1(W1);
   beam->put_W2(W2);
   beam->put_W3(W3);
   beam->put_W4(W4);
   beam->put_F1(F1);
   beam->put_F2(F2);
   beam->put_C1(C1);

   gdrsection.QueryInterface(ppSection);
}




bool CBoxBeamFactory::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSI,std::_tstring* strErrMsg)
{
   Float64 H1, H2, H3, H4, H5, H6, H7, W1, W2, W3, W4, F1, F2, C1, J, shearKeyDepth, endBlockLength;
   GetDimensions(dimensions,H1, H2, H3, H4, H5, H6, H7, W1, W2, W3, W4, F1, F2, C1, J, shearKeyDepth, endBlockLength);

   if ( H1 <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("H1 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( H2 <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("H2 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( H3 <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("H3 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( H4 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("H4 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( H5 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("H5 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( H6 <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("H6 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( H7 <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("H7 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( W1 <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("W1 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( W2 <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("W2 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( W3 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("W3 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( W4 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("W4 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( F1 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("F1 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( F2 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("F2 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( C1 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("C1 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( C1 >= H7 )
   {
      std::_tostringstream os;
      os << _T("C1 must be less than H7") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( H1+H2+H3+TOLERANCE <= H4+H5+H6+H7 )
   {
      std::_tostringstream os;
      os << _T("H1+H2+H3 must be greater than or equal to H4+H5+H6+H7") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( F1 > W3/2 )
   {
      std::_tostringstream os;
      os << _T("F1 must be less than W3/2") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( F1 > H2/2 )
   {
      std::_tostringstream os;
      os << _T("F1 must be less than H2/2") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( F2 > W3/2 )
   {
      std::_tostringstream os;
      os << _T("F2 must be less than W3/2") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( F2 > H2/2 )
   {
      std::_tostringstream os;
      os << _T("F2 must be less than H2/2") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( J < 0.0 )
   {
      std::_tostringstream os;
      os << _T("Maximum joint size must be zero or greater") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( shearKeyDepth > H1+H2+H3+TOLERANCE )
   {
      std::_tostringstream os;
      os << _T("Shear Key Depth must be less than total section depth") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   return true;
}

void CBoxBeamFactory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions)
{
   std::vector<std::_tstring>::iterator iter;
   pSave->BeginUnit(_T("BoxBeamDimensions"),4.0);
   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::_tstring name = *iter;
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CBoxBeamFactory::LoadSectionDimensions(sysIStructuredLoad* pLoad)
{
   Float64 parent_version;
   if ( pLoad->GetParentUnit() == _T("GirderLibraryEntry") )
      parent_version = pLoad->GetParentVersion();
   else
      parent_version = pLoad->GetVersion();


   IBeamFactory::Dimensions dimensions;

   // In Feb,2008 we created version 10.0 which updated the flawed dimensioning for box beams. This update completely
   // rearranged all box dimensions. 
   if ( parent_version < 10.0 )
   {
      // ordering of old dimensions
      const long NDMS=17;
      //                            0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15   16
      LPCTSTR box_dims[NDMS]={_T("H1"),_T("H2"),_T("H3"),_T("H4"),_T("H5"),_T("H6"),_T("H7"),_T("H8"),_T("W1"),_T("W2"),_T("W3"),_T("W4"),_T("W5"),_T("F1"),_T("F2"),_T("C1"),_T("Jmax")};
      Float64 dim_vals[NDMS];
      for (long id=0; id<NDMS; id++)
      {
         if ( !pLoad->Property(box_dims[id],&(dim_vals[id])) )
         {
            std::_tstring name(box_dims[id]);
            if ( parent_version < 9.0 && name == _T("Jmax"))
            {
               dim_vals[id] = 0.0;
            }
            else if ( parent_version < 3.0 && (name == _T("C1") || name == _T("C2") ) )
            {
               dim_vals[id] = 0.0;
            }
            else
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
      } // end for loop

      // check that redundant dimensions matched up in old section
      // H1 == H6+H7+H8
      ATLASSERT( ::IsEqual(dim_vals[0], dim_vals[5]+dim_vals[6]+dim_vals[7]) );
      // W1-2*W4 == W2-2*W3
      ATLASSERT( ::IsEqual(dim_vals[8]-2*dim_vals[11], dim_vals[9]-2*dim_vals[10]) );

      // Now we have values in old format, convert to new. 
      // Refer to BoxBeam.vsd visio file in documentation for mapping
      Float64 H1, H2, H3, H4, H5, H6, H7, W1, W2, W3, W4, F1, F2, C1, J;

      // H1 = H8
      H1 = dim_vals[7];

      // H2 = H7
      H2 = dim_vals[6];

      // H3 = H6
      H3 = dim_vals[5];

      // H4 = H5
      H4 = dim_vals[4];

      // H5 = H4
      H5 = dim_vals[3];

      // H6 = H3
      H6 = dim_vals[2];

      // H7 = H2
      H7 = dim_vals[1];

      // W1 = W4
      W1 = dim_vals[11];

      // W2 = (W1-2*W4-W5)/2
      W2 = (dim_vals[8] - 2*dim_vals[11] - dim_vals[12])/2.0;

      // W3 = W5
      W3 = dim_vals[12];

      // W4 = W3
      W4 = dim_vals[10];

      // F1 and F2 - a bug found in the WBFL in 10/2011 requires these to be swapped
      F1 = dim_vals[14];
      F2 = dim_vals[13];

      // the rest map 1-1
      C1 = dim_vals[15];
       J = dim_vals[16];

      // now we can set our local dimensions
      dimensions.push_back(Dimension(_T("H1"),H1));
      dimensions.push_back(Dimension(_T("H2"),H2));
      dimensions.push_back(Dimension(_T("H3"),H3));
      dimensions.push_back(Dimension(_T("H4"),H4));
      dimensions.push_back(Dimension(_T("H5"),H5));
      dimensions.push_back(Dimension(_T("H6"),H6));
      dimensions.push_back(Dimension(_T("H7"),H7));
      dimensions.push_back(Dimension(_T("W1"),W1));
      dimensions.push_back(Dimension(_T("W2"),W2));
      dimensions.push_back(Dimension(_T("W3"),W3));
      dimensions.push_back(Dimension(_T("W4"),W4));
      dimensions.push_back(Dimension(_T("F1"),F1));
      dimensions.push_back(Dimension(_T("F2"),F2));
      dimensions.push_back(Dimension(_T("C1"),C1));
      dimensions.push_back(Dimension(_T("Jmax"),J));

      dimensions.push_back(Dimension(_T("ShearKeyDepth"),0.0));
      dimensions.push_back(Dimension(_T("EndBlockLength"),0.0));
   }
   else
   {
      Float64 dimVersion = 1.0;
      if ( 14 <= parent_version )
      {
         if ( pLoad->BeginUnit(_T("BoxBeamDimensions")) )
            dimVersion = pLoad->GetVersion();
         else
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      std::vector<std::_tstring>::iterator iter;
      for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
      {
         std::_tstring name = *iter;
         Float64 value;
         if ( !pLoad->Property(name.c_str(),&value) )
         {
            // failed to read dimension value...
            
            // if this is before dimension data block versio 2 and the
            // dimension is Jmax, the fail to read is expected
            if ( dimVersion < 2 && parent_version < 9.0 && name == _T("Jmax") )
            {
               value = 0.0; // set the default value
            }
            else if ( dimVersion < 2 && parent_version < 3.0 && name == _T("C1") )
            {
               value = 0.0;
            }
            // Added shear key and end block in version 3
            else if ( dimVersion < 3 && name == _T("ShearKeyDepth") )
            {
               value = 0.0;
            }
            else if ( dimVersion < 3 && name == _T("EndBlockLength") )
            {
               value = 0.0;
            }
            else
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }

         dimensions.push_back( Dimension(name,value) );
      }

      if( dimVersion < 4 )
      {
         // A bug in the WBFL had F1 and F2 swapped. Version 4 fixes this
         IBeamFactory::Dimensions::iterator itdF1(dimensions.end()), itdF2(dimensions.end());
         IBeamFactory::Dimensions::iterator itd(dimensions.begin());
         IBeamFactory::Dimensions::iterator itdend(dimensions.end());
         while(itd!=itdend)
         {
            IBeamFactory::Dimension& dims = *itd;
            std::_tstring name = dims.first;

            if( name == _T("F1") )
            {
               itdF1 = itd;
            }
            else if ( name == _T("F2") )
            {
               itdF2 = itd;
            }

            itd++;
         }

         if (itdF1==dimensions.end() || itdF2==dimensions.end())
         {
            ATLASSERT(false); // should never happen F1 and F2 must be in list
         }
         else
         {
            // swap values
            Float64 F1val = itdF1->second;
            Float64 F2val = itdF2->second;

            itdF1->second = F2val;
            itdF2->second = F1val;
         }
      }

      if ( 14 <= parent_version && !pLoad->EndUnit() )
         THROW_LOAD(InvalidFileFormat,pLoad);
   }
   return dimensions;
}


Float64 CBoxBeamFactory::GetSurfaceArea(IBroker* pBroker,const CSegmentKey& segmentKey,bool bReduceForPoorlyVentilatedVoids)
{
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   Float64 perimeter = pSectProp->GetPerimeter(pgsPointOfInterest(segmentKey,0.00));
   
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 Lg = pBridge->GetSegmentLength(segmentKey);

   Float64 solid_box_surface_area = perimeter*Lg;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const GirderLibraryEntry* pGdrEntry = pGroup->GetGirder(segmentKey.girderIndex)->GetGirderLibraryEntry();
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();
   Float64 W3 = GetDimension(dimensions,_T("W3"));
   Float64 H2 = GetDimension(dimensions,_T("H2"));
   Float64 F1 = GetDimension(dimensions,_T("F1"));
   Float64 F2 = GetDimension(dimensions,_T("F2"));

   Float64 void_surface_area = Lg*( 2*(H2 - F1 - F2) + 2*(W3 - F1 - F2) + 2*sqrt(2*F1*F1) + 2*sqrt(2*F2*F2) );

   if ( bReduceForPoorlyVentilatedVoids )
      void_surface_area *= 0.50;

   Float64 surface_area = solid_box_surface_area + void_surface_area;

   return surface_area;
}

void CBoxBeamFactory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions, 
                                  IBeamFactory::BeamFace endTopFace, Float64 endTopLimit, IBeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, IBeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover)
{
   HRESULT hr = S_OK;

   CComObject<CStrandMoverImpl>* pStrandMover;
   CComObject<CStrandMoverImpl>::CreateInstance(&pStrandMover);

   CComPtr<IStrandMover> sm = pStrandMover;

   // set the shapes for harped strand bounds - only in the thinest part of the webs
   Float64 H1 = GetDimension(dimensions,_T("H1"));
   Float64 H2 = GetDimension(dimensions,_T("H2"));
   Float64 H3 = GetDimension(dimensions,_T("H3"));
   Float64 W2 = GetDimension(dimensions,_T("W2"));
   Float64 W3 = GetDimension(dimensions,_T("W3"));

   Float64 width = W2;
   Float64 depth = H1+H2+H3;

   CComPtr<IRectangle> lft_harp_rect, rgt_harp_rect;
   hr = lft_harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));
   hr = rgt_harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));

   lft_harp_rect->put_Width(width);
   lft_harp_rect->put_Height(depth);
   rgt_harp_rect->put_Width(width);
   rgt_harp_rect->put_Height(depth);

   Float64 hook_offset = (W3 + width)/2.0;

   CComPtr<IPoint2d> lft_hook, rgt_hook;
   lft_hook.CoCreateInstance(CLSID_Point2d);
   rgt_hook.CoCreateInstance(CLSID_Point2d);

   lft_hook->Move(-hook_offset, -depth/2.0);
   rgt_hook->Move( hook_offset, -depth/2.0);

   lft_harp_rect->putref_HookPoint(lft_hook);
   rgt_harp_rect->putref_HookPoint(rgt_hook);

   CComPtr<IShape> lft_shape, rgt_shape;
   lft_harp_rect->get_Shape(&lft_shape);
   rgt_harp_rect->get_Shape(&rgt_shape);

   CComQIPtr<IConfigureStrandMover> configurer(sm);
   hr = configurer->AddRegion(lft_shape, 0.0);
   ATLASSERT (SUCCEEDED(hr));
   hr = configurer->AddRegion(rgt_shape, 0.0);
   ATLASSERT (SUCCEEDED(hr));

   // set vertical offset bounds and increments
   Float64 hptb  = hpTopFace     == IBeamFactory::BeamBottom ? hpTopLimit     - depth : -hpTopLimit;
   Float64 hpbb  = hpBottomFace  == IBeamFactory::BeamBottom ? hpBottomLimit  - depth : -hpBottomLimit;
   Float64 endtb = endTopFace    == IBeamFactory::BeamBottom ? endTopLimit    - depth : -endTopLimit;
   Float64 endbb = endBottomFace == IBeamFactory::BeamBottom ? endBottomLimit - depth : -endBottomLimit;

   hr = configurer->SetHarpedStrandOffsetBounds(0, hptb, hpbb, endtb, endbb, endIncrement, hpIncrement);
   ATLASSERT (SUCCEEDED(hr));

   hr = sm.CopyTo(strandMover);
   ATLASSERT (SUCCEEDED(hr));
}

std::_tstring CBoxBeamFactory::GetImage()
{
   return std::_tstring(_T("BoxBeam.gif"));
}


CLSID CBoxBeamFactory::GetCLSID()
{
   return CLSID_BoxBeamFactory;
}

LPCTSTR CBoxBeamFactory::GetImageResourceName()
{
   return _T("BoxBeam");
}

HICON  CBoxBeamFactory::GetIcon() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_BOXBEAM) );
}

void CBoxBeamFactory::GetDimensions(const IBeamFactory::Dimensions& dimensions,
                                    Float64& H1, 
                                    Float64& H2, 
                                    Float64& H3, 
                                    Float64& H4, 
                                    Float64& H5,
                                    Float64& H6, 
                                    Float64& H7, 
                                    Float64& W1, 
                                    Float64& W2, 
                                    Float64& W3, 
                                    Float64& W4, 
                                    Float64& F1, 
                                    Float64& F2, 
                                    Float64& C1,
                                    Float64& J,
                                    Float64& shearKeyDepth,
                                    Float64& endBlockLength)
{
   H1 = GetDimension(dimensions,_T("H1"));
   H2 = GetDimension(dimensions,_T("H2"));
   H3 = GetDimension(dimensions,_T("H3"));
   H4 = GetDimension(dimensions,_T("H4"));
   H5 = GetDimension(dimensions,_T("H5"));
   H6 = GetDimension(dimensions,_T("H6"));
   H7 = GetDimension(dimensions,_T("H7"));
   W1 = GetDimension(dimensions,_T("W1"));
   W2 = GetDimension(dimensions,_T("W2"));
   W3 = GetDimension(dimensions,_T("W3"));
   W4 = GetDimension(dimensions,_T("W4"));
   F1 = GetDimension(dimensions,_T("F1"));
   F2 = GetDimension(dimensions,_T("F2"));
   C1 = GetDimension(dimensions,_T("C1"));
   J  = GetDimension(dimensions,_T("Jmax"));
   shearKeyDepth   = GetDimension(dimensions,_T("ShearKeyDepth"));
   endBlockLength  = GetDimension(dimensions,_T("EndBlockLength"));
}



void CBoxBeamFactory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, 
                                               pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing)
{
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   Float64 W1 = GetDimension(dimensions,_T("W1"));
   Float64 W2 = GetDimension(dimensions,_T("W2"));
   Float64 W3 = GetDimension(dimensions,_T("W3"));
   Float64 W4 = GetDimension(dimensions,_T("W4"));
   Float64 J  = GetDimension(dimensions,_T("Jmax"));

   // girder width is max of top and bottom width
   Float64 gwt = 2*(W1+W2) + W3;
   Float64 gwb = 2*(W4+W2) + W3;

   Float64 gw = Max(gwt,gwb);

   if ( sdt == pgsTypes::sdtCompositeCIP || sdt == pgsTypes::sdtCompositeSIP )
   {
      if(sbs == pgsTypes::sbsUniform || sbs == pgsTypes::sbsGeneral)
      {
         *minSpacing = gw;
         *maxSpacing = MAX_GIRDER_SPACING;
      }
      else
      {
         ATLASSERT(false); // shouldn't get here
      }
   }
   else
   {
      if (sbs == pgsTypes::sbsUniformAdjacent || sbs == pgsTypes::sbsGeneralAdjacent)
      {
         *minSpacing = gw;
         *maxSpacing = gw+J;
      }
      else
      {
         ATLASSERT(false); // shouldn't get here
      }
   }
}

bool CBoxBeamFactory::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType)
{
   bool is_key;

   switch( spacingType )
   {
   case pgsTypes::sbsUniformAdjacent:
   case pgsTypes::sbsGeneralAdjacent:
      {
      Float64 sk = GetDimension(dimensions,_T("ShearKeyDepth"));
      is_key = sk > 0.0;
      }
      break;
   default:
      is_key = false;
   }

   return is_key;
}

// Free function to compute area of a horizontally chopped inverted triangle
// as shown below
//                       W
//    |----------------------------------
//    | ********w******** | ********   /
//    |h***************** | ****   /
//    | ***************** | ** /
//  H |-------------------|/
//    |                /
//    |           /
//    |      /
//    | /
//    V
//
//   Triangle above of Height (H) and Width (W). Want area chopped at depth (h)
static Float64 AreaChoppedTriangle(Float64 W, Float64 H, Float64 h)
{
   ATLASSERT(h>=0.0 && h<=H);
   ATLASSERT(H>0.0);

   Float64 w = W * (1-h/H); // width of rectanglar part

   return w*h + (W-w)*h/2.0;
}


void CBoxBeamFactory::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint)
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;

   if (!IsShearKey(dimensions, spacingType))
   {
      return;
   }

   Float64 H1, H2, H3, H4, H5, H6, H7, W1, W2, W3, W4, F1, F2, C1, J, shearKeyDepth, endBlockLength;
   GetDimensions(dimensions,H1, H2, H3, H4, H5, H6, H7, W1, W2, W3, W4, F1, F2, C1, J, shearKeyDepth, endBlockLength);

   if (shearKeyDepth==0.0)
   {
      return;
   }

   // NOTE: To understand what is going on here, refer to figures in BoxBeam.vsd for descriptions 
   //       of shear key area regions and variables
   Float64 Htotal = H1 + H2 + H3;
   Float64 HW = Htotal - (H4 + H5 + H6 + H7);

   // First compute uniform area that occurs when beams have zero joint spacing. This is done for 
   // each of the three comparisons of W4 and W1.
   // Walk down section from top to bottom until shear key depth is depleted...
   if (W4>W1)
   {
      // Bottom flange wider than top (Case 1)
      // 5 sections to deal with
      Float64 A1(0.0), A2(0.0), A3(0.0), A4(0.0), A5(0.0);
      Float64 Hreg = shearKeyDepth; // shear key remaining from top of current section
      Float64 h;
      if (Hreg > 0.0)
      {
         h = Min(Hreg, H4);
         A1 = (W4-W1) * h;

         Hreg -= H4;
         if (Hreg > 0.0)
         {
            h = Min(Hreg, H5);
            A2 = (W4-W1) * h;
            A3 = W1*h / 2.0;

            Hreg -= H5;
            if (Hreg > 0.0)
            {
               h = Min(Hreg, HW);
               A4 = W4 * h;

               Hreg -= HW;
               if (Hreg > 0.0)
               {
                  if (Hreg < H6)
                     A5 = AreaChoppedTriangle(W4, H6, Hreg);
                  else
                     A5 = W4 * H6 / 2.0;
               }
            }
         }
      }

      *uniformArea = A1 + A2 + A3 + A4 + A5;
   }
   else if (W4<W1)
   {
      // Top wider than bottom (Case 2)
      // 5 sections to deal with
      Float64 A1(0.0), A2(0.0), A3(0.0), A4(0.0), A5(0.0);
      Float64 Hreg = shearKeyDepth - H4; // shear key remaining from top of current section
      Float64 h;
      if (Hreg > 0.0)
      {
         h = Min(Hreg, H5);
         A1 = W1 * h / 2.0;

         Hreg -= H5;
         if (Hreg > 0.0)
         {
            h = Min(Hreg, HW);
            A2 = W1 * h;

            Hreg -= HW;
            if (Hreg > 0.0)
            {
               h = Min(Hreg, H6);
               A3 = (W1-W4) * h;
               A4 = AreaChoppedTriangle(W4, H6, h);

               Hreg -= H6;
               if (Hreg > 0.0)
               {
                  h = Min(Hreg, H7);
                  A5 = (W1-W4)*h;
               }
            }
         }
      }

      *uniformArea = A1 + A2 + A3 + A4 + A5;

   }
   else // (W4==W1)
   {
      // Flange widths are equal (Case 3) - 3 sections to deal with
      Float64 A1(0.0), A2(0.0), A3(0.0);
      Float64 Hreg = shearKeyDepth - H4; // shear key remaining from top of current section
      Float64 h;
      if (Hreg > 0.0)
      {
         h = Min(Hreg, H5);
         A1 = W1 * h / 2.0;

         Hreg -= H5;
         if (Hreg > 0.0)
         {
            h = Min(Hreg, HW);
            A2 = W1 * h;

            Hreg -= HW;
            if (Hreg > 0.0)
            {
               if (Hreg<H6)
                  A3 = AreaChoppedTriangle(W1, H6, Hreg);
               else
                  A3 = W1 * H6 / 2.0;
            }
         }
      }

      *uniformArea = A1 + A2 + A3;
   }

   // Next pick up area of bottom chamfer
   Float64 hc1 = shearKeyDepth - Htotal + C1;
   if (hc1>0.0)
   {
      *uniformArea += hc1*C1/2.0;
   }

   // Lastly, compute portion of shear key area per joint spacing
   *areaPerJoint = shearKeyDepth;
}

Float64 CBoxBeamFactory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   Float64 W1 = GetDimension(dimensions,_T("W1"));
   Float64 W2 = GetDimension(dimensions,_T("W2"));
   Float64 W3 = GetDimension(dimensions,_T("W3"));
   Float64 W4 = GetDimension(dimensions,_T("W4"));

   Float64 top = 2*(W1+W2) + W3;

   Float64 bot = 2*(W4+W2) + W3;

   return Max(top,bot);
}
