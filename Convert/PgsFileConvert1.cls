VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "PgsFileConvert1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
'///////////////////////////////////////////////////////////////////////
'// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
'// Copyright (C) 1999  Washington State Department of Transportation
'//                     Bridge and Structures Office
'//
'// This program is free software; you can redistribute it and/or modify
'// it under the terms of the Alternate Route Open Source License as
'// published by the Washington State Department of Transportation,
'// Bridge and Structures Office.
'//
'// This program is distributed in the hope that it will be useful, but
'// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied
'// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
'// the Alternate Route Open Source License for more details.
'//
'// You should have received a copy of the Alternate Route Open Source
'// License along with this program; if not, write to the Washington
'// State Department of Transportation, Bridge and Structures Office,
'// 4500 3rd AVE SE - P.O. Box  47340, Olympia, WA 98503, USA or e-mail
'// Bridge_Support@wsdot.wa.gov
'///////////////////////////////////////////////////////////////////////

Option Explicit
Private Declare Function CopyFile Lib "kernel32" Alias "CopyFileA" (ByVal lpExistingFileName As String, ByVal lpNewFileName As String, ByVal bFailIfExists As Long) As Long
Private Declare Function GetTempPath Lib "kernel32" Alias "GetTempPathA" (ByVal nBufferLength As Long, ByVal lpBuffer As String) As Long
Private Declare Function GetTempFileName Lib "kernel32" Alias "GetTempFileNameA" (ByVal lpszPath As String, ByVal lpPrefixString As String, ByVal wUnique As Long, ByVal lpTempFileName As String) As Long
Const MAX_PATH = 260

' returns -1=error, 0= no converson, 1=converted
' if file is converted, outFname is different than inFname
Enum ConvertStatus
   Failed = -1
   NoAction = 0
   Converted = 1
End Enum

Public Function Convert(inFname As String, outFname As String) As ConvertStatus
   Dim parser As New DOMDocument
   
   If (parser Is Nothing) Then
      MsgBox "There was an error creating the XML document parser. Please notify Bridge_Support@wsdot.wa.gov", vbOKOnly + vbCritical, "Error"
      Convert = -1
      Exit Function
   End If
   
   'Load the file into the parser
   If (Not parser.Load(inFname)) Then
      MsgBox "Error Loading File " & inFname, vbOKOnly, "Error"
      Convert = -1
      Exit Function
   End If

   'Check if the file is the correct type and correct version
   Dim orig_version As Double
   orig_version = GetVersion(parser)
   If (orig_version = 0.9) Then
      ' File is current version - do nothing
      Convert = 0
      Exit Function
   End If
   
   If (orig_version <> 0.8 And orig_version <> 0.83) Then
      MsgBox "Not a PGSuper file or is the wrong version", vbOKOnly, "Error"
      Convert = -1
      Exit Function
   End If
   
   'Get the ProjectData node of the XML Tree.  This is where
   'the PGSuper project data begins
   Dim xmlProjectData As IXMLDOMNode
   Set xmlProjectData = parser.documentElement.selectSingleNode("ProjectData")
   Debug.Assert Not xmlProjectData Is Nothing
   
   ' Convert from 0.80 to 0.83
   If orig_version = 0.8 Then
      Convert_p80 xmlProjectData
   End If
   
   ' Convert from 0.83 to 0.90
   Convert_p83 xmlProjectData
   
   'Update to latest File Version Number
   parser.documentElement.Attributes.getNamedItem("version").Text = "0.90"
   
   'Do a little adjustment of the XML so the WBFL parser will like it
   Dim xml As String
   xml = parser.xml
   xml = Replace(xml, "</Prestressing><Shear version=""2"">", "</Prestressing>" + Chr(13) + Chr(10) + "<Shear version=""2"">", , , vbTextCompare)
   xml = Replace(xml, "</Shear><Specification version=""1"">", "</Shear>" + Chr(13) + Chr(10) + "<Specification version=""1"">", , , vbTextCompare)
   xml = Replace(xml, "</LiftingAndHauling></ProjectData>", "</LiftingAndHauling>" + Chr(13) + Chr(10) + "</ProjectData>", , , vbTextCompare)
   parser.loadXML xml
   
   'Make temporary file name
   Dim pathname As String
   pathname = String$(MAX_PATH, 0)
   Call GetTempPath(MAX_PATH, pathname)
   
   Dim prefix As String
   prefix = "PG"
   Dim bkname As String
   bkname = String$(MAX_PATH, 0)
   Call GetTempFileName(pathname, prefix, 0, bkname)
   bkname = Left$(bkname, InStr(bkname, Chr$(0)) - 1)

   'Save Converted File
   parser.save bkname
   
   'Clean Up
   Set xmlProjectData = Nothing
   Set parser = Nothing
   
   'Return
   outFname = bkname
   Convert = 1
