// door_glass.cs glass door datablocks

datablock fxDTSBrickData ( brickDoorGlassOpenCWData )
{
	brickFile = "./door_Glass_openCW.blb";
	uiName = "Glassiest Door";
	
	isDoor = 1;
	isOpen = 1;
	
	closedCW = "brickDoorGlassCWData";
	openCW = "brickDoorGlassOpenCWData";
	
	closedCCW = "brickDoorGlassCCWData";
	openCCW = "brickDoorGlassOpenCCWData";
	
	orientationFix = 1;
};

datablock fxDTSBrickData ( brickDoorGlassCCWData : brickDoorGlassOpenCWData )
{
	brickFile = "./door_Glass_closedCCW.blb";
	
	isOpen = 0;
};

datablock fxDTSBrickData ( brickDoorGlassOpenCCWData : brickDoorGlassOpenCWData )
{
	brickFile = "./door_Glass_openCCW.blb";

	isOpen = 1;
};

datablock fxDTSBrickData ( brickDoorGlassCWData : brickDoorGlassOpenCWData )
{
	brickFile = "./door_Glass_closedCW.blb";
	category = "special";
	subCategory = "Doors";

	iconName = "Add-Ons/Brick_Doors/bricks/Glass Door";
	
	isOpen = 0;
};