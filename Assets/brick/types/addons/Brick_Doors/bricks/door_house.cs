// door_House.cs House door datablocks
// this contains explanations on some things that the other files don't

// this is the door that all the other ones inherit from
datablock fxDTSBrickData ( brickDoorHouseOpenCWData )
{
	brickFile = "./door_House_openCW.blb";
	// disabled this so you can have a ui name but not be in the brick selection dialog
	// category = "special";
	// subCategory = "misc";
	uiName = "House Door";
	
	// we need some info on the door
	isDoor = 1;
	isOpen = 1;
	
	// we need to know what datablock to switch to when we open & close a door
	closedCW = "brickDoorHouseCWData";
	openCW = "brickDoorHouseOpenCWData";
	
	closedCCW = "brickDoorHouseCWData";
	openCCW = "brickDoorHouseOpenCCWData";
	
	// this is done so i can copy textures from the window brick easily
	orientationFix = 1;
};

datablock fxDTSBrickData ( brickDoorHouseOpenCCWData : brickDoorHouseOpenCWData )
{
	brickFile = "./door_House_openCCW.blb";
	isOpen = 1;
};

// since this is the default state of the door we have to load it last
// this is done so this door gets loaded instead of the others when loading, similar to the treasure chest
datablock fxDTSBrickData ( brickDoorHouseCWData : brickDoorHouseOpenCWData )
{
	brickFile = "./door_House_closed.blb";
	
	// we only want to be able to plant this door, since it's the closed one
	// so it has to be in the brick selection menu
	category = "special";
	subCategory = "Doors";
	
	iconName = "Add-Ons/Brick_Doors/bricks/House Door";

	isOpen = 0;
};