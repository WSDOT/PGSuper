General {#ug_library_dialogs_project_criteria_general}
==============================================
This tab allows you define general information about your Project Criteria.

Project Criteria allow you to define the version of the LRFD Specifications that is the basis for your analysis/design. They also allow you to customize certain provisions that are likely to be agency-specific, or not defined in the LRFD Specifications.

Item | Description 
------|-------------------------------------------------------------------
Entry Name | Each Design Criteria Entry must have a unique name. The name may contain any characters and can be up to 32 characters in length.
Description | Provide a description for the Project Criteria Entry.
Criteria Basis | Select the version of the LRFD Specifications that you would like to use along with the provisions described by the current Project Criteria Entry. Checking the box causes the most recent edition of the AASHTO LRFD Bridge Design Specifications that are supported by the software to be used. When new editions of the specifications are implemented, the project criteria will automatically update to that edition.
Use Constants and Equations from the Following Version of the Selected Specification | All of the LRFD Specifications prior to the 4th Edition 2007 are available in SI and US units. Selection of the SI version of the Specifications means that SI-specific constants will be used when evaluating specification equations.
Section Properties | Select either Gross or Transformed for the section properties that will in the analysis
Compute effective flange width | Select a method for computing the effective flange width.

The Description field can include the \%BDS\% symbol. When writing the project criteria description into reports the \%BDS\% symbol is replaced with the name of the currently selected LRFD Specification. For example, if the Description field contained "%BDS% is the basis of analysis" the resulting text would be "AASHTO LRFD Bridge Design Specifications, 9th Edition 2020 is the basis of analysis". Using the substitution symbol is an easy way to keep the description field current when updating the specifcation basis.
