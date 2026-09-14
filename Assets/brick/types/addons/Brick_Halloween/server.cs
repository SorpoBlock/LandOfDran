// exec 2012 halloween stuff
exec( "./2012/server.cs" );

function execHalloween()
{
	exec( "Add-Ons/Brick_Halloween/server.cs" );
}

datablock fxDTSBrickData (brickPumpkinBaseData)
{
	brickFile = "./pumpkin_base.blb";
	category = "special";
	subCategory = "misc";
	uiName = "Pumpkin";
	iconName = "Add-Ons/Brick_Halloween/pumpkin";
};

datablock fxDTSBrickData (brickPumpkinFaceData)
{
	brickFile = "./pumpkin_face.blb";
	uiName = "Pumpkin_Face";
};

datablock fxDTSBrickData (brickPumpkinScaredData)
{
	brickFile = "./pumpkin_scared.blb";
	uiName = "Pumpkin_Scared";
};

datablock fxDTSBrickData (brickPumpkinAsciiData)
{
	brickFile = "./pumpkin_ascii.blb";
	uiName = "Pumpkin_Ascii";
};

datablock fxDTSBrickData (brickGraveStoneData)
{
	brickFile = "./gravestone.blb";
	category = "special";
	subCategory = "misc";
	uiName = "Gravestone";
	iconName = "Add-Ons/Brick_Halloween/gravestone";
};

function carvePumpkin(%obj)
{
	//Pumpkin List
	%a = -1;
	%list[%a++] = "Face";
	%list[%a++] = "Scared";
	%list[%a++] = "Ascii";

	//Randomly Select the pumpkin
	%r = getRandom(0,%a);
	%obj.setDataBlock("brickPumpkin" @ %list[%r] @ "Data");
}

package pumpkinCarving
{
	function fxDTSBrick::onProjectileHit(%obj, %projectile, %client)
	{
      if(isObject(%projectile.sourceObject))
      {
         if(%projectile.sourceObject.getClassName() $= "player" && %obj.getDataBlock().getName() $= "brickPumpkinBaseData" && %projectile.getDataBlock().getName() $= "swordProjectile" && getTrustLevel(%obj, %projectile.sourceObject) >= $TrustLevel::Build)
         {
            carvePumpkin(%obj);
         }
      }
		parent::onProjectileHit(%obj, %projectile, %client);
	}
};
activatePackage(pumpkinCarving);