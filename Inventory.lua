--[[
	Inventory

	Everyone starts with a hammer, a wrench, and a paint can, and can carry up to 5 items. Q slides their items out on the
	right of the screen and puts the picked one in their hand, the mouse wheel picks another while they're out, and Ctrl+W
	throws the one in their hand. Left clicking an item on the ground picks it up.

	Holding left mouse with the hammer or wrench in hand swings it, hitting right away and then about once a second until it's
	let go. The hammer knocks loose the brick it hits, and the wrench opens the brick's wrench dialog and stops. Hitting anything
	else plays HammerHit or WrenchMiss.

	Holding left mouse with the paint can in hand sprays their paint palette's color where they're looking, painting every
	brick the crosshair passes over with that color and material.

	Run from serverstart.lua with dofile("Inventory.lua"), after the item types are added.
]]

--From NetTypes/NetType.h's SimObjectType enum
local DYNAMIC_TYPE_ID = 1
local BRICK_TYPE_ID = 4

--How far out from the camera a click looks for an item or brick, a third person camera sits well behind its player
local CLICK_RANGE = 60
--How far what was clicked can be from the player and still be reached
local REACH = 10

--How often a hammer or wrench hits again while left mouse stays down, in milliseconds
local TOOL_REPEAT_MS = 1000

--How far from the player the paint can reaches, a little further than the hammer and wrench, and how often it paints the brick under the crosshair while spraying, in milliseconds
local PAINT_REACH = 13
local PAINT_TICK_MS = 30

--How fast a thrown item leaves the player in studs per second, how far in front of them it starts, and how far above their position
local THROW_SPEED = 30
local THROW_START = 2.5
local THROW_HEIGHT = 1.5

--Item types from serverstart.lua everyone gets as they join, filling their slots in this order
local STARTING_ITEMS = {"hammer", "wrench", "paintCan"}

--Net IDs of items giveStartingItems handed out and nobody has thrown yet
--These are removed instead of dropped when their player leaves, so people coming and going don't leave piles of tools behind
startingItems = {}

--By client net ID while their paint can sprays: the paint can, its spray emitter, its sound loop, and the schedule of the next paintTick
spraying = {}

--The client's player, the first dynamic they control, or nil
local function playerOf(client)
	if client:getNumControlled() == 0 then
		return nil
	end
	return client:getControlledIdx(0)
end

--Whether a spot is within reach studs of the client's player
local function withinReach(client, x, y, z, reach)
	local player = playerOf(client)
	if player == nil then
		return false
	end

	local px, py, pz = player:getPosition()
	return (x - px) ^ 2 + (y - py) ^ 2 + (z - pz) ^ 2 <= reach * reach
end

--Bricks keep colors as bytes, so this is whether two 0-1 colors paint the same
local function sameColor(r1, g1, b1, a1, r2, g2, b2, a2)
	local function byte(value)
		return math.floor(math.max(0, math.min(1, value)) * 255 + 0.5)
	end
	return byte(r1) == byte(r2) and byte(g1) == byte(g2) and byte(b1) == byte(b2) and byte(a1) == byte(a2)
end

function giveStartingItems(client)
	for _, typeName in ipairs(STARTING_ITEMS) do
		local typeID = getDynamicType(typeName)
		if typeID ~= nil then
			local item = createItem(typeID, 0, 0, 0)
			if client:addItem(item) == nil then
				item:destroy()
			else
				startingItems[item.id] = true
			end
		end
	end

	return client
end
registerEventListener("ClientJoin", "giveStartingItems")

local function stopSpraying(client)
	local spray = spraying[client:getID()]
	if spray == nil then
		return
	end

	spraying[client:getID()] = nil
	if spray.tick ~= nil then
		cancel(spray.tick)
	end
	if spray.loop ~= nil then
		stopSoundLoop(spray.loop)
	end
	if spray.emitter ~= nil then
		spray.emitter:destroy()
	end
end

--Paints the brick under the crosshair and keeps the spray the palette's color, for as long as they hold the paint can out
function paintTick(client)
	local spray = spraying[client:getID()]
	if spray == nil then
		return
	end

	--Put away, switched for another item, or thrown
	local held = client:getHeldItem()
	if held == nil or held.id ~= spray.item.id then
		stopSpraying(client)
		return
	end

	local r, g, b, a = client:getPaintColor()
	local material = client:getPaintMaterial()
	if spray.emitter ~= nil then
		spray.emitter:setColor(r, g, b, a)
	end

	local hit, x, y, z = client:getCursorItem(CLICK_RANGE)
	if hit ~= nil and hit.type == BRICK_TYPE_ID and withinReach(client, x, y, z, PAINT_REACH) then
		if not sameColor(r, g, b, a, hit:getColor()) then
			hit:setColor(r, g, b, a)
		end
		if hit:getMaterial() ~= material then
			hit:setMaterial(material)
		end
	end

	spray.tick = schedule(PAINT_TICK_MS, "paintTick", client)
end

local function startSpraying(client, can)
	--A stream from the can to wherever they're looking, in their paint color
	local emitter = addEmitter("paintEmitter")
	local player = playerOf(client)
	if emitter ~= nil then
		emitter:attachToDynamic(can)
		if player ~= nil then
			emitter:aimWith(player, PAINT_REACH)
		end
	end

	spraying[client:getID()] = { item = can, emitter = emitter, loop = can:startSoundLoop("SprayLoop") }
	paintTick(client)
end

--By client net ID while their hammer or wrench keeps hitting: the tool, and the schedule of its next toolTick
toolSwings = {}

