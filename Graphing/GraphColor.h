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

#pragma once

#include <PGSuperColors.h>

#define GRAPH_BACKGROUND WHITE //RGB(220,255,220)
#define GRAPH_GRID_PEN_STYLE PS_DOT
#define GRAPH_GRID_PEN_WEIGHT 1
#define GRAPH_GRID_COLOR GREY50 //RGB(0,150,0)

#define GRAPH_PEN_WEIGHT 2


class CGraphColor
{
public:
   CGraphColor();
   CGraphColor(IndexType nGraphs);

   void SetGraphCount(IndexType nGraphs);
   void SetHueRange(Float64 minHue,Float64 maxHue); // a value between 0 and 360
   void SetSaturation(Float64 saturation); // a value between 0 and 1
   void SetLightness(Float64 lightness); // a value between 0 and 1

   // number of colors before repeating
   // note that repeat colors are shifted in the hue range so that
   // no two same colors are used (e.g. every time red is used
   // it will be a different shade)
   void SetColorPerBandCount(IndexType nColorsPerBand);

   COLORREF GetColor(IndexType index);

private:
   bool m_bInitialized;
   void Init();
   void ComputeColorParameters();

   IndexType m_nGraphs;
   Float64 m_MinHue;
   Float64 m_MaxHue;
   Float64 m_Saturation;
   Float64 m_Lightness;
   IndexType m_nColorBands;
   IndexType m_ColorsPerBand;
   Float64 m_BandHueStep;
};