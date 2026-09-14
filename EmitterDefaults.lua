--Particle and emitter types, from the old game's defaults (add-ons/emitter_defaults/server.lua), run from serverstart.lua
--Angles are in degrees like Blockland, the old scripts gave them in radians. Emitters now eject every particle they owe each frame like
--Blockland, where the old game ejected at most one a frame, so ejection periods are longer than the old ones to keep about the same look
--Colors, drag, and gravity can be tables like {1, 0.5, 0} or strings like "1 0.5 0". See the Emitters section of LuaAPI.md for every field
--Smoke, nuts, bubbles, and the fountain are lit, so they darken at night and in shade. Glows, fire, and sparks aren't

--Admin orb
addParticleType("CameraParticle", {
	texture = "Assets/particles/dot.png",
	color0 = {0.6, 0, 0, 0},
	color1 = {1.0, 1.0, 0.266, 0.3},
	color2 = {0.6, 0, 0, 0},
	color3 = {0.6, 0, 0, 0},
	size0 = 4.5, size1 = 4.5, size2 = 0, size3 = 0,
	time0 = 0, time1 = 0.5, time2 = 0.95, time3 = 1,
	drag = 0.02,
	gravity = {0.02, 0.02, 0.02},
	lifetimeMS = 200,
	useInvAlpha = true,
	needsSorting = true
})

addEmitterType("CameraEmitter", {
	particles = "CameraParticle",
	uiName = "Admin Orb",
	ejectionOffset = 0.5,
	ejectionPeriodMS = 16,
	ejectionVelocity = 0,
	velocityVariance = 0,
	thetaMin = 0,
	thetaMax = 180,
	phiVariance = 360
})

addParticleType("playerJetParticle", {
	texture = "Assets/particles/cloud.png",
	color0 = {0.0, 0.0, 1.0, 0.6},
	color1 = {1.0, 0.46, 0.0, 0.5},
	color2 = {1.0, 0.46, 0.0, 0.4},
	color3 = {1.0, 0.0, 0.0, 0.0},
	size0 = 1.6, size1 = 1.0, size2 = 1.0, size3 = 2.1,
	time0 = 0, time1 = 0.2, time2 = 0.3, time3 = 1,
	drag = 0.2,
	gravity = {0, -5, 0},
	inheritedVelFactor = 1,
	lifetimeMS = 200,
	lifetimeVarianceMS = 10,
	spinSpeed = 1,
	useInvAlpha = true,
	needsSorting = true
})

--Points straight down from whatever it's attached to
addEmitterType("playerJetEmitter", {
	particles = "playerJetParticle",
	uiName = "Player Jet",
	ejectionPeriodMS = 10,
	ejectionVelocity = 5,
	velocityVariance = 0,
	thetaMin = 180,
	thetaMax = 180,
	phiVariance = 360
})

addParticleType("wrenchSparkParticle", {
	texture = "Assets/particles/chunk.png",
	color0 = {0.2, 0.07, 0.0, 1.0},
	color1 = {0.0, 0.0, 0.0, 0.47},
	color2 = {0.0, 0.0, 0.0, 0.47},
	color3 = {0.0, 0.0, 0.0, 0.0},
	size0 = 0.75, size1 = 0.3, size2 = 0, size3 = 0,
	time0 = 0, time1 = 0.45, time2 = 0.55, time3 = 1,
	drag = 2,
	gravity = {0, -20, 0},
	spinSpeed = 150,
	inheritedVelFactor = 0.2,
	lifetimeMS = 400,
	lifetimeVarianceMS = 300,
	useInvAlpha = false,
	needsSorting = true
})

addEmitterType("wrenchSparkEmitter", {
	particles = "wrenchSparkParticle",
	uiName = "Wrench Oil",
	lifetimeMS = 50,
	ejectionOffset = 0.5,
	ejectionPeriodMS = 5,
	ejectionVelocity = 15,
	velocityVariance = 9,
	thetaMin = 0,
	thetaMax = 90,
	phiVariance = 360
})

