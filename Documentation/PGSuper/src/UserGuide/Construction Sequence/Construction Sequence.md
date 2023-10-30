Construction Sequence {#ug_construction_sequence}
==============================================
The method for modeling the construction sequence depends on the method for computing prestress losses. When a traditional method for estimating prestress losses is used, such as the AASHTO LRFD approximate or refined methods, a typical construction sequence is assumed. The construction sequence must be explicitly defined for the time-step method. Each is described below.

Assumed Construction Sequence
------------------------
PGSuper has an assumed construction sequence that is typical for precast-prestressed girder bridges. Previous versions of this software modeled the construction sequence with stages. However, the construction sequence is now modeled as a series of Construction Events. There really aren't any differences between Stages and Events. Construction Events provide a better model for time-step analysis and we wanted to keep the concepts consistent.

The assumed construction events are:
1. Construct Girders (formerly Casting Yard Stage)
2. Erect Girders at the Bridge Site
3. Cast Deck (formerly Bridge Site Stage 1)
4. Final condition without live load (formerly Bridge Site Stage 2)
5. Final condition with live load (formerly Bridge Site Stage 3)

> Note: The Geometry Control Event occurs in the Final condition event above. This is when finished roadway elevations are established for analysis. Refer to @ref tg_vertical_geometry for details.

Certain construction activities are assumed to take place during each construction event. For example, in the Construct Girders Event the following activities take place:
1. Tension the strand
2. Cast the girder
3. Wait for the concrete to cure
4. Release the prestressing
5. Lift the girder and move it to storage
6. Wait to ship the girder to the bridge site.

Erecting the girders consists of:
1. Ship the girders to the bridge site
2. Erect the girders onto their permanent bearings
3. Remove temporary strands
4. Cast diaphragms

The construction activities generate analysis intervals. The analysis intervals model the intervals of time over which changes occur. For example, while the girder is in storage, creep and shrinkage are happening. Boundary conditions change during lifting and shipping so there are new states of stresses to analyze.

@ref ug_analysis_results describes the analysis intervals in more detail.

Explicit Construction Sequence
------------------
The construction sequence is an important element of the time step analysis. Forces, stresses, and displacements change with time due to changes in material properties, structure topology, loading conditions, and time dependent effects due to creep and shrinkage of concrete, and relaxation of strands and tendons. 

The timeline models the lifespan of a bridge with the sequence, timing, and duration of events that occur during the life of the bridge. At design time you typically will not know the exact construction schedule the bridge builder will use. Assumptions about the timing and sequence of construction activities will have to be made at design time.

> NOTE: LRFD 5.12.3.4.1 (*pre-2017: 5.14.1.3.1*) "The method of construction assumed for the design shall be shown in the contract documents."

Timeline Events
----------------
A natural model for describing the timeline is used. The timeline is described in terms of significant events in the life span of the bridge. Many different kinds of activities can take place during an event.

An event is a significant occurrence in the life span of a spliced girder bridge. There are not any built-in events. All events are defined by the designer.

> NOTE:	The default construction sequence for time-step analysis is the assumed sequence described above. All you have to do is adjust it to fit your project.

Events are defined by the time of their occurrence and the activities that take place during the event.

### Activities ###
An activity is something that happens during an event. Most of the activities defined by PGSplice are mandatory however you have a great deal of flexibility as to how you incorporate them into the timeline events. Some activities are instantaneous, such as erecting a segment or placing a load. Other activities occur over time such as placing the deck concrete and waiting for it to cure.

The activities modeled in PGSplice are:
* Construct Girders
* Erect Piers
* Erect Girders
* Cast Deck
* Apply Loads
* Geometry Control

> NOTE: The order of events and activities are important. Girders must be constructed before they can be erected, the deck and railing systems must be constructed before opening the bridge to traffic, and so on.

#### Construct Girder Activity ####
During this activity, precast girders are constructed at a precasting facility. Strands are stressed, concrete is placed into forms, time passes for curing and initial strength gain, prestress forces are released into the girder, and the girder is lifted and moved into its storage configuration. Prestressing strands experience initial relaxation and elastic shortening losses.

> TIP: The construction of girders can be modelled in any sequence. They can be modelled as being constructed all at once or one at a time based on an actual fabrication schedule.

#### Erect Piers ####
This activity models the construction of permanent abutments and piers. The sequence of erecting piers has no effect on the analysis. The only requirement is that supports are erected before girders are erected.

> TIP: The best technique is to model the erection of all abutments and piers in the first event.

#### Erect Girders Activity ####
During this activity, precast girders are removed from storage, transported to the bridge site, and erected onto the abutments and piers.

> TIP: 	Girdes can be erected one at a time, span by span, or all at once.

#### Cast Deck Activity ####
The bridge deck is cast. Time elapses while the concrete cures and the deck becomes composite with the girders.

#### Apply Loads ####
A loading is applied to the bridge. The loading can consist of
* Installation of the railing system
* Installation of an overlay
* Opening the bridge to live load
* Any user defined loading

#### Geometry Control ####
Geometry control activities define when the finished roadway elevations are established as well as when finished roadway specification checks and reports occur. Refer to @ ref tg_vertical_geometry for more information.

### Timeline Modeling ###
The timeline is modeled with the Timeline Manager. Select *Edit > Timeline* to open the Timeline Manager window.

The Timeline Manager window is used to manage events. The grid in the center of the window lists high level information about each event including the day of occurrence, elapsed time (duration), and description.

> TIP: You get to pick the event description. Using a meaningful description makes it easier to understand what is happening during the event.

#### Managing Events ####
Timeline events can be managed directly from the Timeline Manager window. The occurrence, elapsed time, and description of the event can be modified by entering values directly into the grid.

When changing the day of occurrence or elapsed time of an event the occurrences of subsequent events are automatically updated.
Unwanted events can be removed by selecting the event and pressing [Remove].

Press [Add] to create new events.

#### Assigning Activities to Events ####
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

#### Time Step Events ####
The time step analysis performed by PGSplice uses a non-linear step-by-step solution. Shorter duration events result in a more accurate analysis, especially when concrete elements are young and more sensitive to creep and shrinkage effects.

A typical timeline may have the precast segments constructed on day 0 and erected on day 28. There is a significant amount of elapsed time between these two events. You might consider adding time step events on days 3, 7, and 15 to improve the accuracy of the analysis.

> TIP: A time step event is an event without any activities. 

To create a time step event:
1. Press [Add] in the Timeline Manager window to create a new event.
2. Enter the day of occurrence
3. Enter a description ("Time Step" is a good description)

#### Analysis Intervals ####
A sequence of time step analysis intervals is created from the timeline events. This significantly reduces the amount of modeling you have to do. For example, all you have to do is model the construction of a precast girder. PGSuper will generate the analysis intervals that correspond to stressing the prestressing tendons, casting the concrete, curing of the concrete, releasing the prestress force onto the girder, moving the girder into storage and the elapsed time during storage. Similarly, all you have to do is model the erection of a girder and PGSuper will take care of details like transportation and removal of temporary strands after erection. You don't have to explicitly model every nitty-gritty detail during the life span of a bridge.

Analysis Intervals are discussed in detail in the @ref ug_analysis_results.

