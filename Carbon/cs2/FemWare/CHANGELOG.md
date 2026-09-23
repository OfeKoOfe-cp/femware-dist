# Velocity CS2 — Complete Feature Changelog & Reference

A 1:1 inventory and changelog of all features, subsystems, and visual implementations in **Velocity CS2**.

---

## Table of Contents
1. [Combat Engine](#1-combat-engine)
   - [Ragebot](#ragebot)
   - [Legitbot](#legitbot)
   - [Triggerbot](#triggerbot)
   - [Lag Compensation & Backtracking](#lag-compensation--backtracking)
   - [Extrapolation](#extrapolation)
   - [Desync Resolver](#desync-resolver)
   - [Penetration & Autowall Engine](#penetration--autowall-engine)
   - [Zeusbot (Taserbot)](#zeusbot-taserbot)
   - [Knifebot](#knifebot)
   - [Anti-Aim & Desync](#anti-aim--desync)
   - [Fake Lag](#fake-lag)
   - [Peek Assistance](#peek-assistance)
2. [Visuals & ESP Engine](#2-visuals--esp-engine)
   - [Player ESP (Overlay)](#player-esp-overlay)
   - [Chams System](#chams-system)
   - [Player Glow](#player-glow)
   - [Item ESP (Dropped Weapons & Gear)](#item-esp-dropped-weapons--gear)
   - [Projectile ESP & Trajectories](#projectile-esp--trajectories)
   - [Objective ESP (Bomb & Defuse)](#objective-esp-bomb--defuse)
   - [Radar & Radar Stream](#radar--radar-stream)
3. [Movement Engine](#3-movement-engine)
   - [Subtick Bunnyhop](#subtick-bunnyhop)
   - [Auto Air-Strafer](#auto-air-strafer)
   - [Subtick Jumpbug](#subtick-jumpbug)
   - [Subtick Edgebug](#subtick-edgebug)
   - [Auto Edgejump & Edgestop](#auto-edgejump--edgestop)
   - [Fast Ladder](#fast-ladder)
   - [Accurate Slow-Walk](#accurate-slow-walk)
   - [Velocity Diagnostics & Peak Trackers](#velocity-diagnostics--peak-trackers)
4. [World & Atmosphere Modifiers](#4-world--atmosphere-modifiers)
   - [Weather Simulator](#weather-simulator)
   - [Volumetric Fog](#volumetric-fog)
   - [Skybox Modulator](#skybox-modulator)
   - [Lighting & Shadows](#lighting--shadows)
   - [Post-Processing & Atmosphere Presets](#post-processing--atmosphere-presets)
   - [Smoke Modulators](#smoke-modulators)
5. [Misc, HUD & Cosmetics](#5-misc-hud--cosmetics)
   - [Bullet Impacts, Tracers & Beams](#bullet-impacts-tracers--beams)
   - [Hit Markers, Damage Numbers & Hit Sounds](#hit-markers-damage-numbers--hit-sounds)
   - [Visual Removals](#visual-removals)
   - [Camera Modifiers & Thirdperson](#camera-modifiers--thirdperson)
   - [Viewmodel Offset Controls](#viewmodel-offset-controls)
   - [HUD Additions](#hud-additions)
   - [Spectator List & Info](#spectator-list--info)
   - [Scoreboard Weapon & Money Revealer](#scoreboard-weapon--money-revealer)
   - [Utilities & Gameplay Automation](#utilities--gameplay-automation)
6. [Skin Changer & Inventory System](#6-skin-changer--inventory-system)
   - [Weapon Skin Changer](#weapon-skin-changer)
   - [Sticker System (11,000+ VPK Catalog)](#sticker-system-11000-vpk-catalog)
   - [Keychain / Charm Selector](#keychain--charm-selector)
   - [Knife Changer](#knife-changer)
   - [Glove Changer](#glove-changer)
   - [Agent Changer](#agent-changer)
7. [User Interface & Scripting Engine](#7-user-interface--scripting-engine)
   - [Velocity Dark Glass Design System](#velocity-dark-glass-design-system)
   - [Interactive 3D Model Preview](#interactive-3d-model-preview)
   - [Lua Scripting Studio](#lua-scripting-studio)
   - [Config & Serialization System](#config--serialization-system)

---

## 1. Combat Engine

### Ragebot
- **Target Selection & Filtering**: Real-time evaluation of all valid enemy player pawns. Checks team affiliation, alive state, dormant status, and gun game immunity.
- **Weapon Group Configuration**: 6 weapon profiles (`pistol`, `smg`, `rifle`, `shotgun`, `sniper`, `utility`) each with custom FOV, hitchance, min damage, and hitbox configurations.
- **Dynamic Multipoint System**: Hitbox point generation using bone rotation quaternions and bounding boxes. Generates center and edge multipoints scaled dynamically by weapon inaccuracy.
- **Hitbox Selection**: Independent toggles for Head, Neck, Chest, Stomach, Pelvis, Arms, Legs, and Feet.
- **Hitchance Engine**: Full Monte Carlo spread ray simulation against target bone matrices. Evaluates weapon inaccuracy and spread circles with up to 256 ray samples.
- **Hitchance Override & Adaptive Hitchance**: Quick-bindable override value; adaptive mode dynamically lowers required hitchance when target velocity drops below 5 u/s.
- **Minimum Damage & Overrides**: Minimum required damage calculation with lethal detection (`std::fmaxf(1.0f, hp)`). Min damage override on hotkey.
- **Silent Aim**: Dispatches aim vectors strictly through `subtick_moves` and user command view angles without displacing client render camera.
- **No Spread Mode**: Client-side spread cancellation for unspread-capable servers.
- **Auto Scope**: Automatically scopes snipers (AWP, SSG 08, SCAR-20, G3SG1) upon acquiring a valid target or stop prediction.
- **Body Aim Conditions**: Force body aim toggle, body aim in air, and lethal body aim (preferring torso/stomach hitgroups when damage will kill).
- **Head Priority at Low HP**: Prioritizes head shots when enemy health is within specified threshold.
- **Force Shot**: Allows firing at maximum weapon accuracy on ground or in air even if target is moving.
- **Early Counter-Strafe Prediction**: Predicts player deceleration within the current subtick cycle to fire prematurely before movement completely halts.
- **Subtick Attack Registration**: Injects precise subtick button press events (`set_button(in_attack)`, `set_pressed(true)`, `set_when(0.0f)`) into `base_cmd->mutable_subtick_moves()`, guaranteeing 100% registration on semi-automatic and sniper rifles.
- **Doubletap (DT)**: Subtick command shift enabling instant two-shot bursts on rapid-fire weapons. Includes lethal mode toggle to preserve second shot if first shot kills.
- **Thread-Safe Candidate Scanning**: 100% crash-proof sequential scanning preventing CS2 internal `patterns::trace_bullet` engine concurrency memory corruption.

### Legitbot
- **Smooth & Humanized Aimbot**: Configurable smoothing (`1` to `100`), field of view (`0.1°` to `30°`), and nearest hitbox targeting.
- **Curve & Bezier Smoothing**: Non-linear bezier mouse trajectory paths emulating natural wrist and arm movement.
- **Recoil Control System (RCS)**: Independent pitch and yaw compensation percentage (`0%` to `100%`) with randomized min/max jitter ranges.
- **Standalone RCS**: Automatically controls weapon recoil when manually tracking targets without aim assistance.
- **Human Reaction Delay**: Configurable millisecond reaction delay (`0` to `500 ms`) before aimbot acquires target.
- **Target Switch Dwell Delay**: Prevents robotic target snapping by enforcing a linger time on the previous target before re-engaging.
- **Dynamic FOV**: Automatically scales targeting field of view inversely with target distance.
- **Safety Checks**: Flashbang blinded check, smoke line-of-sight obstruction check, jump check, and sniper scope requirement check.
- **Target Indicator**: Clean visual indicator over current legitbot focus target with custom color.
- **Auto-Pistol & Kill Delay**: Automatically fires semi-automatic pistols on trigger hold; pauses firing for configurable milliseconds after target death.

### Triggerbot
- **Instant Response & Reaction Delay**: Millisecond delay slider (`0` to `200 ms`) before dispatching shot upon target entering crosshair.
- **Hit Chance Verification**: Validates whether the bullet will hit the target with current spread before firing.
- **Head-Only Mode**: Ignores limbs and torso, firing strictly when crosshair enters head hitbox.
- **Seed Verification Mode**: Inspects internal weapon spread seed to guarantee bullet impact placement.
- **Gaussian Trigger Jitter**: Adds randomized micro-delays mimicking human nervous system reaction variations.

### Lag Compensation & Backtracking
- **Simulation History Rollback**: Maintains a 32-record circular history per player pawn containing origin, rotation, eye angles, velocity, and complete 128-bone matrices.
- **Tickbase Verification**: Compares target simulation time against server tick intervals with clamp boundaries (`sv_maxunlag`).
- **Bone Matrix Restoration**: Backups and restores bone transforms during hitbox evaluation to eliminate memory contamination.
- **Visual Record History**: Provides visual bone record snapshots for skeleton and backtrack chams visualization.

### Extrapolation
- **Movement Extrapolation**: Physics-based linear and trajectory movement prediction for players breaking lag compensation.
- **Collision & Gravity Integration**: Integrates `sv_gravity` and world trace KD-tree collision bounds to extrapolate falling and jumping targets accurately.
- **Pointer-Stable Record Cache**: Backed by `std::deque` to guarantee zero pointer invalidation during multi-player scan iterations.

### Desync Resolver
- **Desync Detection**: Analyzes network velocity, animation layers, and eye yaw delta.
- **Head-Chain Angle Rotation**: Rotates the upper bone hierarchy (head, neck, spine 1–4) across predicted desync angles to expose real head hitboxes.

### Penetration & Autowall Engine
- **Source 2 Bullet Trace Emulation**: Simulates real bullet ballistic paths through dynamic game world geometry using `patterns::trace_bullet_data_init` and `patterns::trace_bullet`.
- **Material Scale Calculation**: Calculates energy loss, penetration modifiers, surface thickness, and damage falloff across concrete, wood, metal, glass, plastic, and cardboard.
- **Armor & Hitgroup Scaling**: Computes weapon-specific armor penetration ratios, helmet protection, and hitgroup multipliers (head 4x, stomach 1.25x, legs 0.75x).

### Zeusbot (Taserbot)
- **Automatic Taser**: Scans hitbox bounding boxes within taser range (180 units) with field of view limits.
- **Auto Drop**: Automatically issues `drop` console command to discard empty Zeus x27 in competitive matches.

### Knifebot
- **Slash vs. Stab Logic**: Calculates frontal knife slash (48 units) and backstab (32 units) reach distance.
- **Orientation Analysis**: Dot product validation of attacker-to-victim direction vectors against victim forward eye vectors (`> 0.475f`) to guarantee 1-hit backstabs.

### Anti-Aim & Desync
- **Pitch Control**: `None`, `Down` (89°), `Up` (-89°), `Jitter`.
- **Yaw Modes**: `None`, `Backward` (180°), `Rotate`, `Jitter`, `Spin`.
- **Custom Parameters**: Yaw offset, jitter range, spin velocity speed.
- **Manual Direction Overrides**: Dedicated keybinds for `Manual Left`, `Manual Right`, and `Manual Back` with mutually exclusive state clearing.
- **Hide Shots (Shot Masking)**: Flickers view angles away on the exact firing tick to conceal shot angles from observers.

### Fake Lag
- **Choke Modes**: `Always`, `Moving`, `In-Air`.
- **Choke Limit**: Configurable choked command packets (`1` to `14` ticks).
- **Adaptive Mode**: Dynamically scales choked packets between min and max bounds based on player velocity.
- **Lag on Peek**: Maximizes fake lag when crossing corner angles.
- **Cancel on Shot**: Instantly flushes queued commands when attacking.

### Peek Assistance
- **Quick Peek (Auto Peek)**: Places a 3D ground ring marker upon activation; automatically counter-strafes back to the origin immediately after firing.
- **Duck Peek**: Automatically ducks behind cover and un-crouches for lethal shots.
- **Fake Duck**: Exploits subtick duck transitions to maintain crouch on the server while client perspective remains elevated.

---

## 2. Visuals & ESP Engine

### Player ESP (Overlay)
- **Bounding Box**: Full 2D box or cornered bracket box with configurable corner lengths, fill color, and border outline.
- **Skeleton**: Bone connection lines for all joints (head, neck, spine, shoulders, elbows, hands, hips, knees, feet).
- **Backtrack Skeleton**: Displays historical bone positions showing the exact backtrack window.
- **Dynamic Health Bar**: Left, top, or bottom positioning; smooth color gradient (full HP to low HP); numeric health text; glow effect; battery-segmented style; dynamic damage drop animation.
- **Ammo Bar**: Current magazine count bar with gradient, outline, numeric ammo counter, and glow.
- **Player Name**: Sanitized gamer tag text with custom color and font shadow.
- **Weapon Display**: Three modes: `Text Only`, `Icon Only`, and `Text + Icon` using high-resolution game font glyphs.
- **Player Information Flags**:
  - `Money`: Target economy (`$16,000`).
  - `Armor`: Kevlar (`K`) or Helmet (`HK`).
  - `Kit`: Defuse kit possession.
  - `Scoped`: Zoomed state alert.
  - `Defusing`: Bomb defuse action indicator with remaining seconds.
  - `Flashed`: Blinded state alert.
  - `Ping`: Latency indicator.
  - `Distance`: Meter/foot distance from local player.
- **Offscreen Indicator (Out of FOV Arrows)**: Triangular arrows positioned on screen perimeter pointing toward offscreen enemies with distance fading.

### Chams System
- **15 Shading Materials**:
  1. `Liquid`
  2. `Metallic`
  3. `Matte`
  4. `Flat (Unlit)`
  5. `Bloom`
  6. `Outlines (Rim Lighting)`
  7. `Glow (Pulsating Bloom)`
  8. `Electric (Arc Discharge)`
  9. `Distortion`
  10. `Hologram`
  11. `Pearl`
  12. `Crystal`
  13. `Velvet`
  14. `Plasma`
  15. `Glass`
- **Occlusion Layers**: Independent configuration for `Visible (Primary)`, `Invisible / Occluded (Secondary / Ignore-Z)`, and `Overlay (Fresnel)` layers.
- **Backtrack Ghost Chams**: Renders historical record poses behind moving targets.
- **Local Player & Viewmodel Chams**: Independent chams for local player pawn, viewmodel weapon, and hands/arms.
- **Local Scoped Opacity Reduction**: Dims or removes local player model while scoped in thirdperson.

### Player Glow
- **Outline Glow**: Outer glow mask with custom color, alpha, and bloom intensity.

### Item ESP (Dropped Weapons & Gear)
- **Classification**: Automatic recognition of dropped pistols, SMGs, rifles, shotguns, snipers, and utilities.
- **Display Modes**: Text, SVG weapon icons, and dual mode.
- **Ammo & Distance Filtering**: Renders remaining clip size and filters items past specified distance cutoff.
- **Dropped Item Chams & Glow**: Custom materials and outlines for weapons and equipment lying on the map.

### Projectile ESP & Trajectories
- **Grenade Overlays**: Real-time tracking of HE Grenades, Flashbangs, Smokes, Molotovs, Incendiaries, and Decoys.
- **Smoke Expiration Timer**: Circular countdown timer with ground ring indicating smoke radius and dissipation warning.
- **Molotov / Inferno Boundary**: Renders precise ground fire propagation polygon with fill, outline, and glow.
- **Grenade Proximity Warning**: On-screen directional arc warning indicating lethal blast proximity.
- **Grenade Flight Trajectory**: Flight path line with bounce normals, collision points, and predicted detonation radius.

### Objective ESP (Bomb & Defuse)
- **Planted C4 Overlay**: Displays planted bomb site (`A` or `B`), countdown timer bar, and explosion damage estimate.
- **Defuse Timer**: Real-time defuse progress bar comparing remaining defusal time against bomb detonation timer.

### Radar & Radar Stream
- **Engine Radar Reveal**: Sets internal `m_bSpotted` state, rendering all enemies on the native in-game minimap.
- **Custom 2D Overlay Radar**: Draggable dark glass minimap displaying player blips, view direction cones, and bomb location.
- **Web Radar Stream**: Local HTTP server broadcasting live match coordinates for second-screen or mobile viewing.

---

## 3. Movement Engine

### Subtick Bunnyhop
- **Auto-Jump Timing**: Issues jump command on the exact frame the player touches the ground.
- **Hitchance Slider**: Adds humanization by allowing hop failure rates (`0%` to `100%`).
- **Consecutive Hop Limiter**: Caps maximum consecutive automated bunnyhops.

### Auto Air-Strafer
- **Directional Modes**: Fully directional WASD air-strafing optimizing air acceleration.
- **Turn Rate Limiting**: Clamps maximum angle delta per tick to prevent robotic snap turns.
- **Humanized Wobble**: Adds subtle sinusoidal mouse movement variations.

### Subtick Jumpbug
- **Fall Damage Nullification**: Executes an unduck-to-jump transition on the tick before impact, resetting vertical velocity without damage.

### Subtick Edgebug
- **Edge Slide**: Detects brush boundary edges and slides off lips, preventing all fall damage and preserving horizontal velocity.
- **Configurable Modes & Passes**: Mode selector (`0` to `4`) and extra subtick duck cycles.

### Auto Edgejump & Edgestop
- **Edgejump**: Automatically jumps on the final ground tick when running off ledges.
- **Edgestop**: Blocks user movement when approaching high or lethal drops.

### Fast Ladder
- **Ladder Ascent & Descent**: Optimizes yaw angle relative to ladder surface normals to maximize climbing velocity.

### Accurate Slow-Walk
- **Silent Movement Clamping**: Restricts player movement velocity to `33 u/s`, maintaining full weapon accuracy and silent footsteps.

### Velocity Diagnostics & Peak Trackers
- **Real-Time Readout**: Displays horizontal velocity in units per second (`u/s`).
- **Peak Indicator**: Records and displays maximum speed achieved during jumps and landings.

---

## 4. World & Atmosphere Modifiers

### Weather Simulator
- **Physical Particles**: Snow, Rain, Stars, and Embers.
- **Wind & Turbulence**: Wind strength, direction angles, and chaotic air turbulence simulation.
- **Surface Wetness**: Simulates surface reflection and dampness across world textures.

### Volumetric Fog
- **Atmospheric Depth**: Custom fog color, density slider, anisotropy scattering, and draw distance cutoff (`up to 12,000 units`).

### Skybox Modulator
- **Custom Skyboxes**: In-engine skybox replacement with ambient color tinting, cloud tint, and sun glow adjustment.

### Lighting & Shadows
- **Custom Sun Lighting**: Directional vector rotation, light intensity, and custom RGB sunlight illumination.
- **Ambient Lighting**: Modulates ambient environmental light level and shadow darkness.
- **Bloom & Gamma**: Post-processing bloom intensity and gamma level controls.
- **Depth of Field (DoF)**: Near blurry, near crisp, far crisp, and far blurry focal ranges.

### Post-Processing & Atmosphere Presets
- **Night Mode**: Darkens map geometry and ambient light for night-time aesthetics.
- **Candlelight Mood**: Warm amber lighting preset.
- **Cyberpunk Neon**: Dual-tone cyan and magenta vignette lighting, dynamic scanlines, and high-contrast glow.
- **Chromatic Aberration**: Fullscreen RGB channel displacement warping peripheral vision.

### Smoke Modulators
- **Smoke Removal**: Completely removes volumetric smoke particle clouds.
- **Wireframe & Color Modulation**: Renders smoke volume as wireframe meshes or custom tinted colors.

---

## 5. Misc, HUD & Cosmetics

### Bullet Impacts, Tracers & Beams
- **Dual Bullet Impacts**: Client predicted impacts (blue) vs. server confirmed impacts (red) with duration control.
- **Bullet Tracers & Beams**: High-energy laser beams with customizable color, width, lifetime, and noise styles.

### Hit Markers, Damage Numbers & Hit Sounds
- **Hit Markers**: Classic cross, damage numeric popup, or hybrid display with glow.
- **Hit Sparks**: 4 particle archetypes: `Cross Stars`, `Embers`, `Diamonds`, and `Neon Runes`.
- **Heart Kill Particles**: Bursts floating heart motes upon eliminating enemies.
- **Floating Damage Numbers**: Damage text rising from hit position with headshot gold tint.
- **Audio Feedback**: Custom WAV sound playback on hit and kill (`Killcard`, `Bell`, `Coin Pickup`, `Shop Click`, `Bullet Casing`, `Popcan`).

### Visual Removals
- **No Scope**: Removes sniper black scope overlay, rendering full-screen crosshair lines.
- **No Flash**: Adjusts flashbang maximum whiteout alpha (`0%` to `100%`).
- **No Smoke**: Disables smoke grenade particle rendering.
- **No Visual Recoil / Punch**: Removes camera shake and recoil kick.
- **No Decals**: Purges blood splatters and bullet holes.
- **No Legs & Overhead**: Removes first-person leg models and overhead UI indicators.
- **No 3D Skybox & Skybox Fog**: Removes background 3D skybox geometry.

### Camera Modifiers & Thirdperson
- **Field of View (FOV)**: Custom camera FOV (`60°` to `140°`).
- **Scoped FOV Override**: Custom field of view while scoped.
- **Thirdperson Mode**: Toggle bind, camera distance slider (`30` to `250 units`), and collision hull bounds check preventing clipping through walls.
- **Aspect Ratio Modifier**: Custom screen aspect ratio (e.g. 4:3 stretched, 16:10, 21:9).

### Viewmodel Offset Controls
- **XYZ Coordinates**: Offsets weapon position horizontally (`X`), forward/backward (`Y`), and vertically (`Z`).
- **Viewmodel FOV**: Independent weapon field of view slider (`54°` to `90°`).

### HUD Additions
- **Jump Rings**: 3D animated shockwave rings spawning at feet upon jumping or landing:
  - Styles: `Ring`, `Disc`, `Ripples`, `Hexagon`.
  - Particle Physics: `Sparks`, `Embers`, `Smoke`, `Neon Runes`, `None`.
- **Custom Crosshair Overlay**: Dot, Cross, Circle, T-Style, Cross-Dot with dynamic spread and hit pulse.
- **Custom Sniper Scope Lines**: Clean crosshair lines with animated fade-in and glow.
- **3D Cosmetic Hats**: `Cone`, `Halo`, `Double Rim`, `Hex Crown` with `Gradient`, `Wireframe`, and `Neon Rim` shading.
- **Motion Trails**: Trailing ribbons, neon lines, or beads attached to feet, waist, or weapon.
- **Velocity Counter & Graph**: Dark glass card with bold digital speed readout and real-time velocity graph.
- **Combat Badges**: On-screen status badges indicating active combat states: `DMG`, `HC`, `BAIM`, `FD`, `PEEK`.
- **Hit/Kill Reminders (Event Logs)**: Rounded dark glass notification cards with skull/crosshair icons, pill badges, and damage values.

### Spectator List & Info
- **Observer Detection**: Displays avatar, name, and spectated target of players watching the local player.

### Scoreboard Weapon & Money Revealer
- **Economy Tracker**: Displays real-time cash balance and equipped primary weapon for all enemies on the scoreboard.

### Utilities & Gameplay Automation
- **Auto Accept**: Automatically clicks the "Accept" button when a matchmaking match is found.
- **Auto Buy**: Automatically purchases primary weapon, secondary weapon, armor, defuser, taser, and grenades at round start.
- **Clantag Changer**: Animated custom clantag marquee synchronized with tickbase.
- **Preserve Killfeed**: Keeps kill notices visible indefinitely.
- **Streamproof Mode**: Hides menu and visual overlays from recording software (OBS, Discord, ShadowPlay).
- **Discord Rich Presence**: Native Discord IPC integration for FemWare displaying live game status (`"Playing on de_dust2"` or `"Main Menu"`), match timers, and `"Femboying with Femware"` activity preset, featuring the custom glowing animated `"FW"` asset (enabled by default).

---

## 6. Skin Changer & Inventory System

### Weapon Skin Changer
- **Custom PaintKit ID**: Assigns any weapon finish index.
- **Float Wear Value**: Precise wear control (`0.000` to `1.000`).
- **Pattern Seed**: Custom pattern template seed (`0` to `1000`).
- **StatTrak Counter**: Activates StatTrak counter with kill tracking.

### Sticker System (11,000+ VPK Catalog)
- **11,000+ Official Stickers**: Extracted directly from CS2's `pak01_dir.vpk` (`scripts/items/items_game.txt`).
- **Real In-Game Names**: Real-time localization resolution via CS2 engine (`addresses::globals::localize`), resolving names like `"Titan (Holo) | Katowice 2014"`, `"Crown (Foil)"`, `"Howling Dawn"`.
- **Multi-Token Name Search**: Search by name tokens (e.g. `"kato 14"`, `"titan holo"`, `"cloud9 foil"`).
- **5 Independent Sticker Slots**: Dedicated slot selectors (`[S1]` to `[S5]`) with per-slot wear, scale, and rotation.
- **Rarity Highlights**: Visual rarity color strip reflecting official item tier.

### Keychain / Charm Selector
- **In-Game Charm Names**: Searchable charm catalog with localized names.
- **Custom Charm Seed**: Controls charm pattern and alignment.

### Knife Changer
- **18 Knife Models**: Karambit, Butterfly, M9 Bayonet, Talon, Skeleton, Flip, Gut, Bayonet, Huntsman, Bowie, Falchion, Shadow Daggers, Ursus, Stiletto, Nomad, Survival, Paracord, Classic.
- **Finish Application**: Applies any weapon or knife skin to replaced knife model.

### Glove Changer
- **8 Glove Types**: Sport, Specialist, Moto, Hand Wraps, Bloodhound, Hydra, Broken Fang, Driver Gloves.
- **Full Wear & Finish Support**: Custom glove textures and wear values.

### Agent Changer
- **Custom Player Models**: Overrides default CT and T agents with official agent skins including custom voice lines.

---

## 7. User Interface & Scripting Engine

### Velocity Dark Glass Design System
- **Acrylic Backdrop Blur**: Real-time background blur under all UI panels.
- **Smooth Window Dragging**: Seamless window repositioning with focus resync (`WM_ACTIVATE` / `WM_SETFOCUS`) eliminating mouse snap bugs.
- **Multi-Category Navigation**: Organized sidebar tabs: `Ragebot`, `Legitbot`, `Visuals`, `Movement`, `Changer`, `Misc`, `Config`, `Lua Studio`.

### Interactive 3D Model Preview
- **Live 3D Rendering**: Interactive CT (SEAL Team 6) and T (Phoenix Connexion) models in the Visuals tab.
- **Instant Preview Reflection**: Reflects box ESP, skeletons, chams materials, and health bars directly on the 3D model.

### Lua Scripting Studio
- **Integrated IDE**: Code editor with syntax highlighting and font scaling.
- **Clean SVG Icon System**: Document and folder vector icons for script browser.
- **Lua Console**: Tagged, color-coded log chips with timestamps, auto-scroll to newest entries, and copy-to-clipboard.
- **In-Game Documentation**: Browsable API reference with syntax-highlighted snippets and one-click Copy / Insert-to-Studio.
- **Sandboxed Runtime**: Stdlib stripped of `os`, `io`, `debug`, `package`, and bytecode-loading surfaces; scripts cannot touch disk or the host process.
- **Instruction Watchdog**: Every top-level run and render callback runs under an instruction budget — runaway loops abort with an error instead of freezing the frame.
- **Error Handling**: A callback that errors 8 consecutive times is auto-unregistered and logged; GC is incrementally tuned with a bounded per-frame step.
- **Lua API**: `render` (text, shapes, gradients, polylines, triangles, outlined/shadowed text, world-to-screen), `client` (local, eye pos, players, weapon, cursor, input), `engine` (in-game state, map, ping, fps, time), `convar` (read-only typed cvar access), and `events` (render callbacks).
- **Math Extensions**: `clamp`, `lerp`, `saturate`, `pingpong`, `normalize_yaw`, `angle_diff`, `distance`, `vector_to_angle`, `randomf` on top of the standard math library.
- **Full Reference**: See [LUA_API.md](LUA_API.md) for the complete binding table.

### Config & Serialization System
- **JSON Configuration**: Complete state serialization into human-readable JSON.
- **Clean Export**: Zero placeholder configs or corrupted presets.
- **Instant Import/Export**: Quick clipboard and disk profile loading.

---
*Generated for Velocity CS2 Client.*
