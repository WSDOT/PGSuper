General {#ug_library_dialogs_load_rating_criteria_general}
==============================================
Load Rating Criteria library entries define custom Load Rating Criteria. Load Rating Criteria is based on the criteria defined in the AASHTO Manual for Bridge Evaluation. 

Item | Description
----|--------
Entry Name | Each Load Rating Criteria entry must have a unique name. The name may contain any characters and can be up to 32 characters in length.
Description | Provide a description for this Load Rating Criteria.
Rating Criteria Basis | Select the version of the AASHTO Manual for Bridge Evaluation that you would like to use along with the options defined in this entry. Checking the box causes the most recent edition of AASHTO's The Manual for Bridge Evaluation that is supported by the software to be used. When new editions of the manual are implemented, the rating criteria basis will automatically update to that edition.
Always load rate when performing a specification check | When this box is checked, a load rating will always be performed during a design specification check.

The Description field can include the \%MBE\% symbol. When writing the load rating criteria description into reports the \%MBE\% symbol is replaced with the name of the currently selected Manual for Bridge Evaluation. For example, if the Description field contained "%MBE% is the basis of analysis" the resulting text would be "The Manual for Bridge Evaluation, Third Edition 2018 is the basis of analysis". Using the substitution symbol is an easy way to keep the description field current when updating the load rating basis.
