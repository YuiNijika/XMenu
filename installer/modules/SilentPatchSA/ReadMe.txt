SilentPatch 1.1 for GTA SA
Build 33
Last update - 25.10.2024 (re-released 2.11.2024)


DESCRIPTION

	SilentPatch for the 3D-era Grand Theft Auto games is the first and flagship release of the "SilentPatch family",
	providing numerous fixes for this beloved franchise. SilentPatch addresses a wide range of issues,
	from critical fixes for crashes and other blockers to various major and minor improvements identified by
	the passionate community in these games over decades. SilentPatch does not alter the core gameplay experience,
	making it an optimal choice for both first-time players and the old guard returning for yet another playthrough.

	Featured fixes:

	Fixes marked with ** are supported in 1.0, Steam, and Rockstar Games Launcher versions.
	Fixes marked with * require 1.0.

	CRITICAL FIXES
	Compatibility issues, crashes, progression blockers

	** The 14ms frame delay has been removed. As a result, the game now locks properly on 30 FPS instead of 25 FPS.
	** More precise frame limiter, reducing lag spikes a bit when playing with the Frame Limiter on.
	** The mouse should not lock up randomly when exiting the menu on newer systems anymore.
	** DirectPlay dependency has been removed - this should improve compatibility with Windows 8 and newer.
	** Path to the User Files directory is now obtained using a dedicated API call rather than a legacy registry
	   entry, future-proofing the games more.
	** Fixed an issue that would cause games to freeze if III/VC/SA were running at the same time.
	** Fixed an occasional crash when minimizing the game while standing next to a mirror.
	** Fixed a crash on car explosions - most likely to happen when playing with a multi-monitor setup.
	** Fixed a crash when entering advanced display options on a dual monitor machine after: starting the game on the
	   primary monitor in maximum resolution, exiting, starting again in maximum resolution on the secondary monitor.
	   The secondary monitor's maximum resolution had to be greater than the maximum resolution of the primary
	   monitor.
	** Fixed an occasional crash occurring when standing next to escalators.
	** Fixed a crash occurring when the vending machine was continuously used for an extended period.
	** Fixed a streaming-related deadlock, which could occasionally result in the game being stuck on a black screen
	   when entering or exiting interiors (this is the issue people used to fix by setting CPU affinity to one core).
	** A significant memory leak when taking photos with an in-game camera has been fixed.
	** Fixed a crash that occurred when mashing the replay button near groups of gang members holding items.
	** Fixed a crash that occurred when starting a cutscene after playing a replay where CJ wore different clothes
	   from what he is currently wearing.
	** Fixed a crash that occurred when playing back a replay with CJ having a different body type
	   (fat/muscular/normal) than his current one.
	* Game timers now tick more accurately, making them not freeze if the framerate exceeds 1000 frames per second; in
	  other words, this fixes occasional freezes on fadeouts if playing with the Frame Limiter off.
	* A heap corruption in one place is now fixed (did not affect gameplay but could potentially make the game crash).
	* Traveling far away from the map will no longer trigger the extra gang territories glitch, nor will it corrupt
	  the Taxi Driver submission.
	* A 1.0 no-DVD-only bug where recruiting gang members would stop working after activating a replay has been fixed
	  (contributed by Wesser).
	* Fixed a 1.01 only tiny memory leak which occurred every time the player switched a radio station.
	* The gym glitch ("You have worked out enough..." showing infinitely) has been fixed.
	* Saving in Madd Dogg's mansion will no longer trigger the missing basketball glitch.
	* Fixed an occasional soft lock in "Mountain Cloud Boys" - the player will not freeze after arriving at the
	  meeting anymore.
	* Possible soft lock in "Sweet's Girl" initial cutscene fixed.
	* Fixed a script error in how Driving and Bike Schools destroyed the cones used in lessons, where random objects
	  from the map could be destroyed instead. This glitch was most famously known as the "Blackboard glitch"
	  (contributed by Wesser).
	* Fixed a script error in "Air Raid" where the player's heavy weapon (like a minigun) would disappear after the
	  mission.

	OTHER FIXES
	All the remaining, non-critical fixes.

	** The mouse's vertical axis sensitivity now matches the horizontal axis sensitivity.
	** The mouse's vertical axis does not lock during camera fade-ins now.
	** Fixed sun lens flare effect not appearing with AMD/Intel graphics cards.
	** Fixed an issue introducing graphical artifacts from ped animations with high RAM usage - the so-called
	   "streaming memory bug".
	** Fixed a bug causing cheat-spawned melee weapons to be forcibly replaced by other melee weapons upon walking
	   into a pickup.
	** Some car panels now swing after the car's explosion (like they were meant to be but the code forcibly fixed
	   them immediately after damage).
	** Num5 is now bindable (like in the 1.01 patch).
	** Fixed a glitch where random cars would end up being impounded to the garage, replacing player's vehicles.
	** Impound garages can now only impound cars and bikes, as other vehicle types are either too big or cannot leave
	   the garage.
	** A muzzle flash will now show up when firing the last bullet from the clip.
	** Script sprites now have bilinear filtering applied.
	** Car generator counters now work properly for generators with a fixed number of spawns.
	** Randomizer error causing peds not to spawn in some areas has been fixed.
	** Randomizer error causing prostitutes to be quiet during solicitation has been fixed.
	** Text boxes can now show together with a Mission Passed text.
	** Mirror reflection doesn't break with Anti-Aliasing enabled anymore.
	** With Visual FX Quality set to Very High, mirror reflection quality has been bumped.
	** The Anti-Aliasing option has been altered - instead of listing 1, 2, and 3 options (which in fact are 2x/2x/4x
	   MSAA), the game will now show proper MSAA values from 2x up to 16x (depending on max MSAA level supported by
	   the graphics card).
	** Colliding with another car will now damage proper parts on both cars - previously, both cars got damaged the
	   same way.
	** Slightly reduced stencil shadows memory overhead.
	** Fixed an AI issue where enemies became too accurate after the player had been in the car earlier.
	** Alt + F4 now works properly.
	** FILE_FLAG_NO_BUFFERING flag has been removed from IMG reading functions - speeding up streaming.
	** Metric-to-imperial conversion constants have been replaced with more accurate ones.
	** Dancing minigame timings have been improved, now they do not lose accuracy over time depending on the PC's
	   uptime.
	** Car generators placed in interiors are now placed correctly - this 'unhides' two vehicles in Madd Dogg's
	   mansion, which were always there but they were not visible.
	** Bombs in cars stored in garages now save properly.
	** Streaming has been greatly improved during Supply Lines mission (or more general, any time when using an RC
	   vehicle) - it now behaves as expected, as opposed to displaying LODs way too quickly.
	** The health triangle displaying when aiming at peds is now properly orientated (it's now upside down) for peds
	   the player can recruit.
	** Setting a BMX on fire will no longer set CJ on fire.
	** Keyboard input latency decreased by one frame.
	** Coronas now properly rotate as the camera gets closer to them, like on the PS2.
	** Light shadows from fire now show up properly.
	** Fixed parachute animations.
	** "Keep weapons after wasted" and "keep weapons after busted" are now reset on the New Game.
	** Fixed a glitch allowing bikes without engines to spawn.
	** Allowed extra6 parts to be picked when a random extra is to be picked.
	** Fixed in-car camera mouse behavior when looking left/right/behind.
	** Steam and RGL versions have proper aspect ratios now.
	** Steam/RGL versions will now default Steer with Mouse option to disabled, like in 1.0/1.01.
	** Wind animations now apply to CJ's clothes when driving a Quadbike (contributed by Wesser).
	** Quadbike's handlebar movement now matches CJ's steering animations when driving at low speeds (contributed by
	   Wesser).
	** Fixed the sitting radio station change animation playing in boats where CJ stands upright (contributed by
	   Wesser).
	** Pickups, car generators, and stunt jumps spawned through the text IPL files now reinitialize on a New Game.
	   Most notably, this fixes several pickups (like fire extinguishers) going missing after starting a new game.
	** Fixed crosshair issues when the sniper rifle is equipped and a photo is taken by a gang member (contributed by
	   Wesser).
	** Fixed an issue where biker cops kept shooting at the player even after losing the wanted level.
	** Fixed an SCM interpreter issue where a request to spawn a biker cop with a type PEDTYPE_COP spawned a normal
	   cop instead (contributed by Wesser).
	** Racing checkpoints are now correctly colored even if no enex markers were displayed on-screen before.
	** Adjusted the probability of traffic vehicles turning on their lights to match the PS2 version, including a low
	   chance that they may never turn them on.
	** Fixed an issue where vehicles exploded twice if the driver left the car before the explosion.
	** Script randomness is now 16-bit instead of 15-bit.
	** Fixed black shooting stars, they are now white as originally intended.
	** Improved the behavior of exploded cars losing their wheels - now the car sinks from the side of the detached
	   wheel instead of always sinking from the front left side. The rear right wheel can now also be detached during
	   an explosion.
	** Slightly improved the spawning logic of planes. While they can still crash after spawning, this should now
	   occur less frequently.
	** Hovering with a jetpack is now possible using the keyboard controls by holding the next/previous weapon buttons
	   simultaneously (Q + E by default).
	** In missions set during the riots, gang members in the player's group will no longer be targeted by the police
	   helicopter, fixing a glitch where they'd abandon the player unexpectedly.
	** Fixed a bug where stealing the car from the passenger side while holding the throttle and/or brake button would
	   kill the driver, or briefly resurrect them if they were already dead.
	** Credits now scale to resolution correctly.
	** Mission title and 'Mission Passed' texts now stay on screen for the same duration, regardless of screen
	   resolution.
	** The heat haze effect now rescales correctly when changing the resolution in-game.
	** The underwater ripple effect is now consistent across all resolutions.
	** Heat-seeking missile crosshair and the weapon crosshair shown while aiming with a gamepad now properly scale to
	   resolution.
	** The boundaries of the cursor on the Map screen, and the cursor itself now scale to resolution correctly
	   (contributed by Wesser).
	** The inner padding of the text boxes with a background now scales to resolution correctly (contributed by
	   Wesser).
	** Nitrous will no longer regenerate faster when reversing the car (contributed by Wesser).
	** Hydra's jet thrusters no longer randomly fail to appear (contributed by B1ack_Wh1te).
	* Detached vehicle parts will now keep the same color and lighting as the vehicle they came from.
	* Detached vehicle parts are now rendered from both sides.
	* Resolved single-pixel wide seams showing on the Map screen with Anti-Aliasing enabled.
	* Several vehicles now have extra animated components: Phoenix hood scoops, Sweeper brushes, Newsvan antenna,
	  radars on several boats, and extra flaps on Stuntplane and Beagle.
	* Animated engine components on Bandito, BF Injection, and Hotknife will not animate if the engine is off.
	* Firetruck (firela variant) now has a functional ladder - it can be raised by moving the right analog stick
	  down/pressing Num2.
	* artict3 trailers can now be chained (as it was most likely intended since the model has a hook dummy which was
	  not functional until now).
	* Tug now has a functional tow bar (the model has a hook dummy which was not functional until now).
	* DFT-30 left middle wheel now displays properly (the game now accepts a typo present in its hierarchy).
	* Dumper's suspension is now animated (the game now accepts a typo present in its hierarchy).
	* Uranus tail light coronas are now placed correctly, instead of lighting up in the car's interior (the game now
	  accepts a typo present in its hierarchy).
	* Stats counted in kilograms are now displayed correctly.
	* 16:9 resolutions are now selectable (like in the 1.01 patch).
	* Wet road reflections render properly again (just like with Road Reflections Fix).
	* Hunter's interior does not disappear when viewed through the glass door panel.
	* Weapons are now visible when viewed through a vehicle window.
	* Holding a weapon will not cause some objects to be incorrectly lit anymore.
	* Blown-up vehicles are now correctly colored and no longer shine (like in the 1.01 and Steam versions).
	* Dirty cars are now able to get clean (like in the 1.01 patch).
	* Each car has a unique number plate now.
	* Custom number plates now show up correctly in all cases.
	* Custom number plates are now also allowed on bikes.
	* Number plates are now bilinear filtered, resulting in a smoother look.
	* Vehicle lights do not get dark after being initially lit anymore (like on the PS2).
	* Moonphases now show up correctly, like on the PS2 version (only when playing in 32-bit color mode).
	* Toggling car lights on does not make windows invisible when viewed from inside anymore.
	* Illumination value from timecyc.dat now accepts any float value in 0.0-2.0 ranges, not just 0.0, 1.0, and 2.0.
	* In addition, if the illumination value is absent from the timecycle (like on a stock PC timecycle), the game
	  will now default to 1.0.
	* Lights now get cast on vehicles and peds properly - previously, they would disappear under some conditions.
	* Muzzle flash looks better now.
	* Coronas no longer have a Z test forced all the time - as a result, the sun glare now matches the original PS2
	  version.
	* With User Tracks automatic scan enabled, MP3 playback will now work properly if QuickTime is not installed.
	* PCM WAVE has been expanded to also accept additional profiles (Now 8/16/24bits, Mono/Stereo, and up to 48Khz).
	* PCM WAVE files with an ID3-TAG will now also work with the game.
	* Temple and Queens are now correctly called on the police scanner.
	* Impound garages now function correctly, allowing the player to recover his last vehicle after it had vanished
	  after a mission started.
	* In addition, impound garages will now store the player's car when he's busted.
	* The streamed entity list has been expanded a bit, so now the game world shouldn't flicker when looking down with
	  high Draw Distance settings anymore.
	* Mouse rotates an airborne car only with the Steer with Mouse option enabled.
	* The Towtruck tow hitch does not get bugged after it has been fixed anymore.
	* Plane doors don't corrupt after the plane has been fixed anymore.
	* Fixing a plane will now reset its moving props to an undamaged state.
	* Several vehicle components (most notably, Rumpo's front bumper and Bandito's moving prop) will not get glitched
	  after the vehicle has been fixed anymore.
	* Weapons and a jetpack now cast proper shadows.
	* Crosshair doesn't mess up the weapon icon when on a jetpack anymore.
	* Free resprays will not carry on a New Game now.
	* Fixed ambulance and firetruck dispatch timers - they reset on New Game now.
	* Several stat counters now reset on New Game - so the player will not level up quicker after starting New Game
	  from a save.
	* The "To stop Carl..." message now resets properly on New Game.
	* Previously present only on the PS2, the 'Cars drive on water' cheat is now toggleable - its string is SEAROADER.
	* Very long loading times will now loop loading screens, as opposed to fading to white.
	* Rhino does not gain extra wheels after being fixed anymore.
	* Pushing pedestrians against the wall with a vehicle will not trigger passenger's voice lines anymore - instead,
	  now they are triggered when the player runs over pedestrians.
	* Pay 'n Spray will no longer clean the car BEFORE the garage doors close - now it cleans them while the car is
	  hidden behind the garage door.
	* Fixed a bug where paint jobs would vanish from cars stored in a garage if they were stored without looking at
	  them.
	* Helicopter rotors and plane propellers now work correctly. They now have a blurring effect present in Vice City
	  and the PS2 version of San Andreas.
	* Dual rear wheels now show up properly (Yosemite, Tanker, etc.).
	* Quadruple Stunt Bonus now works correctly.

	ENHANCEMENTS
	Any changes that don't strictly fix game bugs.

	** If the settings file is absent, the game will now default to your desktop resolution instead of 800x600x32.
	** When playing on Visual FX Quality higher than Low, the game will now cast up to 6 lights on each model both
	   indoors and outdoors (on Low details, the game's stock behavior has been kept - allowing up to 4 lights per
	   model outdoors and 6 indoors).
	** Censorships from Steam and RGL versions for German players have been removed.
	** Remade the monitor selection dialog, adding several quality-of-life improvements - such as remembering the
	   selected screen, modern styling, and an option to skip the dialog appearing on every game launch.
	** The Steam/RGL version of the game will no longer reject 1.0/1.01 saves (still, a compatible SCM is needed for
	   the save to work).
	** Sliding mission titles and odd job texts from the GTA III beta can now be re-enabled (off by default).
	** An unused 'Minimal HUD' feature can now be re-enabled (off by default).
	* IMGs bigger than 4GB are now handled properly.
	* User Tracks now supports the FLAC codec (Only 8/16/24bits, Mono/Stereo, and up to 48Khz).
	* EAX/NVIDIA splashes are now removed.
	* Subtitle and Radio text sizes can now be toggled between the original release and the updated Steam version.
	* Area name color now matches the gang color of the gang that owns that territory (off by default).
	* The "True Invincibility" option has been added - with the option enabled, police helicopters will not hurt the
	  player when they have an Invincibility cheat enabled (off by default).
	* Made the game select metric/imperial units based on system locale settings.
	* Sun reflections on peds and vehicles now change direction depending on the time of day, like in III and VC (off
	  by default).


