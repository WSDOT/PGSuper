Fill Live Load Distribution Factors with Computed Values {#ug_dialogs_fill_live_load_distribution_factors}
==============================================
Automatically fill selected entries in the live load distribution factors grids with computed values. Data in the input grid data will be replaced by computed or input values for the selected girder(s) or Pier(s).

This is a one-time operation. Values will **NOT** be automatically updated if the bridge is later modified.

> NOTE: If computation fails for any girder (probably due to range of applicability reasons), an error message will be posted and grid data remains unchanged.

Select Live Load Distribution Factor Computation Method
-------------------------------------------------------
Select the method to compute the live load distribution factors. 

### Option 1 - Compute Live Load Distribution Factors in accordance with current Project Criteria. ###
The options for computation methods and Range of Applicability are the same as those on the [Live Load Distribution Factors](@ref ug_dialogs_live_load_distribution_factors) window. However, the data will only be filled once and then you can change any data in the grid to suit your needs.

### Option 2 - Compute all Live Load Distribution Factors using the Lever Rule. ###
Selecting this option causes all live load distribution factors to be computed with the lever rule.

### Option 3 - Fill Live Load Distribution Factors With a Single Value. ###
This option allows a single value to be assigned to all selected girders and piers in the grid.

Select Girders(s) to be Filled
-------------------------------
Select the span and girders that the live load distribution factors are to be computed for.

Select Piers(s) to be Filled
-------------------------------
Select the pier and girders that the live load distribution factors are to be computed for.


Press [Fill Input Grid Using Selected Options] cause the live load distribution factors to be computed, the grids to be populated, and close this window.