local function stopToolSwings(client)
	local swing = toolSwings[client:getID()]
	if swing == nil then
		return
	end

	toolSwings[client:getID()] = nil
	if swing.tick ~= nil then
		cancel(swing.tick)
	end
end

--One hit of the hammer or wrench wherever the crosshair is, returns true if the wrench opened a brick's wrench dialog
local function hitWithTool(client, tool)
	local hit, x, y, z = client:getCursorItem(CLICK_RANGE)
	if x == nil or not withinReach(client, x, y, z, REACH) then
		return false
	end

	--Sparks and a burst wherever it hits, brick or not
	addEmitter(tool .. "SparkEmitter", x, y, z)
	addEmitter(tool .. "ExplosionEmitter", x, y, z)

	local isBrick = hit ~= nil and hit.type == BRICK_TYPE_ID
	if tool == "hammer" then
		if isBrick then
			hit:remove(true)
		else
			playSound("HammerHit", x, y, z)
		end
	elseif isBrick then
		playSound("WrenchHit", x, y, z)
		client:openWrenchDialog(hit)
		return true
	else
		playSound("WrenchMiss", x, y, z)
	end

	return false
end

--Hits with the tool in their hand and does it again in a second, for as long as they hold it out
function toolTick(client)
	local swing = toolSwings[client:getID()]
	if swing == nil then
		return
	end

	--Put away, switched for another item, or thrown
	local held = client:getHeldItem()
	if held == nil or held.id ~= swing.item.id then
		stopToolSwings(client)
		return
	end

	--A wrenched brick's dialog is open, so it's done
	if hitWithTool(client, held:getTypeName()) then
		stopToolSwings(client)
		held:stopAnimation("swing")
		return
	end

	swing.tick = schedule(TOOL_REPEAT_MS, "toolTick", client)
end

function inventoryClick(client, posX, posY, posZ, dirX, dirY, dirZ, mask)
	--Left mouse only
	if (mask & 1) == 0 or playerOf(client) == nil then
		return client, posX, posY, posZ, dirX, dirY, dirZ, mask
	end

	--Clicking the ground gives no object, but still where it was hit
	local hit, x, y, z = client:getCursorItem(CLICK_RANGE)
	local reached = x ~= nil and withinReach(client, x, y, z, REACH)

	--An item on the ground goes into the first empty slot
	if reached and hit ~= nil and hit.type == DYNAMIC_TYPE_ID and hit:isItem() and not hit:isHeld() then
		if client:addItem(hit) == nil then
			client:centerPrint("You can't carry any more items.", 2000)
		end
		return client, posX, posY, posZ, dirX, dirY, dirZ, mask
	end

	local held = client:getHeldItem()
	if held == nil then
		return client, posX, posY, posZ, dirX, dirY, dirZ, mask
	end

	local tool = held:getTypeName()

	--Sprays until left mouse is let go, see inventoryClickRelease
	if tool == "paintCan" then
		if spraying[client:getID()] == nil then
			startSpraying(client, held)
		end
		return client, posX, posY, posZ, dirX, dirY, dirZ, mask
	end

	if tool ~= "hammer" and tool ~= "wrench" then
		return client, posX, posY, posZ, dirX, dirY, dirZ, mask
	end

	--Swings and hits until left mouse is let go, see inventoryClickRelease
	held:playAnimation("swing", true)
	stopToolSwings(client)
	toolSwings[client:getID()] = { item = held }
	toolTick(client)

	return client, posX, posY, posZ, dirX, dirY, dirZ, mask
end
registerEventListener("ClientClick", "inventoryClick")

function inventoryClickRelease(client, posX, posY, posZ, dirX, dirY, dirZ, mask)
	if (mask & 1) ~= 0 then
		stopSpraying(client)
		stopToolSwings(client)

		--Every carried item, in case they picked another slot partway through a swing
		for slot = 0, 4 do
			local item = client:getItem(slot)
			if item ~= nil then
				item:stopAnimation("swing")
			end
		end
	end

	return client, posX, posY, posZ, dirX, dirY, dirZ, mask
end
registerEventListener("ClientClickRelease", "inventoryClickRelease")

--Throws the item in their hand the way they're looking
function throwItem(client, slot)
	local _, open = client:getSelectedSlot()
	if not open or client:getItem(slot) == nil then
		return client, slot
	end

	stopSpraying(client)
	stopToolSwings(client)

	local player = playerOf(client)
	local item = client:removeItem(slot)
	item:stopAnimation()
	startingItems[item.id] = nil

	if player ~= nil then
		local dirX, dirY, dirZ = client:getCameraDirection()
		local x, y, z = player:getPosition()
		local velX, velY, velZ = player:getVelocity()

		item:setPosition(x + dirX * THROW_START, y + THROW_HEIGHT + dirY * THROW_START, z + dirZ * THROW_START)
		item:setVelocity(velX + dirX * THROW_SPEED, velY + dirY * THROW_SPEED, velZ + dirZ * THROW_SPEED)
		--Tumbling end over end
		item:setAngularVelocity(math.random() * 10 - 5, math.random() * 10 - 5, math.random() * 10 - 5)
	end

	return client, slot
end
registerEventListener("ClientDropItem", "throwItem")

--Registered before serverstart.lua's leave, so their player is still around to drop things next to
function dropItemsOnLeave(client)
	stopSpraying(client)
	stopToolSwings(client)

	for slot = 0, 4 do
		local item = client:getItem(slot)
		if item ~= nil then
			if startingItems[item.id] then
				startingItems[item.id] = nil
				item:destroy()
			else
				client:removeItem(slot)
			end
		end
	end

	return client
end
registerEventListener("ClientLeave", "dropItemsOnLeave")
