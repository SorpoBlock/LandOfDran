// halloween 2012 pack

// skulls
datablock fxDTSBrickData ( brickSkullOpenData )
{
	brickFile = "./skull_open.blb";
	uiName = "Skull Open";
	
	orientationFix = 3;
	
	isDoor = 1;
	skipDoorEvents = 1;
	isOpen = 1;
	noBrickSounds = 1;
	
	// we need to know what datablock to switch to when we open & close a door
	closedCW = "brickSkullData";
	openCW = "brickSkullOpenData";
	
	closedCCW = "brickSkullData";
	openCCW = "brickSkullOpenData";
};

datablock fxDTSBrickData ( brickSkullData : brickSkullOpenData )
{
	brickFile = "./skull.blb";
	category = "special";
	subCategory = "misc";
	uiName = "Skull";
	iconName = "Add-Ons/Brick_Halloween/2012/Icon_Skull";
	
	orientationFix = 3;
	
	// door things
	isOpen = 0;
};

function brickSkullData::onPlant( %this, %obj )
{
	// cool check
	if( getRandom( 1, 64 ) == 16 )
	{
		// he's cool
		%obj.setDataBlock( brickSkullCoolData );
		return;
	}
	
	// randomly choose between the four expressions
	%rand = getRandom( 0, 3 );
	
	switch ( %rand )
	{
		case 0: %obj.setDataBlock( brickSkullData );
		case 1: %obj.setDataBlock( brickSkullAngryData );
		case 2: %obj.setDataBlock( brickSkullSadData );
		case 3: %obj.setDataBlock( brickSkullMurrayData );
	}
}

// angry
datablock fxDTSBrickData ( brickSkullAngryOpenData )
{
	brickFile = "./skull_Angry_open.blb";
	uiName = "Skull Angry Open";
	
	orientationFix = 3;
	
	isDoor = 1;
	skipDoorEvents = 1;
	isOpen = 1;
	noBrickSounds = 1;
	
	// we need to know what datablock to switch to when we open & close a door
	closedCW = "brickSkullAngryData";
	openCW = "brickSkullAngryOpenData";
	
	closedCCW = "brickSkullAngryData";
	openCCW = "brickSkullAngryOpenData";
};

datablock fxDTSBrickData ( brickSkullAngryData : brickSkullAngryOpenData )
{
	brickFile = "./skull_angry.blb";
	uiName = "Skull Angry";
	
	orientationFix = 3;
	isOpen = 0;
};

// sad
datablock fxDTSBrickData ( brickSkullSadOpenData )
{
	brickFile = "./skull_sad_open.blb";
	uiName = "Skull Sad Open";
	
	orientationFix = 3;
	
	isDoor = 1;
	skipDoorEvents = 1;
	isOpen = 1;
	noBrickSounds = 1;
	
	// we need to know what datablock to switch to when we open & close a door
	closedCW = "brickSkullSadData";
	openCW = "brickSkullSadOpenData";
	
	closedCCW = "brickSkullSadData";
	openCCW = "brickSkullSadOpenData";
};
datablock fxDTSBrickData ( brickSkullSadData : brickSkullSadOpenData )
{
	brickFile = "./skull_sad.blb";
	uiName = "Skull Sad";
	
	orientationFix = 3;
	isOpen = 0;
};

// murray
datablock fxDTSBrickData ( brickSkullMurrayOpenData )
{
	brickFile = "./skull_murray_open.blb";
	uiName = "Skull Murray Open";
	
	orientationFix = 3;
	
	isDoor = 1;
	skipDoorEvents = 1;
	isOpen = 1;
	noBrickSounds = 1;
	
	// we need to know what datablock to switch to when we open & close a door
	closedCW = "brickSkullMurrayData";
	openCW = "brickSkullMurrayOpenData";
	
	closedCCW = "brickSkullMurrayData";
	openCCW = "brickSkullMurrayOpenData";
};

datablock fxDTSBrickData ( brickSkullMurrayData : brickSkullMurrayOpenData )
{
	brickFile = "./skull_murray.blb";
	uiName = "Skull Murray";
	
	orientationFix = 3;
	isOpen = 0;
};

// cool 
datablock fxDTSBrickData ( brickSkullCoolOpenData )
{
	brickFile = "./skull_cool_open.blb";
	uiName = "Skull Cool Open";
	
	orientationFix = 3;
	
	isDoor = 1;
	skipDoorEvents = 1;
	isOpen = 1;
	noBrickSounds = 1;
	
	// we need to know what datablock to switch to when we open & close a door
	closedCW = "brickSkullCoolData";
	openCW = "brickSkullCoolOpenData";
	
	closedCCW = "brickSkullCoolData";
	openCCW = "brickSkullCoolOpenData";
};

datablock fxDTSBrickData ( brickSkullCoolData : brickSkullCoolOpenData )
{
	brickFile = "./skull_cool.blb";
	uiName = "Skull Cool";
	
	orientationFix = 3;
	isOpen = 0;
};

// coffin

// standing
datablock fxDTSBrickData ( brickCoffinOpenData )
{
	brickFile = "./coffin_open.blb";
	// category = "special";
	// subCategory = "misc";
	uiName = "Coffin Standing";
	
	orientationFix = 1;
	
	// door things
	isDoor = 1;
	isOpen = 1;
	
	// we need to know what datablock to switch to when we open & close a door
	closedCW = "brickCoffinData";
	openCW = "brickCoffinOpenData";
	
	closedCCW = "brickCoffinFalseData";
	openCCW = "brickCoffinOpenData";
	
	// so we can plant inside it
	isWaterBrick = true;
};

datablock fxDTSBrickData ( brickCoffinFalseData : brickCoffinOpenData )
{
	brickFile = "./coffin_falseBack.blb";
	isOpen = 0;
};

datablock fxDTSBrickData ( brickCoffinData : brickCoffinOpenData )
{
	brickFile = "./coffin.blb";
	category = "special";
	subCategory = "misc";
	iconName = "Add-Ons/Brick_Halloween/2012/Icon_Coffin_Standing";
	
	isOpen = 0;
	
	orientationFix = 1;
};

// coffin sitting
datablock fxDTSBrickData ( brickCoffinDownOpenData )
{
	brickFile = "./coffin_down_open.blb";
	uiName = "Coffin";
	
	orientationFix = 2;
	
	// door things
	isDoor = 1;
	isOpen = 1;
	
	// we need to know what datablock to switch to when we open & close a door
	closedCW = "brickCoffinDownData";
	openCW = "brickCoffinDownOpenData";
	
	closedCCW = "brickCoffinDownFalseData";
	openCCW = "brickCoffinDownOpenData";
	
	isWaterBrick = true;
};

datablock fxDTSBrickData ( brickCoffinDownFalseData : brickCoffinDownOpenData )
{
	brickFile = "./coffin_down_falseBack.blb";
	isOpen = 0;
};

datablock fxDTSBrickData ( brickCoffinDownData : brickCoffinDownOpenData )
{
	brickFile = "./coffin_down.blb";
	category = "special";
	subCategory = "misc";
	iconName = "Add-Ons/Brick_Halloween/2012/Icon_Coffin";
	
	isOpen = 0;
};








