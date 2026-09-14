# Land of Dran Server Lua API

This documents every function the server-side Lua environment exposes to scripts
(`serverstart.lua` and anything `dofile`'d from it, or run through the eval console).
Generated from the current `LuaFunctions/*.cpp` and `Networking/PacketsFromClient/*.cpp`
source - if you add or change a binding, update this file too.

## Conventions

- Every `Dynamic`, `StaticObject`, client, brick, light, and emitter table has an `id` field (its net ID) and a
  `type` field you can compare against: `1` = Dynamic, `2` = Static, `3` = Client, `4` = Brick,
  `5` = Light, `6` = Emitter (`NetTypes/NetType.h`'s `SimObjectType`). `raycast()` and `client:getCursorItem()` can
  return a Dynamic, a Static, or a Brick, so check `.type` before assuming which.
- Functions documented as `Expected N arguments` in an error message are strict about
  argument count - passing the wrong number logs an error and does nothing (they don't
  throw a Lua error, so a mistake here fails silently unless you're watching the log).
- Color arguments (`r,g,b[,a]`) are floats in the 0-1 range.
- Mouse button masks (used by `ClientClick`, see below) follow SDL's convention:
  `1` = left, `2` = middle, `4` = right. Check with `(mask & 1) ~= 0`, etc. - more than
  one bit can be set if multiple buttons are held.

---

## Logging / misc

| Function | Arguments | Description |
|---|---|---|
| `info(...)` | any number of values | Logs a line to the server's info log. Values are stringified (tables holding a Dynamic/Static/Client/Brick/Light/Emitter print as `[Dynamic N]`/`[Static N]`/`[Client N]`/`[Brick N]`/`[Light N]`/`[Emitter N]`). |
| `error(...)` | any number of values | Same as `info`, but logged as an error and prefixed accordingly. |
| `debug(...)` | any number of values | Same as `info`, but only logged when the `logger/verbose` setting is on. |
| `shutdown()` | none | Stops the main program loop (shuts the whole process down, not just the server). |

## Time of day, sky, and water

The server owns the time of day, the look of the day/night cycle, and the water level. It sends
them to every client once a second, and right away when one of these functions changes them or a
client finishes joining.

| Function | Arguments | Returns | Description |
|---|---|---|---|
| `setTimeOfDay(fraction)` | `fraction`: `0` = midnight, `0.25` = sunrise, `0.5` = noon, `0.75` = sunset. Values outside 0-1 wrap around. | none | Jumps to that time of day. The server starts at noon. |
| `getTimeOfDay()` | none | number, 0-1 | Current time of day, on the same scale as `setTimeOfDay`. |
| `setTimeScale(scale)` | `scale`: in-game seconds that pass per real second | none | A full day is 1000 in-game seconds (`DAY_LENGTH_SECONDS` in `LandOfDran.h`), so the default of `1` is a ~16.7 minute day. `0` freezes time, negative values run it backwards. |
| `getTimeScale()` | none | number | Current time scale. |
| `setWaterLevel([y])` | `y`: world height of the water surface, or no argument / `nil` | none | Puts a water surface at height `y` across the whole world, or removes it when called with no argument. Off by default. Dynamics in the water float or sink depending on their buoyancy (see `dynamic:setBuoyancy`) and are slowed by drag. Players at least half under water swim: W and S follow the camera up and down, A and D stay level, holding jump swims up, and pressing jump with their head above the surface jumps out. The server plays the `Splash` sound where a dynamic falls in fast and `ExitWater` where one comes out fast, if they're registered. Clients draw ripples spreading across the surface where dynamics go in, come out, or move along it. |
| `getWaterLevel()` | none | number, or `nil` if there's no water | Current water height. |

All of these except the getters use the strict `Expected 1 number argument` check described
above (`setWaterLevel` also accepts no arguments).

The day/night cycle blends between four phases: `"night"`, `"dawn"`, `"day"`, and `"dusk"` (phase
names ignore case). Each has its own sky, fog, and sun color, and the colors you see at any moment
are a mix of the phases on either side of the current time. Everything is lit by the ambient color,
light from the sky itself, and the sun adds its light on top wherever it reaches, so shadows are
the ambient color with the sun taken away. At night the "sun" is the moon, shining from the opposite
side of the sky.

| Function | Arguments | Returns | Description |
|---|---|---|---|
| `setSkyColor(phase, r, g, b)` | phase name, color | none | Color of the sky straight up during that phase. It fades into the fog color toward the horizon. |
| `getSkyColor(phase)` | phase name | r, g, b | That phase's sky color. |
| `setFogColor(phase, r, g, b)` | phase name, color | none | Color of the fog, and of the sky near the horizon, during that phase. |
| `getFogColor(phase)` | phase name | r, g, b | That phase's fog color. |
| `setSunColor(phase, r, g, b[, brightness])` | phase name, color, optional brightness | none | Color of the sunlight (moonlight for `"night"`) during that phase. Sunlight is much brighter than a screen color, so the color is multiplied by `brightness`; leave it out to keep the phase's current brightness. Defaults: `day` is `1, 0.7, 0.5` at brightness `15`, `dawn` and `dusk` are about `6`, `night` is `0.5, 0.6, 1` at `0.35`. |
| `getSunColor(phase)` | phase name | r, g, b, brightness | That phase's sunlight, split into a color whose brightest channel is 1 and its brightness. |
| `setAmbientColor(phase, r, g, b)` | phase name, color | none | Light from the sky that reaches everything, shadows included, during that phase. It's dim next to sunlight, so small values go a long way. Defaults: `day` is about `0.45, 0.32, 0.22`, `dawn` and `dusk` about `0.25, 0.2, 0.2`, `night` about `0.015, 0.018, 0.03`. Values above 1 are allowed. |
| `getAmbientColor(phase)` | phase name | r, g, b | That phase's ambient color. |
| `setFogDistance(start, end)` | distances from the camera in world units | none | Fog begins at `start` and completely hides everything past `end`. Needs `0 <= start < end <= 900`. Defaults to `150, 290`. Grass and water always reach past `end`, and shadows cover the view out to `end`, so a longer fog distance spreads the same shadow detail over more ground. |
| `getFogDistance()` | none | start, end | Current fog distances. |
| `resetDayCycle()` | none | none | Puts every phase's sky, fog, sun, and ambient color and the fog distances back to their defaults. Doesn't change the time of day or time scale. |

The setters log an error and do nothing if the phase name is unknown or the arguments are the wrong
count or type. Negative colors and brightness are treated as 0.

## Scheduling

| Function | Arguments | Returns | Description |
|---|---|---|---|
| `schedule(delayMS, functionName, ...)` | `delayMS`: milliseconds from now to run; `functionName`: **string** name of a global function; any further arguments are passed through to that function when it runs | schedule ID (number) | Calls the named global function once, after `delayMS` milliseconds. Extra arguments after `functionName` are forwarded to it. Runs are one-shot - call `schedule` again inside the callback for a repeating timer (see `PickupSystem.lua`'s bob loop). |
| `cancel(scheduleID)` | `scheduleID`: value returned by `schedule` | none | Cancels a pending scheduled call before it fires. No-op if it already fired or was already cancelled. |

## Events

| Function | Arguments | Returns | Description |
|---|---|---|---|
| `registerEventListener(eventName, functionName)` | both strings | none | Binds a global function as a listener for a built-in event. Multiple listeners can be bound to the same event; they run in registration order, each receiving whatever the previous one returned (see below), so every listener for a given event must accept and return the same argument list. |
| `unregisterEventListener(eventName, functionName)` | both strings | none | Removes a previously-registered listener. |
| `getNumListeners(eventName)` | string | count | How many functions are bound to an event. |
| `getListenerIdx(eventName, index)` | string, 0-based index | function name (string) | Name of the listener at that index. |

### Built-in events

| Event | Listener signature | Notes |
|---|---|---|
| `ClientJoin` | `function(client) ... return client end` | Fires once a client finishes phase-1 loading (right after connecting). `serverstart.lua`'s `join()` creates and gives them their player dynamic here. |
| `ClientLeave` | `function(client) ... return client end` | Fires when a client disconnects, before it's removed from the client list. Use this to clean up anything the client owned (see `PickupSystem.lua`'s `dropHeldOnLeave`). |
| `ClientChat` | `function(client, message) ... return client, message end` | Fires when a client sends a chat message, before it's broadcast. Return a modified `message` to alter it, or an empty string to suppress it. |
| `ClientPlantBrick` | `function(client, brick) ... return client, brick end` | Fires after a client plants its ghost brick and the server accepts it. The brick is already placed and sent to clients; call `brick:remove()` to take it back out. |
| `ClientAdminLogin` | `function(client) ... return client end` | Fires when a client enters the right eval console password. Not fired for the single player host, who is made admin automatically. `serverstart.lua` plays the `Admin` sound to them here. |
| `ClientClick` | `function(client, posX, posY, posZ, dirX, dirY, dirZ, mask) ... return client, posX, posY, posZ, dirX, dirY, dirZ, mask end` | Fires on every mouse click. `posX/Y/Z` and `dirX/Y/Z` are the camera's position and look direction *at the moment of the click*; `mask` is the SDL mouse button mask (see Conventions). |
| `ClientStartTalking` | `function(client) ... return client end` | Fires when a client starts sending voice chat. Calling `client:setVoiceMuted(true)` here cuts them off before anyone hears them. |
| `ClientStopTalking` | `function(client) ... return client end` | Fires when a client lets go of push to talk, or half a second after their voice stops arriving (a lost last packet, or muted while talking). Not fired for a client who leaves while talking. |

---

## Dynamics

Dynamics are physics-simulated objects (players, projectiles, pickups, etc).

### Global functions

| Function | Arguments | Returns | Description |
|---|---|---|---|
| `createDynamic(typeID, x, y, z)` | `typeID` from `newDynamicType`/`getDynamicType`; spawn position | Dynamic | Spawns a new dynamic of the given type at the given position. |
| `getDynamicId(netId)` | net ID | Dynamic | Looks up a dynamic by its net ID. Errors if it doesn't exist. |
| `getDynamicIdx(index)` | 0-based index | Dynamic | Looks up a dynamic by its position in the internal list (see `getNumDynamics`). |
| `getNumDynamics()` | none | count | How many dynamics currently exist. |
| `newDynamicType(scriptName, modelFilePath, scaleX, scaleY, scaleZ)` | `scriptName`: unique name used to refer to this type later; `modelFilePath`: path to the model file; scale on each axis | typeID | Registers a new kind of dynamic (model + scale). Call once at startup per type. |
| `getDynamicType(scriptName)` | string | typeID | Looks up a previously-registered type's ID by its script name. |
| `addAnimation(typeID, animationName, startFrame, endFrame, speed, fadeInMS, fadeOutMS)` | type to attach the animation to; frame range; playback speed; fade in/out durations in ms | none | Adds a named animation clip to a dynamic type. The first animation added to a type is used as its walk cycle. |
| `raycast(startX, startY, startZ, endX, endY, endZ[, dynamicToIgnore])` | ray start/end points; optionally a Dynamic to exclude from the hit test | hit object, x, y, z, normalX, normalY, normalZ, distance; or `nil` | Casts a ray through the physics world. Returns the Dynamic, Static, or Brick it hit first, then the world position of the hit, the normal of the surface it hit (pointing out of it), and the distance from the start point. Returns just `nil` if it hit nothing. `local hit = raycast(...)` still works if you only need the object. |

### `dynamic:` methods

| Method | Arguments | Returns | Description |
|---|---|---|---|
| `dynamic:destroy()` | none | none | Removes the dynamic from the world and un-controls it for any client controlling it. |
| `dynamic:getPosition()` | none | x, y, z | Current world position. |
| `dynamic:setPosition(x, y, z)` | position | none | Teleports the dynamic. |
| `dynamic:getVelocity()` | none | x, y, z | Current linear velocity. |
| `dynamic:setVelocity(x, y, z)` | velocity | none | Sets linear velocity directly. |
| `dynamic:getAngularVelocity()` | none | x, y, z | Current angular velocity. |
| `dynamic:setAngularVelocity(x, y, z)` | angular velocity | none | Sets angular velocity directly. |
| `dynamic:setAngularFactor(x, y, z)` | per-axis multiplier (0 = locked) | none | Restricts which axes the physics engine is allowed to rotate the object around, e.g. `(0,0,0)` to stop it tipping over. |
| `dynamic:activate()` | none | none | Wakes the physics body up if it was asleep. |
| `dynamic:isActive()` | none | bool | Whether the physics body is currently active (not asleep). |
| `dynamic:getGravity()` | none | x, y, z | Current per-object gravity vector. |
| `dynamic:setGravity(x, y, z)` | gravity vector | none | Overrides gravity for just this object. |
| `dynamic:getFriction()` | none | value | Current friction coefficient. |
| `dynamic:setFriction(friction)` | 0-10, clamped | none | Sets friction. |
| `dynamic:getRestitution()` | none | value | Current restitution (bounciness). |
| `dynamic:setRestitution(restitution)` | 0-10, clamped | none | Sets restitution. |
| `dynamic:getRotation()` | none | w, x, y, z | Current orientation as a quaternion. |
| `dynamic:setRotation(w, x, y, z)` | full quaternion | none | Sets orientation from an explicit quaternion. |
| `dynamic:setRotation(yaw, pitch, roll)` | 3 args instead of 4 | none | Alternate overload: sets orientation from Euler angles instead of a quaternion. |
| `dynamic:getMass()` | none | value | Current mass. |
| `dynamic:setMassProps(mass, centerX, centerY, centerZ)` | mass and local center of mass | none | Sets mass and center of mass together. |
| `dynamic:setMeshColor(meshName, r, g, b, a)` | mesh name within the model, color | none | Recolors one mesh of the model and broadcasts the change to clients. |
| `dynamic:setMeshDecal(meshName, faceName)` | mesh name within the model; file name of an image in `Assets/faces` (e.g. `"smiley.png"`, up to 64 characters), or `""` to remove it | none | Shows a face on one mesh, drawn over its color using the mesh's texture coordinates, and broadcasts the change. A model's face plate (a mesh named `Face1`, or `Face` without one) is see-through except for the face, so without a face it isn't drawn at all, and it casts no shadow or outline. Clients look the face up in their own `Assets/faces` folder, so a face they don't have isn't shown. |
| `dynamic:setHighlight(r, g, b, a, thickness)` | color; `thickness` is how far (in world units) the outline extends past the model's surface | none | Applies an outline/highlight effect around the whole object and broadcasts it to clients. |
| `dynamic:clearHighlight()` | none | none | Removes the outline/highlight effect. |
| `dynamic:getNumControllers()` | none | count | How many clients currently control this dynamic (usually 0 or 1; 0 means it's a normal server-simulated object, not a player). |
| `dynamic:getControllerIdx(index)` | 0-based index | Client | The client controlling this dynamic at that index. |
| `dynamic:snapToCursor(client, xOffset, yOffset, zOffset)` | client to attach to; view-space offset: `x` = right, `y` = up, `z` = distance in front of the camera | none | Attaches the dynamic to a client's cursor: every physics tick its position is recomputed from that client's live camera position/direction plus this offset, and its gravity is disabled. Calling this again while already snapped just updates the client/offset. |
| `dynamic:unsnap()` | none | none | Detaches from the cursor (if snapped) and restores the gravity it had before snapping. No-op if not snapped. |
| `dynamic:isSnapped()` | none | bool | Whether the dynamic is currently snapped to any client's cursor. |
| `dynamic:getSnapClient()` | none | Client or `nil` | The client it's snapped to, or `nil` if not snapped. |
| `dynamic:playSound(name[, pitch, volume])` | sound type name; see [Sounds](#sounds) | none | Plays a sound once for everyone, following the dynamic as it moves. |
| `dynamic:startSoundLoop(name[, pitch, volume])` | sound type name; see [Sounds](#sounds) | loop ID | Starts a looping sound that follows the dynamic. It stops by itself when the dynamic is destroyed. |
| `dynamic:setBuoyancy(buoyancy)` | 0-10, clamped; default 1.3 | none | How hard water pushes the dynamic up, as a multiple of its weight when it's fully under. `0` sinks (slowed by drag), `1` hangs wherever it is, higher values float with less of it under. Sent to clients too, since they simulate the dynamics they control (players) in water themselves. A swimming player holds their depth while moving, so buoyancy only decides whether they sink or float up while they aren't swimming. |
| `dynamic:getBuoyancy()` | none | number | Current buoyancy. |

---

## Statics

Statics are non-moving objects that still have a mesh and physics presence (walls, floor tiles, buttons, etc). They reuse the same "dynamic type" definitions (model + scale) as dynamics.

### Global functions

| Function | Arguments | Returns | Description |
|---|---|---|---|
| `createStatic(typeID, x, y, z)` | type ID (from `newDynamicType`/`getDynamicType`); position | Static | Spawns a static object of the given type. |
| `getStaticId(netId)` | net ID | Static | Looks up a static by its net ID. |
| `getStaticIdx(index)` | 0-based index | Static | Looks up a static by its position in the internal list. |
| `getNumStatics()` | none | count | How many statics currently exist. |

### `static:` methods

| Method | Arguments | Returns | Description |
|---|---|---|---|
| `static:destroy()` | none | none | Removes the static from the world. |
| `static:getPosition()` | none | x, y, z | Current position. |
| `static:getRotation()` | none | w, x, y, z | Current orientation as a quaternion. |
| `static:getFriction()` | none | value | Current friction coefficient. |
| `static:setFriction(friction)` | 0-10, clamped | none | Sets friction. |
| `static:getRestitution()` | none | value | Current restitution. |
| `static:setRestitution(restitution)` | 0-10, clamped | none | Sets restitution. |
| `static:setMeshColor(meshName, r, g, b, a)` | mesh name, color | none | Recolors one mesh and broadcasts the change. |
| `static:setHighlight(r, g, b, a, thickness)` | color; `thickness` is how far (in world units) the outline extends past the model's surface | none | Applies an outline/highlight effect around the whole object and broadcasts it to clients. |
| `static:clearHighlight()` | none | none | Removes the outline/highlight effect. |
| `static:setColliding(bool)` | true/false | none | Enables or disables collision for the object without removing it. |
| `static:setHidden(bool)` | true/false | none | Shows or hides the object client-side. |

---

## Lights

Lights that shine from a spot in the world, either in every direction or, as spotlights, in a cone
(see `light:setConeAngle`). They have no model or collision.
Clients light everything around them (bricks, models, grass, and the water surface, which also shows
glints of them on its waves) with inverse square falloff, draw a
glowing corona where they are, and give the lights nearest the camera shadows. How many get shadows
is each player's `graphics/pointshadows` setting (0 to 8, default 4). Up to 32 lights light the view
at once, the nearest ones win.

A light reaches until it's too dim to see: roughly `sqrt(brightness * 50)` studs for a light whose
brightest color channel is 1, up to 500 (`light:getRange()` gives the exact value). As a guide,
`brightness` 50 at 5 studs is about an eighth of noon sunlight, so a lamp is around 20-100 and a
floodlight a few thousand.

Players' flashlights (see `client:setFlashlightEnabled`) are lights too: an 80 degree spotlight with
brightness 150 and a 0.8 stud corona, which shows up in `getNumLights` and `getLightIdx` while it's on.
Each client shines it from just past the holding player's `Right_Hand` mesh as it's drawn (or in
front of their eyes if their model has no such mesh) toward where that player looks. Like any
spotlight's, its corona only shows to people inside the beam, so its owner doesn't see their own.
`light:getPosition()` gives the player's position, and the server keeps pointing it with
`setDirection`. `light:setPosition` takes it out of the player's hand, and `light:destroy()`
switches it off without the `LightOff` sound.

### Global functions

| Function | Arguments | Returns | Description |
|---|---|---|---|
| `createLight(x, y, z, r, g, b, brightness, flicker, coronaWidth)` | position; color 0-1 per channel; `brightness` 0 or more (clamped to 100000); `flicker` in world units (0-16); `coronaWidth` in world units (0-256, 0 for no corona) | Light | Places a point light. `flicker` is how far the light jumps around its position: every 40-160 ms it snaps to a new random spot within that distance, which makes its lighting and shadows jitter like a flame. Each client picks its own spots. |
| `getLightId(netId)` | net ID | Light | Looks up a light by its net ID. |
| `getLightIdx(index)` | 0-based index | Light | Looks up a light by its position in the internal list. |
| `getNumLights()` | none | count | How many lights currently exist. |

### `light:` methods

All of these use the strict argument count check, and setters log an error and do nothing if an
argument isn't a finite number.

| Method | Arguments | Returns | Description |
|---|---|---|---|
| `light:destroy()` | none | none | Removes the light. |
| `light:getPosition()` | none | x, y, z | Position, without flicker. |
| `light:setPosition(x, y, z)` | position | none | Moves the light. |
| `light:getColor()` | none | r, g, b | Color. |
| `light:setColor(r, g, b)` | 0-1, clamped | none | Sets the color. |
| `light:getBrightness()` | none | number | Brightness. |
| `light:setBrightness(brightness)` | 0-100000, clamped | none | Sets the brightness. `0` turns the light and its corona off. |
| `light:getFlicker()` | none | number | Flicker distance. |
| `light:setFlicker(distance)` | world units, 0-16, clamped | none | How far the light wanders around its position. |
| `light:getCoronaWidth()` | none | number | Corona width. |
| `light:setCoronaWidth(width)` | world units, 0-256, clamped | none | Width of the glow drawn at the light, `0` for none. |
| `light:getRange()` | none | number | How far the light reaches, from its brightness and color. |
| `light:getDirection()` | none | x, y, z | Which way a spotlight points, normalized, not counting spin. Defaults to straight down. |
| `light:setDirection(x, y, z)` | any length but zero | none | Points the spotlight. Also resets how far it has spun. Does nothing for a light with a cone angle of 0. |
| `light:getConeAngle()` | none | degrees | Full width of the beam, 0 for a light that shines every way. |
| `light:setConeAngle(degrees)` | 0, or 1-179 (clamped) | none | Turns the light into a spotlight with a beam this many degrees wide, softening toward its edge. `0` (the default) makes it shine every way again. A spotlight's corona only shows from inside its beam, and its shadows only draw the directions the beam can reach, so they cost less. |
| `light:getSpin()` | none | degrees per second | How fast the direction turns. |
| `light:setSpin(degreesPerSecond)` | -3600 to 3600, clamped | none | Turns the spotlight's direction around the vertical axis, like a lighthouse. Negative spins the other way. Each client spins it on its own, so players can see it at slightly different angles. A spinning spotlight redraws its shadows every frame. |

---

## Emitters

Emitters eject particles: small camera-facing images that move, spin, and change color and size over
their short lives, like Blockland's. A **particle type** says what one particle looks like and how it
moves, an **emitter type** says how particles are ejected, and an **emitter** is one of those placed in
the world, at a spot or following a dynamic. Only the types and emitters are sent to clients, each
client ejects, moves, and draws the particles on its own, so two players never see exactly the same ones.

Types are made by name, and adding one with a name that's taken replaces it: clients already in the
game get the change right away and existing emitters of that type carry on with it. `EmitterDefaults.lua`,
run from `serverstart.lua`, adds the old game's types (converted to the units below) and a
`fountainEmitter` for testing. The server puts a `playerJetEmitter` under each foot of a jetting player
(see `client:setJetsEnabled`), and makes a `playerBubbleEmitter` wherever a dynamic splashes
into the water, if a type by that name exists. `emitterTest()` in `serverstart.lua` places one of every
default type.

Units are Blockland's: angles in degrees, speeds in studs per second, times in milliseconds. Ejection
directions are relative to the emitter: world space for one at a spot or on a brick, and turning with the
dynamic (or the mesh) for one that follows a dynamic. Gravity is always world space. Each client keeps at
most `graphics/maxparticles` particles alive (0 to 100000, default 20000), and emitters much further from
its camera than the end of the fog don't eject any. Particles fade into the fog and show in water
reflections. Particle types with `lit` set are lit and shadowed like smoke or bubbles would be, the rest
keep their colors day and night like fire and sparks.

### Particle type fields

The table passed to `addParticleType`. Anything left out keeps its default. Vectors can be tables like
`{1, 0.5, 0, 1}` or, like the old game's scripts, strings like `"1 0.5 0 1"`. An unknown field name or
a value of the wrong kind logs an error and the type isn't added. Numbers out of range are clamped.

| Field | Default | Description |
|---|---|---|
| `texture` | required | Image path relative to the game folder, like `"Assets/particles/cloud.png"`. Clients load it from their own copy, so it has to exist there too. |
| `color0` - `color3` | `{1, 1, 1, 1}` | RGBA, 0-1, multiplied by the texture. |
| `size0` - `size3` | `1` | Width in studs, 0-256. |
| `time0` - `time3` | `0`, `0.33`, `0.66`, `1` | When over a particle's life (0 to 1) each color and size key applies, blended in between. Kept in order. |
| `drag` | `0` | Fraction of its velocity a particle loses per second, one number or `{x, y, z}`, 0-1000. |
| `gravity` | `{0, 0, 0}` | Acceleration in studs per second squared. `{0, -20, 0}` falls, `{0, 15, 0}` rises like smoke. |
| `inheritedVelFactor` | `0` | How much of the velocity of the dynamic the emitter follows particles start with. |
| `lifetimeMS` | `1000` | How long each particle lives, 1-60000. |
| `lifetimeVarianceMS` | `0` | Each particle lives up to this much longer or shorter, kept under `lifetimeMS`. |
| `spinSpeed` | `0` | Degrees per second the image turns. |
| `useInvAlpha` | `false` | `true` blends by alpha, for smoke and anything that should cover what's behind it. `false` adds its color on top, for glows, sparks, and fire. |
| `needsSorting` | `false` | Draws this type's particles back to front each frame. Alpha blended particles that overlap need it to look right. |
| `lit` | `false` | Lights the particle like a surface of its color would be: by the sun or moon, ambient light, and point lights, darker in sun and point light shadows. For smoke, dust, and bubbles. Leave it off for anything that glows, like fire, sparks, and jets. The whole particle gets the light at its middle, and shadows don't pick up colors from transparent bricks. |

### Emitter type fields

The table passed to `addEmitterType`.

| Field | Default | Description |
|---|---|---|
| `particles` | required | Names of 1-16 particle types, separated by spaces (`"a b"`) or as a table (`{"a", "b"}`). Each particle is one of them picked at random. They have to be added first. |
| `ejectionPeriodMS` | `100` | Milliseconds between particles, at least 1. Each frame an emitter ejects every particle it owes, spread along the way it moved. |
| `periodVarianceMS` | `0` | Each gap is up to this much longer or shorter, kept under `ejectionPeriodMS`. |
| `ejectionVelocity` | `2` | Studs per second away from the emitter. |
| `velocityVariance` | `1` | Up to this much faster or slower, at most `ejectionVelocity`. |
| `ejectionOffset` | `0` | How far out from the emitter, along the way they go, particles start. |
| `thetaMin`, `thetaMax` | `0`, `90` | Degrees down from the emitter's up each particle goes out at, picked between these (0-180). `0, 0` shoots straight up, `90, 90` flat outward, `180, 180` straight down. |
| `phiReferenceVel` | `0` | Degrees per second the direction particles go out in turns around the vertical, for spirals. |
| `phiVariance` | `360` | Degrees around the vertical past that direction a particle can go, `360` for every way. |
| `lifetimeMS` | `0` | Emitters of this type remove themselves this long after they're made, for one-off bursts. `0` lasts until removed. |
| `uiName` | `""` | Name for menus, not used yet. |

### Global functions

| Function | Arguments | Returns | Description |
|---|---|---|---|
| `addParticleType(name, table)` | name of 1-255 characters; fields above | none | Adds a particle type, or replaces the one with that name. Does nothing if its texture doesn't exist on the server. |
| `addEmitterType(name, table)` | name of 1-255 characters; fields above | none | Adds an emitter type, or replaces the one with that name. |
| `getParticleTable(name)` | particle type name | table | A particle type's fields, with vectors as tables. |
| `getEmitterTable(name)` | emitter type name | table | An emitter type's fields, with `particles` as a string of names. |
| `addEmitter(typeName[, x, y, z])` | emitter type name; position, default `0, 0, 0` | Emitter | Places an emitter. |
| `getEmitterId(netId)` | net ID | Emitter | Looks up an emitter by its net ID. |
| `getEmitterIdx(index)` | 0-based index | Emitter | Looks up an emitter by its position in the internal list. |
| `getNumEmitters()` | none | count | How many emitters currently exist. |

### `emitter:` methods

These use the strict argument count check.

| Method | Arguments | Returns | Description |
|---|---|---|---|
| `emitter:destroy()` | none | none | Removes the emitter. Particles it already ejected live out their lifetimes. `emitter:remove()` does the same, it's the old game's name. |
| `emitter:getPosition()` | none | x, y, z | Where it is, or where the dynamic it follows is. |
| `emitter:setPosition(x, y, z)` | position | none | Moves it there, no longer following a dynamic or on a brick. |
| `emitter:getTypeName()` | none | string | Its emitter type's name. |
| `emitter:setType(typeName)` | emitter type name | none | Switches it to another emitter type. |
| `emitter:attachToDynamic(dynamic[, meshName])` | dynamic; name of one of its model's meshes | none | Follows the dynamic, or the middle of that mesh as it animates, ejecting particles turned the way the dynamic (or mesh) is turned. Removed along with the dynamic. |
| `emitter:attachToBrick(brick)` | brick, or `nil` | none | Moves it to the middle of the brick, and it's removed along with the brick. `nil` leaves it where it is, no longer on or following anything. |

---

## Bricks

Basic box bricks on a grid of 1 stud (1 world unit) horizontally by 1 plate (0.4 world units)
vertically. Positions are a brick's **min corner** in whole studs/plates, not its center. Bricks
can never overlap. A table for a brick that has since been removed stays valid Lua, but its
methods log an error and do nothing.

### Global functions

| Function | Arguments | Returns | Description |
|---|---|---|---|
| `addBrick(x, y, z, width, height, length, r, g, b, a[, angleID])` | min corner in studs/plates; `width` and `length` in studs, `height` in plates, each 1-255; color; `angleID` is 0-3 quarter turns (default 0), where 1 and 3 swap width and length | Brick, or `nil` | Adds a brick. Returns `nil` without an error if it would overlap another brick or be out of bounds (`y` below 0). |
| `getNumBricks()` | none | count | How many bricks exist. |
| `getBrickIdx(index)` | 0-based index | Brick | Looks up a brick by its position in the internal list. Removing bricks changes the order. |
| `getBrickId(id)` | net ID | Brick or `nil` | Looks up a brick by its net ID. |
| `getBrickAt(x, y, z)` | one stud/plate grid cell | Brick or `nil` | The brick filling that cell, if any. |
| `clearAllBricks()` | none | none | Removes every brick. |
| `saveBuild(fileName[, omitOwnership])` | file name inside the `Saves` folder; `omitOwnership` writes every owner as `-1` | bool | Saves every brick in the old Land of Dran binary format. |
| `loadLodSave(fileName[, x, y, z])` | file name inside `Saves`; optional offset in studs/plates | count, or `nil` | Loads an old Land of Dran binary save (either version) on top of the current bricks, returning how many were added. Special bricks, lights, music, and prints in the file are skipped. |
| `loadBlocklandSave(fileName)` | file name inside `Saves` | count, or `nil` | Imports a Blockland `.bls` save using its own color palette, returning how many bricks were added. Brick names are matched against `Assets/brick/types`; special bricks (ramps, etc.) and unrecognized names are skipped and listed in the log. |

Save and load functions only accept a plain file name, with no folders, since saves always live
directly in `Saves/`. Bricks that would overlap an existing brick are skipped when loading.

### `brick:` methods

| Method | Arguments | Returns | Description |
|---|---|---|---|
| `brick:getPosition()` | none | x, y, z | Min corner, in studs/plates. |
| `brick:getDimensions()` | none | width, height, length | Size before rotation. |
| `brick:getAngleID()` | none | 0-3 | Quarter turns around the vertical axis. |
| `brick:getColor()` | none | r, g, b, a | Color, 0-1. |
| `brick:setColor(r, g, b, a)` | color | none | Recolors the brick. |
| `brick:isColliding()` | none | bool | Whether players and objects collide with it. |
| `brick:setColliding(collides)` | bool | none | Turns collision on or off. Non-colliding bricks can still be hit by `raycast()`. |
| `brick:getOwner()` | none | client net ID, or `-1` | Who planted it. `-1` for bricks added by Lua or loaded from a save. |
| `brick:getName()` | none | string | The brick's name, empty by default. |
| `brick:setName(name)` | string | none | Sets the brick's name. |
| `brick:remove([showEffect])` | optional bool | none | Removes the brick. With `true`, clients show it popping loose and flying off like an undone brick. Leave it off when removing many bricks at once. |

---

## Sounds

Sounds are registered by name with `newSoundType`, then played by that name. Clients load the
file from their own copy of the game folder when they join (or right away if they're already
connected), so the file has to exist on the clients too. `.wav` (any bit depth) and `.ogg`
(Vorbis) files work, mono or stereo.

Sounds with no position play at the same volume wherever the listener is. Sounds with a position
pan left and right, are at full volume within 5 studs, and past that lose about 10 dB every time
the distance doubles, getting duller as well, so they're close to silent a couple hundred studs
away. Sounds moving toward or away from the listener, or a listener moving toward or away from
them, shift in pitch (the Doppler effect, with sound traveling 343 studs a second). Closing in or
pulling apart slower than 12 studs a second doesn't shift pitch at all, and the shift eases in
up to its full amount at 30 studs a second, so walking around doesn't bend music. The listener
is the client's camera, but its movement for the Doppler effect is that of whatever the camera
follows, so swinging the camera around doesn't change pitch.

Unless Lua picks a preset with `setAudioEffect`, each client's reverb follows the space around
their camera: out in the open there's almost none, and it gets louder and longer the more
closed in and bigger the space is (bricks, statics, and dynamics all count as walls). Under
the water level everything is muffled and sounds like the `underwater` preset. Positioned
sounds with bricks or objects between them and the camera are muffled too, more the thicker
the bricks in the way, and their echo is muffled along with them. Players can turn
these off or change how many raycasts they use in the audio settings.

Clients play a few sounds by name on their own when the server has registered them: `ClickMove`
and `ClickRotate` when the ghost brick moves or turns, `Jump` when their player jumps, and
`BrickBreak` where a removed brick pops loose. `serverstart.lua` registers these along with
`ClickPlant`, `PlayerConnect`, `PlayerLeave`, `Admin` (played to a client who logs into the eval
console), and `BrickClear` (played to everyone when someone types `/clearbricks` in chat to remove
all of their own bricks). It also registers `Splash` and `ExitWater`, which the server plays by
name where dynamics hit or leave the water, louder the faster they're moving and lower pitched
the bigger they are, and `LightOn` and `LightOff`, which the server plays from a player whose
flashlight turns on or off.

In the functions below, `pitch` is a playback speed multiplier (default `1`, clamped to 0.05-10)
and `volume` is 0-1 (default `1`). They can only be given together.

| Function | Arguments | Returns | Description |
|---|---|---|---|
| `newSoundType(name, filePath[, isMusic])` | unique name and a path relative to the game folder, each 1-255 characters; `isMusic` marks it as music (not used yet) | none | Registers a sound. Logs an error and skips it if the name is taken or the file doesn't exist on the server. |
| `playSound(name[, x, y, z][, pitch, volume])` | sound type name; optional world position | none | Plays a sound once for every client, with no position or at `x, y, z`. Sent unreliably, so a client can occasionally miss one. |
| `startSoundLoop(name[, x, y, z][, pitch, volume])` | sound type name; optional world position | loop ID | Starts a sound that repeats until `stopSoundLoop`, with no position or at `x, y, z`. Clients who join later hear it too. Each client only plays the 16 loops closest to them at once; farther ones pause and pick up where they left off. Loops use the music volume setting on top of `volume`. |
| `stopSoundLoop(loopID)` | ID from `startSoundLoop` or `dynamic:startSoundLoop` | none | Stops a loop. Does nothing if it already ended. |
| `setAudioEffect(preset)` | preset name, case insensitive | none | Puts a reverb effect on every sound for every client, including ones who join later. `auto`, the default, has each client's reverb follow the space around them (see above). `none` turns reverb off. Muffling underwater and behind walls happens either way. Presets: `generic`, `paddedcell`, `auditorium`, `concerthall`, `cave`, `forest`, `plain`, `underwater`, `drugged`, `dizzy`, `psychotic`, `outhouse`, `heaven`, `hell`, `memory`, `dustyroom`, `waterroom`, `racer`, `tunnel`. |

See also `dynamic:playSound`, `dynamic:startSoundLoop`, `client:playSound`, and `client:setAudioEffect`.

---

## Voice chat

Players hold push to talk (V by default) to talk. Their voice comes from their player (the dynamic
their movement keys control, otherwise the first dynamic they control), or from their camera if they
have neither, and is only sent to clients whose camera is within the voice range. For the people
listening it works like a positioned sound: full volume within 10 studs, then about 10 dB quieter each
time the distance doubles, muffled behind bricks and underwater, with the same reverb as every other
sound. Each client plays up to 8 people talking at once. Players set their microphone, microphone
volume, and voice chat volume in the audio settings.

| Function | Arguments | Returns | Description |
|---|---|---|---|
| `setVoiceRange(studs)` | distance, at least 0; default 128 | none | How far a talker can be from a client's camera and still be heard. `0` turns voice chat off for everyone. |
| `getVoiceRange()` | none | studs | The current voice range. |

See also `client:setVoiceMuted`, `client:isVoiceMuted`, `client:isTalking`, and the `ClientStartTalking` and `ClientStopTalking` events.

---

## Clients

A "client" represents one connected player/connection.

### Global functions

| Function | Arguments | Returns | Description |
|---|---|---|---|
| `getNumClients()` | none | count | How many clients are currently connected. |
| `getClientIdx(index)` | 0-based index | Client | Looks up a connected client by index. |
| `messageAll(text)` | text (max 255 chars) | none | Broadcasts a chat message from the server to every connected client, as a single packet. Empty strings are silently ignored, same as `client:message`. |
| `centerPrintAll(text)` / `centerPrintAll(text, durationMS)` / `centerPrintAll(text, durationMS, red, green, blue)` | text (max 255 chars); duration in ms (default 3000, clamped to 60000); color 0-1 (default white) | none | Broadcasts a temporary message to the center of every connected client's screen, as a single packet. |

### `client:` methods

| Method | Arguments | Returns | Description |
|---|---|---|---|
| `client:message(text)` | string (max 255 chars) | none | Sends a chat message from the server to just this client. |
| `client:kick()` | none | none | Disconnects the client. |
| `client:getName()` | none | string | The client's display name. |
| `client:getIP()` | none | string | The client's IP address. |
| `client:getID()` | none | net ID | The client's unique net ID. |
| `client:getPing()` | none | ms | Round-trip ping. |
| `client:getPacketLoss()` | none | value | Current packet loss. |
| `client:isAdmin()` | none | bool | Whether the client logged into the eval console as admin. |
| `client:giveControl(dynamic)` | Dynamic | none | Gives the client physics-simulation authority over the dynamic (e.g. their player). |
| `client:removeControl(dynamic)` | Dynamic | none | Takes physics-simulation authority for the dynamic back from the client. |
| `client:getNumControlled()` | none | count | How many dynamics this client currently controls. |
| `client:getControlledIdx(index)` | 0-based index | Dynamic | The controlled dynamic at that index (index 0 is typically their player). |
| `client:setDefaultController(dynamic)` | Dynamic | none | Sets up movement-key/camera-direction input handling for this dynamic (walking, jumping). Currently the only way to stop this is to destroy the dynamic. Also required before `getCursorItem`/`snapToCursor` will have live camera data for this client. |
| `client:bindCamera(dynamic, fixUpVector, maxFollowDistance)` | Dynamic to follow; whether to lock the camera's up vector; max third-person follow distance | none | Binds the client's camera to follow a dynamic. |
| `client:staticCamera(posX, posY, posZ)` | fixed camera position | none | Detaches the camera and locks it to a fixed position (direction stays free/mouse-controlled). |
| `client:staticCamera(posX, posY, posZ, dirX, dirY, dirZ)` | fixed camera position and direction | none | Same, but also locks the look direction. |
| `client:getCursorItem(maxDistance)` | max ray distance | hit object, x, y, z, normalX, normalY, normalZ, distance; or `nil` | Same return values as `raycast()`. Raycasts from the client's *live* camera position/direction (updated continuously, not just on click) out to `maxDistance`, ignoring the client's own first controlled object. Requires `setDefaultController` to have been called for this client. |
| `client:centerPrint(text)` / `client:centerPrint(text, durationMS)` / `client:centerPrint(text, durationMS, red, green, blue)` | text (max 255 chars); duration in ms (default 3000, clamped to 60000); color 0-1 (default white) | none | Shows a temporary message in the center of just this client's screen. |
| `client:playSound(name[, x, y, z][, pitch, volume])` | same as `playSound` | none | Plays a sound once for just this client. |
| `client:setAudioEffect(preset)` | same as `setAudioEffect` | none | Sets the reverb effect for just this client, until something sets it again. Not remembered: `setAudioEffect`'s preset is what a client gets when they join. |
| `client:setVoiceMuted(muted)` | bool | none | Mutes or unmutes the client's voice chat. The server drops their voice while they're muted, and their game shows "Voice Muted" and stops sending. If they were talking, `ClientStopTalking` fires half a second later. Not remembered if they reconnect. |
| `client:isVoiceMuted()` | none | bool | Whether `setVoiceMuted` muted the client. |
| `client:isTalking()` | none | bool | Whether the client is talking right now, between `ClientStartTalking` and `ClientStopTalking`. |
| `client:setJetsEnabled(enabled)` | bool | none | Whether the client can jet, on by default. Holding right mouse cancels gravity and lifts the player from `setDefaultController` up to 30 studs a second, moving at twice walking speed while they walk, not while swimming. Each foot (`Left_Foot` and `Right_Foot` meshes, or the middle of a model without them) gets a `playerJetEmitter` while they jet, if Lua added that emitter type. Turning it off mid-jet drops them and removes the flames. Not remembered if they reconnect. |
| `client:getJetsEnabled()` | none | bool | Whether `setJetsEnabled` lets the client jet. |
| `client:setFlashlightEnabled(enabled)` | bool | none | Whether the client can use their flashlight, on by default. Players tap their flashlight key (`]` by default) to switch it on or off, and hold it to cycle through colors starting from white. The `LightOn` and `LightOff` sounds play from their player. Turning it off switches off a flashlight that's on. Needs a player from `setDefaultController` to hold it (see Lights). Not remembered if they reconnect. |
| `client:getFlashlightEnabled()` | none | bool | Whether `setFlashlightEnabled` lets the client use a flashlight. |
| `client:applyAppearance(dynamic)` | Dynamic | none | Puts the colors and face the client picked in their appearance editor on the dynamic, usually their player in `ClientJoin`. Their game sends their appearance as they connect: each painted part is matched to a mesh by name ignoring case (parts the model doesn't have are skipped), and the face goes on the `Face1` mesh, or `Face` or `Head` if there's no `Face1`, like `dynamic:setMeshDecal`. If they save a change while connected, it's put on the last dynamic this was called with, and parts they no longer paint go back to the model's own look. |
