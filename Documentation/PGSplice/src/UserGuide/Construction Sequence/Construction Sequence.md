Construction Sequence {#ug_construction_sequence}
==============================================
The construction sequence is an important element of the time step analysis. Forces, stresses, and displacements change with time due to changes in material properties, structure topology, loading conditions, and time dependent effects due to creep and shrinkage of concrete, and relaxation of strands and tendons. 

The timeline models the lifespan of a bridge with the sequence, timing, and duration of events that occur during the life of the bridge. At design time you typically will not know the exact construction schedule the bridge builder will use. Assumptions about the timing and sequence of construction activities will have to be made at design time.

> NOTE: LRFD 5.12.3.4.1 (*pre-2017: 5.14.1.3.1*) "The method of construction assumed for the design shall be shown in the contract documents."

Timeline Events
----------------
PGSplice uses a natural model for describing the timeline. The timeline is described in terms of significant events in the life span of the bridge. Many different kinds of activities can take place during an event.

An event is a significant occurrence in the life span of a spliced girder bridge. PGSplice does not have any built-in events. All events are defined by the designer.

> NOTE:	PGSplice Project Templates have a timeline with events and associated activities. All you have to do is adjust the timeline to fit your project.

Various activities occur during an event. For example, you can create an event called "Phase 1 Post-Tensioning". During this event tendons can be stressed and temporary supports removed.

Events are defined by the time of their occurrence and the activities that take place during the event.

Activities
----------
An activity is something that happens during an event. Most of the activities defined by PGSplice are mandatory however you have a great deal of flexibility as to how you incorporate them into the timeline events. Some activities are instantaneous, such as erecting a segment or placing a load. Other activities occur over time such as placing the deck concrete and waiting for it to cure.

The activities modeled in PGSplice are:
* Construct Segments
* Erect Piers/Temporary Supports
* Erect Segments
* Cast Closure Joints
* Stress Tendons
* Remove Temporary Supports
* Cast Deck
* Apply Loads
* Roadway Geometry Control

> NOTE: The order of events and activities are important. Segments must be constructed before they can be erected, closure joints must be cast before tendons can be stressed, deck and railing systems must be constructed before opening the bridge to traffic, and so on.

### Construct Segments Activity ###
During this activity, precast segments are constructed at a precasting facility. Strands are stressed, concrete is placed into forms, time passes for curing and initial strength gain, prestress forces are released into the segment, and the segment is lifted and moved into its storage configuration. Prestressing strands experience initial relaxation and elastic shortening losses.

> TIP: The construction of segments can be modelled in any sequence. They can be modelled as being constructed all at once or one at a time based on an actual fabrication schedule.

### Erect Piers/Temporary Supports Activity ###
This activity models the construction of permanent abutments and piers as well as temporary supports such as erection towers and strong backs. In this first version of PGSplice, the sequence of erecting piers and temporary supports has little effect on the analysis. The only requirement is that supports are erected before segments are erected. In future versions of PGSplice, temporary supports can be added after a segment is erected to model touch shoring and temporary support conditions during future deck replacement. 

> TIP: The best technique is to model the erection of all abutments, piers, and erection towers in the first event and model erection of strong backs in the same event as when the segments they are attach to are erected.

### Erect Segments Activity ###
During this activity, precast segments are removed from storage, transported to the bridge site, and erected onto the abutments, piers, and temporary supports.

> TIP: 	Segments can be erected one at a time, span by span, or all at once.

### Cast Closure Joints Activity ###
Concrete for the closure joints are casted. Time elapses while the concrete cures and becomes composite with the girders.

> TIP: Closure joints can be cast individually, all at once, or in any combination you deem appropriate.

### Stress Tendons Activity ###
Post-tensioning tendons are stressed. 

> NOTE: Secondary reactions due to post-tensioning will be carried by the temporary erection towers if they have not been previously removed or are not removed during the event in which this activity takes place.

> TIP: Tendons can be stressed all at once or in any sequence. When multiple tendon stressing activates are used for a girder, the effects stressing the tendon has on previously stressed tendons is computed.

### Remove Temporary Supports Activity ###
Temporary supports are removed from the model. Reactions forces are transferred to the girder.

### Cast Deck Activity ###
The bridge deck is cast. Time elapses while the concrete cures and the deck becomes composite with the girders.

### Apply Loads ###
A loading is applied to the bridge. The loading can consist of
* Installation of the railing system
* Installation of an overlay
* Opening the bridge to live load
* Any user defined loading

### Roadway Geometry Control ###
Roadway geometry control activities are used to define milestone events for the bridge vertical geometry analysis and checking. There are three different types of events:

Activity | Description
-----|--------------
The Roadway Geometry Control Event (GCE) | This activity defines when bridge elevations are synchronized to roadway elevations and typically occurs at time first service. Elevation specification checks are performed, and elevations are reported. *There can be only one GCE in the timeline.*
Specification Checking Events | Perform finished roadway elevations specification checks and report finished elevations.
Reporting Events | Report finished elevations only

> Notes:
> - There can be only one Geometry Control Event
> - Spec checking and reporting activities can occur in multiple events, but can only be added to events after deck placement.
> - Top of deck elevations will change over the life of the structure due to time-dependent effects. This activity enables reporting or spec checking to monitor differences between computed top of deck elevation verses the design roadway profile over time.

Learn about how the Roadway Geometry Control Event (GCE) effects vertical geometry analysis in @ref tg_vertical_geometry.

Timeline Modeling
------------------
The timeline is modeled with the Timeline Manager. Select *Edit > Timeline* to open the Timeline Manager window.

The Timeline Manager window is used to manage events. The grid in the center of the window lists high level information about each event including the day of occurrence, elapsed time (duration), and description.

> TIP: You get to pick the event description. Using a meaningful description makes it easier to understand what is happening during the event.

### Managing Events ###
Timeline events can be managed directly from the Timeline Manager window. The occurrence, elapsed time, and description of the event can be modified by entering values directly into the grid.

When changing the day of occurrence or elapsed time of an event the occurrences of subsequent events are automatically updated.
Unwanted events can be removed by selecting the event and pressing [Remove].

Press [Add] to create new events.

### Assigning Activities to Events ###
Activities must be assigned to the timeline events. Begin by finding the event you want to modify. Press [Edit] to open the Event window. 
 
Within the Event window you can manage the various activities that take place during the event. The activities are shown in the Event Activities grid. 
To add an activity:
1. Press [Add] and select an activity from the menu
2. Enter the activity details

To edit the details of an activity:
1. Select the activity
2. Press [Edit] to open the activity details window.
 
To remove an activity:
1. Select the activity
2. Press [Remove]

Some activities have duration. The Cast Deck activity includes placing the deck concrete and waiting for the concrete to gain sufficient strength so that it can be considered composite with the girder. The details of the Cast Deck activity include defining the age when continuity is achieved.
 
The minimum elapsed time of an event is the duration of the longest activity within the event.

### Time Step Events ###
The time step analysis performed by PGSplice uses a non-linear step-by-step solution. Shorter duration events result in a more accurate analysis, especially when concrete elements are young and more sensitive to creep and shrinkage effects.

A typical timeline may have the precast segments constructed on day 0 and erected on day 28. There is a significant amount of elapsed time between these two events. You might consider adding time step events on days 3, 7, and 15 to improve the accuracy of the analysis.

> TIP: A time step event is an event without any activities. 

To create a time step event:
1. Press [Add] in the Timeline Manager window to create a new event.
2. Enter the day of occurrence
3. Enter a description ("Time Step" is a good description)

### Analysis Intervals ###
PGSplice creates a sequence of time step analysis intervals from the timeline events. This significantly reduces the amount of modeling you have to do. For example, all you have to do is model the construction of a precast segment. PGSplice will generate the analysis intervals that correspond to stressing the prestressing tendons, casting the segment concrete, curing of the concrete, releasing the prestress force onto the girder segment, moving the segment into storage and the elapsed time during storage. Similarly, all you have to do is model the erection of a segment and PGSplice will take care of details like transportation and removal of temporary strands after erection. You don't have to explicitly model every nitty-gritty detail during the life span of a bridge.

Analysis Intervals are discussed in detail in the @ref ug_analysis_results.