INSTALLATION

	As SA doesn't load ASI files by default, you'll need an ASI loader. In case you don't have one (if you
	use any ASI plugin such as CLEO, you can skip this step), you can download an ASI loader created by
	myself from the following link (under 'Silent's ASI Loader'):
	https://cookieplmonster.github.io/mods/gta-sa/#asiloader

	For the Rockstar Games Launcher version, you are better off using Ultimate ASI Loader,
	as the launcher forcibly overwrites modified game files even if automatic updates are disabled:
	https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases

	The rest of the installation is easy as pie. Just extract the archive contents to your GTA SA directory
	and that's all. Make sure you check the INI!

	Alternatively, San Andreas Mod Loader can be used to install this plugin (by the time this mod was created, Mod Loader supports 1.0 ONLY):
	http://gtaforums.com/topic/669520-sarel-mod-loader/


SUPPORTED GAME VERSIONS

	* GTA SA 1.0 (all versions)
	* GTA SA newsteam r2 (9 Dec 2014; partial support)
	* GTA SA Rockstar Games Launcher version (partial support)


CREDITS

	SilentPatch includes code contributions from:

	aap
	B1ack_Wh1te
	DK22Pac
	Fire_Head
	Nick007J
	NTAuthority
	Sergeanur
	spaceeinstein
	Wesser


PATREON & GITHUB SPONSORS SUPPORTERS

	BuckoA51
	Calinou
	ddm
	deSSy2724
	Iman Shahani
	Jack
	kiwidog
	Lagahan
	m0b
	mirrorsedger
	MrNicolaZenk
	mrtaufner
	nta
	Phoenyx12
	retrozone.co
	SirJarko
	switchblade
	TJGM
	Vetle Ledaal


SPECIAL THANKS

	People who helped identify issues, tested the patch or were generally supportive:

	Ash_735
	Blackbird88
	gamerzworld
	iFarbod
	Inadequate
	LonesomeRider
	mirh
	Reyks
	Tomasak


CONTACT

	zdanio95@gmail.com - e-mail
	cookieplmonster - Discord

Subscribe to my YouTube channel for more footage from my mods!
https://www.youtube.com/user/CookiePLMonster

Follow my Twitter account to be up to all my mod updates!
http://twitter.com/__silent_

Also take a look at my blog, featuring modding and programming related articles and more!
https://cookieplmonster.github.io/