addParticleType("hammerSparkParticle", {
	texture = "Assets/particles/star1.png",
	color0 = {1.0, 1.0, 0.0, 0.0},
	color1 = {1.0, 1.0, 0.0, 0.47},
	color2 = {1.0, 1.0, 0.0, 0.0},
	color3 = {1.0, 1.0, 0.0, 0.0},
	size0 = 0.45, size1 = 0.15, size2 = 0, size3 = 0,
	time0 = 0, time1 = 0.5, time2 = 0.99, time3 = 1,
	drag = 2,
	gravity = {0, -20, 0},
	spinSpeed = 150,
	inheritedVelFactor = 0.2,
	lifetimeMS = 400,
	lifetimeVarianceMS = 300,
	useInvAlpha = false,
	needsSorting = true
})

addEmitterType("hammerSparkEmitter", {
	particles = "hammerSparkParticle",
	uiName = "Hammer Spark",
	lifetimeMS = 50,
	ejectionOffset = 0.5,
	ejectionPeriodMS = 5,
	ejectionVelocity = 15,
	velocityVariance = 9,
	thetaMin = 0,
	thetaMax = 90,
	phiVariance = 360
})

addParticleType("hammerExplosionParticle", {
	lit = true,
	texture = "Assets/particles/cloud.png",
	color0 = {1.0, 1.0, 1.0, 0.2},
	color1 = {1.0, 1.0, 1.0, 0.2},
	color2 = {1.0, 1.0, 1.0, 0.2},
	color3 = {0.0, 0.0, 0.0, 0.0},
	size0 = 1, size1 = 2, size2 = 3, size3 = 3,
	time0 = 0, time1 = 0.25, time2 = 0.5, time3 = 1,
	drag = 4,
	gravity = {0, 15, 0},
	spinSpeed = 50,
	inheritedVelFactor = 0.2,
	lifetimeMS = 500,
	lifetimeVarianceMS = 300,
	useInvAlpha = true,
	needsSorting = true
})

addEmitterType("hammerExplosionEmitter", {
	particles = "hammerExplosionParticle",
	uiName = "Hammer Smoke",
	lifetimeMS = 50,
	ejectionPeriodMS = 5,
	ejectionVelocity = 30,
	velocityVariance = 3,
	thetaMin = 80,
	thetaMax = 85,
	phiVariance = 360
})

addParticleType("wrenchExplosionParticle", {
	lit = true,
	texture = "Assets/particles/nut.png",
	color0 = {1.0, 1.0, 0.0, 1.0},
	color1 = {1.0, 1.0, 0.0, 1.0},
	color2 = {1.0, 1.0, 0.0, 0.0},
	color3 = {0.0, 0.0, 0.0, 0.0},
	size0 = 0.6, size1 = 0.6, size2 = 0.6, size3 = 0.6,
	time0 = 0, time1 = 0.75, time2 = 0.9, time3 = 1,
	drag = 8,
	gravity = {0, 15, 0},
	spinSpeed = 50,
	inheritedVelFactor = 0.2,
	lifetimeMS = 500,
	lifetimeVarianceMS = 300,
	useInvAlpha = true,
	needsSorting = true
})

addEmitterType("wrenchExplosionEmitter", {
	particles = "wrenchExplosionParticle",
	uiName = "Wrench Nut",
	lifetimeMS = 50,
	ejectionPeriodMS = 5,
	ejectionVelocity = 30,
	velocityVariance = 3,
	thetaMin = 80,
	thetaMax = 85,
	phiVariance = 360
})

addParticleType("playerBubbleParticle", {
	lit = true,
	texture = "Assets/particles/bubble.png",
	color0 = {0.66, 0.8, 1.0, 0.8},
	color1 = {0.66, 0.8, 1.0, 0.8},
	color2 = {0.66, 0.8, 1.0, 0.8},
	color3 = {0.66, 0.8, 1.0, 0.0},
	size0 = 0.6, size1 = 0.6, size2 = 0.6, size3 = 1.0,
	time0 = 0, time1 = 0.25, time2 = 0.5, time3 = 1,
	gravity = {0, -20, 0},
	lifetimeMS = 1200,
	lifetimeVarianceMS = 400,
	useInvAlpha = false,
	needsSorting = true
})