End Function

Private Function GetVersion(parser As DOMDocument) As Double
   Dim strType As String
   Dim dblVersion As Double
   
   strType = parser.documentElement.nodeName
   If (Not strType = "PGSuper") Then
      GetVersion = 0#
      Exit Function
   End If
   
   dblVersion = parser.documentElement.Attributes(0).nodeValue
      
   GetVersion = dblVersion
End Function
Private Function BackupFileName(fname As String) As String
   'Creates the name of the backup file
   BackupFileName = fname + ".bak"
End Function
Private Function GetGirderCount(xmlProjectData As IXMLDOMNode) As Long
  'Get the XSectionData node so we can get the girder count
  'We need the span length because we have to replicate the
  'prestressing and shear data for all girders
  Dim xmlXSection As IXMLDOMNode
  Set xmlXSection = xmlProjectData.selectSingleNode("CrossSection").selectSingleNode("XSectionData")
  Debug.Assert Not xmlXSection Is Nothing
  
  Dim nGirders As Long
  nGirders = xmlXSection.selectSingleNode("GdrLineCount").nodeTypedValue

  GetGirderCount = nGirders
End Function
Private Sub Convert_p80(xmlProjectData As IXMLDOMNode)

' Convert from version 0.80 to 0.83

   'Convert Prestress Data
   ConvertPrestressData_p80 xmlProjectData
   
   'Convert Stirrup Data
   ConvertShearData_p80 xmlProjectData
   
   'Convert the Lifting and Hauling Data
   ConvertLiftingAndHaulingData_p80 xmlProjectData
   
End Sub


