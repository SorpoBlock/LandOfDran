brickhead = newDynamicType("brickhead","Assets/brickhead/brickhead.txt",0.02,0.02,0.02)
addAnimation(brickhead,"walk",0,30,0.04,200,400)

metal = newDynamicType("metal","Assets/cube/cube.txt",0.02,0.02,0.02)

function activeCheck()
	active = 0
	for y = 0, getNumDynamics()-1, 1 do
		if getDynamicIdx(y):isActive() then
			active = active + 1
		end
	end
	info("Num active dynamics: ",active)
end

function jump()
	for i=0,getNumDynamics()-1, 1 do
		getDynamicIdx(i):setVelocity(0,50,0)
	end
end

ang = 0
function a()
	ang = ang + 0.01
	if ang > 6.28 then
		ang = ang - 6.28
	end
	getDynamicIdx(0):setRotation(ang,0,0)
	getDynamicIdx(0):setPosition(0,0,0)	
	getDynamicIdx(0):setAngularVelocity(0,0,0)
	getDynamicIdx(0):setVelocity(0,0,0)

	schedule(1,"a")
end

function color()
	for i=0,getNumDynamics()-1,1 do
		getDynamicIdx(i):setMeshColor("Head",math.random(),math.random(),math.random(),1)
		getDynamicIdx(i):setMeshColor("Face1",math.random(),math.random(),math.random(),1)
		getDynamicIdx(i):setMeshColor("Torso",math.random(),math.random(),math.random(),1)
		getDynamicIdx(i):setMeshColor("Right_Hand",math.random(),math.random(),math.random(),1)
		getDynamicIdx(i):setMeshColor("Left_Hand",math.random(),math.random(),math.random(),1)
		getDynamicIdx(i):setMeshColor("Left_Leg",math.random(),math.random(),math.random(),1)
		getDynamicIdx(i):setMeshColor("Right_Leg",math.random(),math.random(),math.random(),1)
		getDynamicIdx(i):setMeshColor("Left_Foot",math.random(),math.random(),math.random(),1)
		getDynamicIdx(i):setMeshColor("Right_Foot",math.random(),math.random(),math.random(),1)
	end
end

t = {}
for x = 0, 0, 1 do
	for y = 0, 0, 1 do
		idx = x * 7 + y
		t[idx] = createDynamic(0,x*3.5,5,y*3.5)
	end
end

s = createStatic(0,-10,0,0)

function join(client)	
	--Create a player for the client
	dynamic = createDynamic(0,0,50,0)
	dynamic:setAngularFactor(0,0,0);
	client:giveControl(dynamic)
	client:bindCamera(dynamic,true,20)
	client:setDefaultController(dynamic)
	
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
