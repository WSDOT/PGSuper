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

#pragma once

#include <EAF\EAFViewController.h>

// {AB3D74A8-BAF7-4DF6-9328-ABD69B9B80F6}
DEFINE_GUID(IID_ILoadsViewController,
   0xab3d74a8, 0xbaf7, 0x4df6, 0x93, 0x28, 0xab, 0xd6, 0x9b, 0x9b, 0x80, 0xf6);
struct __declspec(uuid("{AB3D74A8-BAF7-4DF6-9328-ABD69B9B80F6}")) ILoadsViewController;

interface ILoadsViewController : IEAFViewController
{
   enum Field {Type,Event,LoadCase,Location,Magnitude,Description};
   enum Direction{Ascending,Descending};
   virtual void SortBy(Field field,Direction direction) = 0;
   virtual IndexType GetLoadCount() const = 0;
   virtual std::_tstring GetFieldValue(IndexType idx, Field field) const = 0;
   virtual void DeleteLoad(IndexType idx) = 0;
};