Private Sub ConvertPrestressData_p80(xmlProjectData As IXMLDOMNode)
  'Get the number of girders
  Dim nGirders As Long
  nGirders = GetGirderCount(xmlProjectData)
  
  'Get the <Prestressing> block
  Dim xmlPrestressing As IXMLDOMNode
  Set xmlPrestressing = xmlProjectData.selectSingleNode("Prestressing")
  Debug.Assert Not xmlPrestressing Is Nothing
  
  'Update <Prestressing> version number
   xmlPrestressing.Attributes.getNamedItem("version").Text = "2"
  
  'Get the one and only PrestressData block
  Dim xmlPrestressData As IXMLDOMNode
  Set xmlPrestressData = xmlPrestressing.selectSingleNode("PrestressData")
  Debug.Assert Not xmlPrestressData Is Nothing
  
  'Update <PrestressData> version number
   xmlPrestressData.Attributes.getNamedItem("version").Text = "2"
  
  'Remove xmlPrestressData from xmlPrestressing
  xmlPrestressing.removeChild xmlPrestressData
  
  'Save the prestressing strand type
  Dim StrandType As Long
  StrandType = xmlPrestressData.selectSingleNode("Strand").nodeTypedValue
  
  'Remove the strand type from the PrestressData block
  xmlPrestressData.removeChild xmlPrestressData.selectSingleNode("Strand")
  
  'The next few operations update the <XSection> data block.
  'Get Fci from the <CrossSection> node
  Dim xmlXSection As IXMLDOMNode
  Set xmlXSection = xmlProjectData.selectSingleNode("CrossSection").selectSingleNode("XSectionData")
  Debug.Assert Not xmlXSection Is Nothing
  Dim dblFci As Double
  dblFci = xmlXSection.selectSingleNode("RelStrength").nodeTypedValue
  
  'Remove <Fci> Node from <CrossSection> node
  xmlXSection.removeChild xmlXSection.selectSingleNode("RelStrength")
  
  'Update <XSectionData> version number
   xmlXSection.Attributes.getNamedItem("version").Text = "2"
  'Done updating the <XSection> data block
  
  'Add <PrestressStrandKey> node to the <Prestressing> node
  Dim xmlPrestressStrandKey As IXMLDOMNode
  Set xmlPrestressStrandKey = xmlProjectData.ownerDocument.createNode(NODE_ELEMENT, "PrestressStrandKey", "")
  xmlPrestressStrandKey.Text = StrandType
  xmlPrestressing.appendChild xmlPrestressStrandKey
  
  'Add <PrestressDataCount> node to the <Prestressing> node
  Dim xmlPrestressDataCount As IXMLDOMNode
  Set xmlPrestressDataCount = xmlProjectData.ownerDocument.createNode(NODE_ELEMENT, "PrestressDataCount", "")
  xmlPrestressDataCount.Text = nGirders
  xmlPrestressing.appendChild xmlPrestressDataCount
  
  Dim span As Long
  span = 0 'Version 0.8 only has one span and it's index is 0
  
  Dim girder As Long
  For girder = 0 To nGirders - 1
     'Add <GirderPrestressData> node to the <Prestressing> node
     Dim xmlGirderPrestressData As IXMLDOMNode
     Set xmlGirderPrestressData = xmlProjectData.ownerDocument.createNode(NODE_ELEMENT, "GirderPrestressData", "")
     xmlPrestressing.appendChild xmlGirderPrestressData
     
     'Set the version number for the <GirderPrestressData> node
     Dim xmlVersionAttribute As IXMLDOMAttribute
     Set xmlVersionAttribute = xmlProjectData.ownerDocument.createAttribute("version")
     xmlVersionAttribute.Text = "1"
     xmlGirderPrestressData.Attributes.setNamedItem xmlVersionAttribute
  
     'Add <Span> node to the <GirderPrestressData> node
     Dim xmlSpan As IXMLDOMNode
     Set xmlSpan = xmlProjectData.ownerDocument.createNode(NODE_ELEMENT, "Span", "")
     xmlSpan.Text = span
     xmlGirderPrestressData.appendChild xmlSpan

     'Add <Girder> node to the <GirderPrestressData> node
     Dim xmlGirder As IXMLDOMNode
     Set xmlGirder = xmlProjectData.ownerDocument.createNode(NODE_ELEMENT, "Girder", "")
     xmlGirder.Text = girder
     xmlGirderPrestressData.appendChild xmlGirder
     
     'Add <PrestressData> node to the <GirderPrestressData> node
     Dim xmlPrestressDataClone As IXMLDOMNode
     Set xmlPrestressDataClone = xmlPrestressData.cloneNode(True)
     xmlGirderPrestressData.appendChild xmlPrestressDataClone
  
     'Append <Fci> to the <GirderPrestressData> block
     Dim xmlFci As IXMLDOMNode
     Set xmlFci = xmlProjectData.ownerDocument.createNode(NODE_ELEMENT, "Fci", "")
     xmlFci.Text = dblFci
     xmlPrestressDataClone.appendChild xmlFci
     
  Next girder
   
  'Reformat the XML string so the WBFL parser can accept it.
  Dim xml As String
  xml = xmlPrestressing.xml
  xml = Replace(xml, "><", ">" + Chr(13) + Chr(10) + "<", , , vbTextCompare)
  Dim parser As New DOMDocument
  parser.loadXML xml
  xmlProjectData.replaceChild parser.firstChild.cloneNode(True), xmlPrestressing