--The server makes one of these where a dynamic falls into the water fast enough to splash
addEmitterType("playerBubbleEmitter", {
	particles = "playerBubbleParticle",
	uiName = "Player Bubbles",
	lifetimeMS = 70,
	ejectionOffset = 0.4,
	ejectionPeriodMS = 5,
	ejectionVelocity = 21,
	velocityVariance = 9,
	thetaMin = 0,
	thetaMax = 30,
	phiVariance = 360
})

addParticleType("gunSmokeParticle", {
	lit = true,
	texture = "Assets/particles/cloud.png",
	color0 = {1.0, 1.0, 1.0, 0.1},
	color1 = {1.0, 1.0, 1.0, 0.1},
	color2 = {1.0, 1.0, 1.0, 0.1},
	color3 = {1.0, 1.0, 1.0, 0.0},
	size0 = 0.2, size1 = 0.4, size2 = 0.5, size3 = 3,
	time0 = 0, time1 = 0.25, time2 = 0.5, time3 = 1,
	drag = 4,
	gravity = {0, 20, 0},
	spinSpeed = 5,
	inheritedVelFactor = 1,
	lifetimeMS = 2000,
	lifetimeVarianceMS = 300,
	useInvAlpha = true,
	needsSorting = true
})

--The old ones had an ejection velocity of 0 plus or minus 3, but Blockland doesn't allow more variance than velocity
addEmitterType("gunSmokeEmitter", {
	particles = "gunSmokeParticle",
	uiName = "Gun Smoke",
	lifetimeMS = 600,
	ejectionPeriodMS = 16,
	ejectionVelocity = 1.5,
	velocityVariance = 1.5,
	thetaMin = 80,
	thetaMax = 85,
	phiVariance = 360
})

addEmitterType("shellTrailEmitter", {
	particles = "gunSmokeParticle",
	uiName = "Shell Smoke",
	ejectionPeriodMS = 16,
	ejectionVelocity = 1.5,
	velocityVariance = 1.5,
	thetaMin = 80,
	thetaMax = 85,
	phiVariance = 360
})

addParticleType("ouchParticle", {
	texture = "Assets/particles/Pain.png",
	color0 = {1.0, 0.0, 0.0, 1.0},
	color1 = {1.0, 1.0, 0.0, 0.47},
	color2 = {1.0, 0.0, 0.0, 0.0},
	color3 = {1.0, 0.0, 0.0, 0.0},
	size0 = 3, size1 = 1.5, size2 = 0.5, size3 = 0,
	time0 = 0, time1 = 0.5, time2 = 0.99, time3 = 1,
	drag = 2,
	gravity = {0, 2, 0},
	spinSpeed = 0.2,
	inheritedVelFactor = 0.2,
	lifetimeMS = 1600,
	lifetimeVarianceMS = 300,
	useInvAlpha = false,
	needsSorting = true
})

addEmitterType("ouchEmitter", {
	particles = "ouchParticle",
	uiName = "Ouch",
	lifetimeMS = 50,
	ejectionOffset = 0.5,
	ejectionPeriodMS = 25,
	ejectionVelocity = 15,
	velocityVariance = 9,
	thetaMin = 0,
	thetaMax = 90,
	phiVariance = 360
})

--New, not from the old game: a small fountain that runs until it's removed, handy for testing
addParticleType("fountainParticle", {
	lit = true,
	texture = "Assets/particles/dot.png",
	color0 = {0.5, 0.7, 1.0, 0.9},
	color1 = {0.4, 0.6, 1.0, 0.8},
	color2 = {0.4, 0.6, 1.0, 0.5},
	color3 = {0.4, 0.6, 1.0, 0.0},
	size0 = 0.5, size1 = 0.5, size2 = 0.4, size3 = 0.3,
	time0 = 0, time1 = 0.3, time2 = 0.7, time3 = 1,
	drag = 0.1,
	gravity = {0, -20, 0},
	lifetimeMS = 1800,
	lifetimeVarianceMS = 300,
	useInvAlpha = true,
	needsSorting = true
})

addEmitterType("fountainEmitter", {
	particles = "fountainParticle",
	uiName = "Fountain",
	ejectionPeriodMS = 8,
	periodVarianceMS = 2,
	ejectionVelocity = 18,
	velocityVariance = 2,
	thetaMin = 0,
	thetaMax = 8,
	phiVariance = 360
})
