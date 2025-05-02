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
#pragma once

#include "PsgLibLib.h"
#include <PsgLib\DebondData.h>

  PSGLIBFUNC const CString& GetStrandDefinitionType(pgsTypes::StrandDefinitionType strandDefinitionType, pgsTypes::AdjustableStrandType adjustableStrandType);
  CString PSGLIBFUNC GetGirderSpacingType(pgsTypes::SupportedBeamSpacing spacingType, bool bSplicedGirder);
  CString PSGLIBFUNC GetTopWidthType(pgsTypes::TopWidthType type);
  CString PSGLIBFUNC GetLiveLoadTypeName(pgsTypes::LiveLoadType llType);
  CString PSGLIBFUNC GetLiveLoadTypeName(pgsTypes::LoadRatingType ratingType);
  std::_tstring PSGLIBFUNC GetDesignTypeName(arFlexuralDesignType type);
  LPCTSTR PSGLIBFUNC GetDeckTypeName(pgsTypes::SupportedDeckType deckType);
  LPCTSTR PSGLIBFUNC GetCastDeckEventName(pgsTypes::SupportedDeckType deckType);

  // Changes in girder fill can make debonding invalid. This algorithm gets rid of any
  // debonding of strands that don't exist
  bool PSGLIBFUNC ReconcileDebonding(const ConfigStrandFillVector& fillvec, std::vector<CDebondData>& rDebond);
  bool PSGLIBFUNC ReconcileExtendedStrands(const ConfigStrandFillVector& fillvec, std::vector<GridIndexType>& extendedStrands);