End Sub
Private Sub ConvertShearData_p80(xmlProjectData As IXMLDOMNode)
  'Get the number of girders
  Dim nGirders As Long
  nGirders = GetGirderCount(xmlProjectData)
  
  'Get the <Shear> block
  Dim xmlShear As IXMLDOMNode
  Set xmlShear = xmlProjectData.selectSingleNode("Shear")
  Debug.Assert Not xmlShear Is Nothing
  
  'Update <Shear> version number
   xmlShear.Attributes.getNamedItem("version").Text = "2"
  
  'Get the one and only ShearData block
  Dim xmlShearData As IXMLDOMNode
  Set xmlShearData = xmlShear.selectSingleNode("ShearData")
  Debug.Assert Not xmlShearData Is Nothing
  
   'Update the <ShearData> version number
   xmlShearData.Attributes.getNamedItem("version").Text = "2"
  
  'Remove xmlShearData from xmlShear
  xmlShear.removeChild xmlShearData
  
  'Save the prestressing strand type
  Dim RebarType As String
  RebarType = xmlShearData.selectSingleNode("MsMaterial").nodeTypedValue
  
  'Remove the rebar type from the ShearData block
  xmlShearData.removeChild xmlShearData.selectSingleNode("MsMaterial")
  
  'Insert the <StirrupMaterial> node at the top of the <Shear> node
  Dim xmlStirrupMaterial As IXMLDOMNode
  Set xmlStirrupMaterial = xmlProjectData.ownerDocument.createNode(NODE_ELEMENT, "StirrupMaterial", "")
  xmlStirrupMaterial.Text = RebarType
  xmlShear.insertBefore xmlStirrupMaterial, xmlShear.firstChild
  
  'Add <ShearDataCount> node to the <Shear> node
  Dim xmlShearDataCount As IXMLDOMNode
  Set xmlShearDataCount = xmlProjectData.ownerDocument.createNode(NODE_ELEMENT, "ShearDataCount", "")
  xmlShearDataCount.Text = nGirders
  xmlShear.appendChild xmlShearDataCount
  
  Dim span As Long
  span = 0 'Version 0.8 only has one span and it's index is 0
  
  Dim girder As Long
  For girder = 0 To nGirders - 1
     'Add <GirderShearData> node to the <Shear> node
     Dim xmlGirderShearData As IXMLDOMNode
     Set xmlGirderShearData = xmlProjectData.ownerDocument.createNode(NODE_ELEMENT, "GirderShearData", "")
     xmlShear.appendChild xmlGirderShearData
  
     'Set the version number for the <GirderShearData> node
     Dim xmlVersionAttribute As IXMLDOMAttribute
     Set xmlVersionAttribute = xmlGirderShearData.ownerDocument.createAttribute("version")
     xmlVersionAttribute.Text = "1"
     xmlGirderShearData.Attributes.setNamedItem xmlVersionAttribute
     
     'Add <Span> node to the <GirderShearData> node
     Dim xmlSpan As IXMLDOMNode
     Set xmlSpan = xmlGirderShearData.ownerDocument.createNode(NODE_ELEMENT, "Span", "")
     xmlSpan.Text = span
     xmlGirderShearData.appendChild xmlSpan
  
     'Add <Girder> node to the <GirderShearData> node
     Dim xmlGirder As IXMLDOMNode
     Set xmlGirder = xmlGirderShearData.ownerDocument.createNode(NODE_ELEMENT, "Girder", "")
     xmlGirder.Text = girder
     xmlGirderShearData.appendChild xmlGirder
     
     'Add <ShearData> node to the <GirderShearData> node
      Dim xmlShearDataClone As IXMLDOMNode
      Set xmlShearDataClone = xmlShearData.cloneNode(True)
      xmlGirderShearData.appendChild xmlShearDataClone
  Next girder
   
  'Reformat the XML string so the WBFL parser can accept it.
  Dim xml As String
  xml = xmlShear.xml
  xml = Replace(xml, "><", ">" + Chr(13) + Chr(10) + "<", , , vbTextCompare)
  Dim parser As New DOMDocument
  parser.loadXML xml
  xmlProjectData.replaceChild parser.firstChild.cloneNode(True), xmlShear
