// door_Jail.cs Jail door datablocks

datablock fxDTSBrickData ( brickDoorJailOpenCWData )
{
	brickFile = "./door_Jail_openCW.blb";
	uiName = "Jail Door";
	
	isDoor = 1;
	isOpen = 1;
	
	closedCW = "brickDoorJailCWData";
	openCW = "brickDoorJailOpenCWData";
	
	closedCCW = "brickDoorJailCWData";
	openCCW = "brickDoorJailOpenCCWData";
	
	orientationFix = 1;
};

datablock fxDTSBrickData ( brickDoorJailOpenCCWData : brickDoorJailOpenCWData )
{
	brickFile = "./door_Jail_openCCW.blb";

	isOpen = 1;
};

datablock fxDTSBrickData ( brickDoorJailCWData : brickDoorJailOpenCWData )
{
	brickFile = "./door_Jail_closed.blb";
	category = "special";
	subCategory = "Doors";

	iconName = "Add-Ons/Brick_Doors/bricks/Jail Door";
	
	isOpen = 0;
};