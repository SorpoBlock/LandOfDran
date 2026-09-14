// door_Plain.cs Plain door datablocks

datablock fxDTSBrickData ( brickDoorPlainOpenCWData )
{
	brickFile = "./door_Plain_openCW.blb";
	uiName = "Plain Door";
	
	isDoor = 1;
	isOpen = 1;
	
	closedCW = "brickDoorPlainCWData";
	openCW = "brickDoorPlainOpenCWData";
	
	closedCCW = "brickDoorPlainCWData";
	openCCW = "brickDoorPlainOpenCCWData";
	
	orientationFix = 1;
};

datablock fxDTSBrickData ( brickDoorPlainOpenCCWData : brickDoorPlainOpenCWData )
{
	brickFile = "./door_Plain_openCCW.blb";

	isOpen = 1;
};

datablock fxDTSBrickData ( brickDoorPlainCWData : brickDoorPlainOpenCWData )
{
	brickFile = "./door_Plain_closed.blb";
	category = "special";
	subCategory = "Doors";

	iconName = "Add-Ons/Brick_Doors/bricks/Plain Door";
	
	isOpen = 0;
};