End Sub

Private Sub ConvertLiftingAndHaulingData_p80(xmlProjectData As IXMLDOMNode)
  'Get the number of girders
  Dim nGirders As Long
  nGirders = GetGirderCount(xmlProjectData)
  
  'Get the <LiftingAndHauling> block
  Dim xmlLifting As IXMLDOMNode
  Set xmlLifting = xmlProjectData.selectSingleNode("LiftingAndHauling")
  Debug.Assert Not xmlLifting Is Nothing
  
  'Update the version number of the <LiftingAndHaulingBlock>
  xmlLifting.Attributes.getNamedItem("version").Text = "2"
  
  'Get the lifting loop location and remove the node
  Dim dblLifting As Double
  dblLifting = xmlLifting.selectSingleNode("LiftingLoopLocation").nodeTypedValue
  xmlLifting.removeChild xmlLifting.selectSingleNode("LiftingLoopLocation")
  
  'Get the truck support location and remove the node
  Dim dblHauling As Double
  dblHauling = xmlLifting.selectSingleNode("TruckSupportLocation").nodeTypedValue
  xmlLifting.removeChild xmlLifting.selectSingleNode("TruckSupportLocation")
  
  'Add <LiftingAndHaulingDataCount> node to the <LiftingAndHauling> node
  Dim xmlLiftingAndHauingDataCount As IXMLDOMNode
  Set xmlLiftingAndHauingDataCount = xmlProjectData.ownerDocument.createNode(NODE_ELEMENT, "LiftingAndHaulingDataCount", "")
  xmlLiftingAndHauingDataCount.Text = nGirders
  xmlLifting.appendChild xmlLiftingAndHauingDataCount
  
  'Store the lifting and hauling data for each girder
  Dim span As Long
  span = 0 'Version 0.8 only has one span and it's index is 0
  
  Dim girder As Long
  For girder = 0 To nGirders - 1
     'Add <LiftingAndHaulintData> node to the <LiftingAndHauling> node
     Dim xmlLiftingAndHaulingData As IXMLDOMNode
     Set xmlLiftingAndHaulingData = xmlProjectData.ownerDocument.createNode(NODE_ELEMENT, "LiftingAndHaulingData", "")
     xmlLifting.appendChild xmlLiftingAndHaulingData
     
     'Set the version number for the <LiftingAndHaulingData> node
     Dim xmlVersionAttribute As IXMLDOMAttribute
     Set xmlVersionAttribute = xmlLiftingAndHaulingData.ownerDocument.createAttribute("version")
     xmlVersionAttribute.Text = "1"
     xmlLiftingAndHaulingData.Attributes.setNamedItem xmlVersionAttribute

  
     'Add <Span> node to the <LiftingAndHaulintData> node
     Dim xmlSpan As IXMLDOMNode
     Set xmlSpan = xmlLiftingAndHaulingData.ownerDocument.createNode(NODE_ELEMENT, "Span", "")
     xmlSpan.Text = span
     xmlLiftingAndHaulingData.appendChild xmlSpan
  
     'Add <Girder> node to the <LiftingAndHaulintData> node
     Dim xmlGirder As IXMLDOMNode
     Set xmlGirder = xmlLiftingAndHaulingData.ownerDocument.createNode(NODE_ELEMENT, "Girder", "")
     xmlGirder.Text = girder
     xmlLiftingAndHaulingData.appendChild xmlGirder
     
     'Add <LiftingLoopLocation> node to the <LiftingAndHaulintData> node
     Dim xmlLiftingLoopLocation As IXMLDOMNode
     Set xmlLiftingLoopLocation = xmlLiftingAndHaulingData.ownerDocument.createNode(NODE_ELEMENT, "LiftingLoopLocation", "")
     xmlLiftingLoopLocation.Text = dblLifting
     xmlLiftingAndHaulingData.appendChild xmlLiftingLoopLocation
     
     'Add <TruckSupportLocation> node to the <LiftingAndHaulintData> node
     Dim xmlTruckSupportLocation As IXMLDOMNode
     Set xmlTruckSupportLocation = xmlLiftingAndHaulingData.ownerDocument.createNode(NODE_ELEMENT, "TruckSupportLocation", "")
     xmlTruckSupportLocation.Text = dblHauling
     xmlLiftingAndHaulingData.appendChild xmlTruckSupportLocation
  Next girder
   
  'Reformat the XML string so the WBFL parser can accept it.
  Dim xml As String
  xml = xmlLifting.xml
  xml = Replace(xml, "><", ">" + Chr(13) + Chr(10) + "<", , , vbTextCompare)
  Dim parser As New DOMDocument
  parser.loadXML xml
  xmlProjectData.replaceChild parser.firstChild.cloneNode(True), xmlLifting
