// door_HouseWindow.cs HouseWindow door datablocks
datablock fxDTSBrickData ( brickDoorHouseWindowOpenCWData )
{
	brickFile = "./door_HouseWindow_openCW.blb";
	uiName = "Window Door";
	
	isDoor = 1;
	isOpen = 1;
	
	closedCW = "brickDoorHouseWindowCWData";
	openCW = "brickDoorHouseWindowOpenCWData";
	
	closedCCW = "brickDoorHouseWindowCWData";
	openCCW = "brickDoorHouseWindowOpenCCWData";
	
	orientationFix = 1;
};

datablock fxDTSBrickData ( brickDoorHouseWindowOpenCCWData : brickDoorHouseWindowOpenCWData )
{
	brickFile = "./door_HouseWindow_openCCW.blb";
	
	isOpen = 1;
};

datablock fxDTSBrickData ( brickDoorHouseWindowCWData : brickDoorHouseWindowOpenCWData )
{
	brickFile = "./door_HouseWindow_closed.blb";
	category = "special";
	subCategory = "Doors";

	iconName = "Add-Ons/Brick_Doors/bricks/Window Door";

	isOpen = 0;
	
};