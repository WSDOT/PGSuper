Limits and Warnings {#ug_library_dialogs_project_criteria_limits_and_warnings}
==============================================
The parameters on the Limits and Warning tab are used to set common sense limits. For example, a perfectly valid input value for concrete strength is 15 ksi, however if local fabricators cannot produce concrete with such a high strength, it is not reasonable that such a value be used in your design. The Limits and Warnings parameters define common sense limits on certain input values and will post informational warnings in the Status Center to alert designers when these limits are exceeded.

General Warnings
----------------

Item | Description
-----|-----------
Warn if stirrup zone length are incompatibility with stirrup spacing | Check this box to issue a warning if input stirrup spacing do not fit exactly within stirrup zones (i.e., if the number of stirrups within a zone is not an integer value). Note that a failure of this check is only a warning, not a spec check failure. The warning is issued at the top of the Spec Check report in the Spec Check Summary section.
Warn if there is a potential for girders to sag under dead load effects | Check this box to evaluate the final camber and issue a warning if downward deflections exceeds the upward camber immediately before deck casting. The evaluation for girder sag can be based on <ul><li>Average camber</li><li>Upper bound camber</li><li>Lower bound camber</li></ul>

Concrete Limits
---------------
Limits on several different concrete properties for normal weight and lightweight concrete are specified in the Concrete Limits group. Warnings are posted in the Status Center if these limits are exceeded. These limits do not affect specification checking or girder design.