End Sub

Private Sub Convert_p83(xmlProjectData As IXMLDOMNode)

' Convert from version 0.83 to 0.90

   'Convert Stirrup Data
   ConvertShearData_p83 xmlProjectData
   
End Sub

Private Sub ConvertShearData_p83(xmlProjectData As IXMLDOMNode)
  
  'Get the <Shear> block
  Dim xmlShear As IXMLDOMNode
  Set xmlShear = xmlProjectData.selectSingleNode("Shear")
  Debug.Assert Not xmlShear Is Nothing
  
  Dim xmlGirderShearData As IXMLDOMNodeList
  Set xmlGirderShearData = xmlShear.selectNodes("GirderShearData")
  Debug.Assert Not xmlGirderShearData Is Nothing
  
  Dim num_nodes As Long
  num_nodes = xmlGirderShearData.length
  Dim i As Long
  For i = 0 To num_nodes - 1
  
     Dim xmlShearData As IXMLDOMNode
     Set xmlShearData = xmlGirderShearData.Item(i).selectSingleNode("ShearData")
     Debug.Assert Not xmlShearData Is Nothing
     
     'Set the version number for the <ShearData> node
     Dim xmlVersionAttribute As IXMLDOMAttribute
     Set xmlVersionAttribute = xmlShearData.ownerDocument.createAttribute("version")
     xmlVersionAttribute.Text = "3"
     xmlShearData.Attributes.setNamedItem xmlVersionAttribute
     
     Dim xmlZoneCount As IXMLDOMNode
     Set xmlZoneCount = xmlShearData.selectSingleNode("ZoneCount")
     Debug.Assert Not xmlZoneCount Is Nothing
     
     'Add top flange bar size and spacing
     Dim xmlTopFlangeBarSize As IXMLDOMNode
     Set xmlTopFlangeBarSize = xmlProjectData.ownerDocument.createNode(NODE_ELEMENT, "TopFlangeBarSize", "")
     xmlTopFlangeBarSize.Text = "0"
     xmlShearData.insertBefore xmlTopFlangeBarSize, xmlZoneCount
  
     Dim xmlTopFlangeBarSpacing As IXMLDOMNode
     Set xmlTopFlangeBarSpacing = xmlProjectData.ownerDocument.createNode(NODE_ELEMENT, "TopFlangeBarSpacing", "")
     xmlTopFlangeBarSpacing.Text = "0.0"
     xmlShearData.insertBefore xmlTopFlangeBarSpacing, xmlZoneCount
    
  Next i

End Sub

