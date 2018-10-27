Live Load Distribution Factors {#ug_dialogs_live_load_distribution_factors}
==============================================

Define the method for computing live load distribution factors.

Option 1 - Compute Live Load Distribution Factors in Accordance With the Currently Selected Project Criteria
--------------------------------------------------------------------------------------------------------------
When this option is selected, live load distribution factors are computed using the methods specified in the Project Criteria. The Ranges Of Applicability (ROA) of the live load distribution factor equations, are girder-type dependent and defined in LRFD 4.6.2.2. By default, analysis will be halted if any range of applicability requirement is violated. However, there may be cases when you want to override this behavior, and options are given below:

### When parameters are Outside of the Range Of Applicability (ROA) requirements:
Select an option from the following

Option | Description
-------|-----------
Halt the Analysis And Issue a Warning | This is the default behavior. Analysis will be halted unless you override this.
Ignore the ROA Requirements and Perform the Analysis Using the Equations | The LRFD equations will be used for computation of distribution factors regardless of their applicability.
Use the Lever Rule Only for Cases Outside of the ROA Requirements | The LRFD equations will be used for computation of distribution factors where parameters are inside of the Range of Applicability. However, the lever rule will be used for cases outside the ROA for the equation. This option may seem safer than the previous option, however, the same amount of caution should be applied when using it.

> NOTE: Ignoring the range of applicability is generally **NOT RECOMMENDED**. This option should only be selected for structures where you need to control the computation of live load distribution factors. Checks using hand calculations, or other methods, should always be done to insure that the live load distribution factors are computed as you intended.

Option 2 - Compute All Live Load Distribution Factors Using the Lever Rule
---------------------------------------------------------------------------
When this option is selected, all distribution factors are computed using the Lever Rule. Note that the LRFD skew factor equations are still applied, however, any ROA requirements for the skew equations are ignored. Again, use caution when using this option.

Option 3 - Directly Input All Live Load Distribution Factors
--------------------------------------------------------------
When this option is selected, you input the live load distribution factors that are to be used in the analysis. LRFD 4.6.2.2.1 defines live load distribution factors for positive moment, negative moment near interior supports, negative moment in main spans, shear, and reaction. 

> TIP: If the configuration of the bridge you are designing doesn't conform to the simplified method of analysis described in the AASHTO LRFD Bridge Design Specifications, compute the live load distribution factors using another method and then input them into this dialog.

Two input grids are provided to accommodate the required input; one for inputting factors in the main span; and another for inputting factors near interior supports. Toggle between the two grids by selecting the "Span Factors" and "Abutment/Pier Factors" options.

> NOTE: **You must enter distribution factors in both the Span Factors and Abutment/Pier Factors grids.**.

### Using the User-Input Grid ###
The following symbols are used in the input grids

Item | Description
-----|---------
+M | Positive Moment
-M | Negative Moment
V | Shear
R | Reactions
 

The tab control at the bottom of the input grid is used to select which span(s) (or pier(s)) are to be edited. In the same manner as Microsoft Excel, multiple tabs can be selected using Ctrl-Click and Shift-Click. When multiple tabs are selected, grid input operations are performed on all selected spans or piers.

Live load distribution factors can be input into the grid by clicking the mouse on individual cells. The arrow keys can also be used to navigate between cells. The grids work much like Microsoft Excel. In fact, you can copy and paste data to/from Excel as long as the selected ranges are the same size.

#### Selecting Ranges In the Grids ####
Copy and paste operations are typically done on a grid by first selecting a range of cells. There are several ways to select multiple grid cells (ranges):
* Click and drag the mouse anywhere on the grid
* Hold the Shift button while using the arrow keys
* Click on header rows to select an entire row
* Click on header columns to select an entire column
* Click the top-left header box to select the entire grid
* Hold the Ctrl key while selecting to select multiple ranges (note this only may be used when copying data from the grid)

#### Copying and Pasting  ####
Copying and Pasting can be performed from the keyboard by using the following Windows standard key combinations:
* Ctrl-C to copy selected data from the grid to the Windows Clipboard
* Ctrl-V to paste data from the Clipboard to the selected cells
* Ctrl-X to cut data from the grid and copy it to the Windows Clipboard

The mouse may also be used to copy and paste by clicking on the right mouse button over selected cells. This will bring up a context menu with the options "Copy" or "Paste".

#### Selecting Multiple Tabs On the Grid ####
Select multiple grid tabs by holding the Ctrl or Shift button while clicking the left mouse button on the tabs. This allows data input onto multiple grids simultaneously in the same manner as the "Drill Down" feature in Excel.

When multiple tabs are selected, any input operations (typing, and pasting) are performed on all selected grids.

### Fill With Computed Values ###
There may be occasions when you wish to use some of the computed live load distribution factors and manually input others. 
 
Press [Fill with Computed Values...] to open the @subpage ug_dialogs_fill_live_load_distribution_factors window.

> TIP: Fill With Computed Values is a one-time operation. Values will **NOT** be automatically updated if bridge data is later modified.
