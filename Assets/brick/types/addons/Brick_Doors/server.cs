//server.cs -- for Brick_Doors, execute all things in here

// test door thing
// datablock fxDTSBrickData ( brickDoorTestData )
// {
	// brickFile = "./dev/testDoor.blb";
	// category = "special";
	// subCategory = "misc";
	// uiName = "Door Test";
	// iconName = "./placeholder";
	// orientationFix = 1;
// };

%error = ForceRequiredAddOn( "Support_Doors" );

if( %error == $Error::AddOn_NotFound )
{
	error("Brick Doors: Support_Doors is missing somehow, what did you do?");
}
else
{
	exec( "./bricks/door_house.cs" );
	exec( "./bricks/door_glass.cs" );
	exec( "./bricks/door_houseWindow.cs" );
	exec( "./bricks/door_jail.cs" );
	exec( "./bricks/door_plain.cs" );
}