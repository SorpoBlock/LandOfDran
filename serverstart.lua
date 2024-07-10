t = {}
for x = 0, 7, 1 do
	for y = 0, 7, 1 do
		idx = x * 7 + y
		--t[idx] = createDynamic(0,x*3.5,5,y*3.5)
	end
end

function join(client)
	
	--Create a player for the client
	dynamic = createDynamic(0,0,50,0)
	client:addControl(dynamic)
	
	return client
end
registerEventListener("ClientJoin","join")

function leave(client)

	--Destroy the client's player(s)
	for i = 0, client:getNumControlled() - 1, 1 do
		client:getControlledIdx(i):destroy()
	end
	
	return client
end
registerEventListener("ClientLeave","leave")

function otherJoin(client)

	--Create the actual player object
	dynamic = createDynamic(0,0,50,0)
	
	--Make it so the object cannot rotate on its own / through collisions
	dynamic:setAngularFactor(0,0,0)

	--Could be messed with
	--dynamic:setGravity(0,30,0)
	
	--Let client be authoritative for the position/rotation of this object, simulating its physics
	client:addControl(dynamic)
	
	--Set up motion controls:
	--Cmd name, movement type, needsTouchGround...
	--Relative to camera dir: x-axis sin phase shift, x-axis multipler, y-axis, z-axis
	dynamic:setMovementControl("forward" , true, "relative",  0.0,  1.0, 0.0, 0.0, -1.5707,  1.0)
	dynamic:setMovementControl("backward" , true,"relative",  0.0, -1.0, 0.0, 0.0, -1.5707, -1.0)
	dynamic:setMovementControl("right" , true,   "relative",  1.5707,  1.0, 0.0, 0.0, 0.0,  1.0)
	dynamic:setMovementControl("left" , true,    "relative",  1.5707, -1.0, 0.0, 0.0, 0.0, -1.0)
	--Absolute: x-velocity, y-velocity, z-velocity
	dynamic:setMovementControl("jump" , true,    "absolute-set",  0.0, 30.0, 0.0)
	
	--50ms ramp up for moving if we're on the ground, 600ms if we're flying
	dyanmic:setMovementBlendSpeed(50,600)
	
	--Set animations for walking, 1.0 is playback speed:
	dynamic:setAnimation("forward", "walk",1.0)
	dynamic:setAnimation("backward","walk",1.0)
	dynamic:setAnimation("left",    "walk",1.0)
	dynamic:setAnimation("right",   "walk",1.0)

	--Have client's camera follow object around
	client:bindCamera(dynamic)
	client:setThirdPersonEnabled(true,30.0) -- Maximum distance away from object
	client:setFirstPersonEnabled(true) -- These are both enabled by default
	
end

