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
