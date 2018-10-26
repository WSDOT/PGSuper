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


// Utility functions for storing floating point values in combo boxes
inline void StoreFloatInCb(CComboBox* pcb, int idx, Float64 fval)
{
   // yes, we are losing precision here, but 32 bits is all we have
   float val = (float)fval;

   DWORD_PTR dval;
   memcpy(&dval, &val, 4);
   pcb->SetItemData(idx, dval);
}

inline int PutFloatInCB(Float64 fval, CComboBox* pcb, sysNumericFormatTool& tool)
{
   std::_tstring str = tool.AsString(fval);
   int idx = pcb->AddString(str.c_str());
   StoreFloatInCb(pcb, idx, fval);

   return idx;
}

inline float GetFloatFromCb(CComboBox* pcb, int idx)
{
   DWORD_PTR dval = pcb->GetItemData(idx);

   float val;
   memcpy(&val, &dval, 4);

   return val;
}


// Handy function for putting a range of floats into a combo box with a given increment
template <class TUnit>
void FillComboWithUnitFloatRange(Float64 selectedVal, Float64 minVal, Float64 maxVal, Float64 incrVal,
                                        CComboBox* pfcCtrl, Uint16 precision, const TUnit& tunit )
{
   pfcCtrl->ResetContent();

   sysNumericFormatTool tool;
   tool.SetFormat( sysNumericFormatTool::Fixed );
   tool.SetPrecision(precision);

   Float64 toler = pow(10.0, -precision-1); // tolerance on equals

   // First convert to output units
   selectedVal = ::ConvertFromSysUnits(selectedVal, tunit);

   // Minval and max val set upper and lower bounds
   minVal = ::ConvertFromSysUnits(minVal, tunit);
   maxVal = ::ConvertFromSysUnits(maxVal, tunit);

   incrVal = ::ConvertFromSysUnits(incrVal, tunit);

   // string for value to be selected
   std::_tstring selectedStr = tool.AsString(selectedVal);

   // Force selected value to fit in range
   if (selectedVal<minVal)
   {
      ATLASSERT(0); // may or may not be a problem - although calling routine should deal with this
      selectedVal=minVal;
   }

   if (selectedVal>maxVal)
   {
      ATLASSERT(0); // may or may not be a problem
      selectedVal=maxVal;
   }

   int  idx_sel = CB_ERR; // did we put selected number in yet

   // Next, our min and max values may not fit exactly to our increment. If not, put them on the head or tail 
   Float64 startVal = CeilOff(minVal, incrVal);

   if (!IsEqual(startVal,minVal,toler))
   {
      int idx = PutFloatInCB(minVal, pfcCtrl , tool);
      if(IsEqual(selectedVal,minVal,toler))
         idx_sel = idx;
   }

   Float64 endVal = FloorOff(maxVal, incrVal);

   Float64 curr = startVal;
   while(curr<=endVal)
   {
       if(idx_sel==CB_ERR && ::IsEqual(curr,selectedVal,toler))
       {
          idx_sel = PutFloatInCB(selectedVal, pfcCtrl, tool); // selected val fits exactly into cb
       }
       else
       {
          if (idx_sel==CB_ERR && ::IsLT(selectedVal,curr,toler))
          {
             idx_sel = PutFloatInCB(selectedVal, pfcCtrl, tool); // put exact selected val into cb
          }

          int idx = PutFloatInCB(curr, pfcCtrl, tool);
          if (idx_sel==CB_ERR && ::IsEqual(selectedVal,curr,toler))
             idx_sel = idx;
      }

      curr += incrVal;
   }

   if (!IsEqual(endVal,maxVal,toler)) // max might not fit in increment
   {
       if (idx_sel==CB_ERR && ::IsLT(selectedVal,maxVal,toler))
       {
          // Selected value is also above top of increment - put in explicitely
          idx_sel = PutFloatInCB(selectedVal, pfcCtrl, tool);
       }

       int idx = PutFloatInCB(maxVal, pfcCtrl, tool);
       if (idx_sel==CB_ERR && ::IsEqual(selectedVal,maxVal,toler))
          idx_sel = idx;
   }

   if(idx_sel==CB_ERR)
   {
      ATLASSERT(0); // something is very messed up
      pfcCtrl->SetCurSel(0);
   }
   else
   {
      pfcCtrl->SetCurSel(idx_sel);
   }
}
