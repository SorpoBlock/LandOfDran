datablock fxDTSBrickData (brick64xCubeData)
{
	brickFile = "./64x Cube.blb";
	category = "Baseplates";
	subCategory = "Cube";
	uiName = "64x Cube";
	iconName = "Add-Ons/Brick_Large_Cubes/64x Cube";
};

datablock fxDTSBrickData (brick32xCubeData)
{
	brickFile = "./32x Cube.blb";
	category = "Baseplates";
	subCategory = "Cube";
	uiName = "32x Cube";
	iconName = "Add-Ons/Brick_Large_Cubes/32x Cube";
};

datablock fxDTSBrickData (brick16xCubeData)
{
	brickFile = "./16x Cube.blb";
	category = "Baseplates";
	subCategory = "Cube";
	uiName = "16x Cube";
	iconName = "Add-Ons/Brick_Large_Cubes/16x Cube";
};

datablock fxDTSBrickData (brick8xCubeData)
{
	brickFile = "./8x Cube.blb";
	category = "Baseplates";
	subCategory = "Cube";
	uiName = "8x Cube";
	iconName = "Add-Ons/Brick_Large_Cubes/8x Cube";
};

datablock fxDTSBrickData (brick4xCubeData)
{
	brickFile = "./4x Cube.blb";
	category = "Baseplates";
	subCategory = "Cube";
	uiName = "4x Cube";
	iconName = "Add-Ons/Brick_Large_Cubes/4x Cube";
};

datablock fxDTSBrickData (brick8xWaterData)
{
	brickFile = "./8x Water.blb";
	category = "Baseplates";
	subCategory = "Water";
	uiName = "8x Water";
	iconName = "Add-Ons/Brick_Large_Cubes/8x Water";
   alwaysShowWireFrame = true;
   isWaterBrick = true;
};

datablock fxDTSBrickData (brick8xWaterRiverData)
{
	brickFile = "./8x River.blb";
	category = "Baseplates";
	subCategory = "Water";
	uiName = "8x River";
	iconName = "Add-Ons/Brick_Large_Cubes/8x River";
   alwaysShowWireFrame = true;
   isWaterBrick = true;
};

datablock fxDTSBrickData (brick8xWaterRapidsData)
{
	brickFile = "./8x Rapids.blb";
	category = "Baseplates";
	subCategory = "Water";
	uiName = "8x Rapids";
	iconName = "Add-Ons/Brick_Large_Cubes/8x Rapids";
   alwaysShowWireFrame = true;
   isWaterBrick = true;
};

datablock fxDTSBrickData (brick32xWaterData)
{
	brickFile = "./32x Water.blb";
	category = "Baseplates";
	subCategory = "Water";
	uiName = "32x Water";
	iconName = "Add-Ons/Brick_Large_Cubes/32x Water";
   alwaysShowWireFrame = true;
   isWaterBrick = true;
};





function fxDTSBrick::createWaterZone(%brick)
{
   %pz = new PhysicalZone()
   {
      isWater = true;
      waterViscosity = 40;
      waterDensity = 1;
      waterColor = "1 0 1 1";
      polyhedron = "0 0 0 1 0 0 0 -1 0 0 0 1";
   };
   if(!isObject(%pz))
   {
      error("ERROR: Could not create physical zone for " @ %brick.getDatablock().getName() @ "brick!");
      return;
   }
   missionCleanup.add(%pz);

   %brick.physicalZone = %pz;

   %brick.onColorChange();

   //set size
   %boxMin = getWords(%brick.getWorldBox(), 0, 2);
   %boxMax = getWords(%brick.getWorldBox(), 3, 5);
   %boxDiff = vectorSub(%boxMax, %boxMin);
   %boxDiff = vectorAdd(%boxDiff, "0 0 0.1"); 
   %pz.setScale(%boxDiff);
      
   //set position - this looks wierd because the the physical zone isn't positioned from it's center
   %posA = %brick.getWorldBoxCenter();
   %posB = %pz.getWorldBoxCenter();
   %posDiff = vectorSub(%posA, %posB);
   %posDiff = vectorSub(%posDiff, "0 0 0.1");
   %pz.setTransform(%posDiff);
}



function brick8xWaterData::onPlant(%data, %brick)
{
   %brick.setColliding(0);
   %brick.setRayCasting(0);
   %brick.setShapeFX(2);

   //if we've planted directly on top of an identical water brick, hide it
   if(!isObject($LoadingBricks_Client))
   {
      %count = %brick.getNumDownBricks();
      for(%i = 0; %i < %count; %i++)
      {        
         %downBrick = %brick.getDownBrick(%i);

         if(!isObject(%downBrick.physicalZone))
            continue;

         if(%downBrick.getDataBlock().brickSizeX != %data.brickSizeX ||
            %downBrick.getDataBlock().brickSizeY != %data.brickSizeY ||
            %downBrick.getDataBlock().brickSizeZ != %data.brickSizeZ )
            continue;

         %pos = %brick.getPosition();
         %downBrickPos = %downBrick.getPosition();

         %xDelta = mAbs(getWord(%pos, 0) - getWord(%downBrickPos, 0));
         %yDelta = mAbs(getWord(%pos, 1) - getWord(%downBrickPos, 1));

         if(  %xDelta > 0.05  ||
              %yDelta > 0.05  )
            continue;

         %downBrick.setRendering(0);
      }
   }
}

function brick8xWaterData::onLoadPlant(%data, %obj)
{
   brick8xWaterData::onPlant(%data, %obj); 
}


function brick8xWaterData::onTrustCheckFinished(%data, %brick)
{
   Parent::onTrustCheckFinished(%data, %brick);
   %brick.createWaterZone();  
}


function brick8xWaterData::onDeath(%data, %brick)
{
   if(isObject(%brick.physicalZone))
      %brick.physicalZone.delete();
   
   //unhide water bricks that are directly under this brick
   %count = %brick.getNumDownBricks();
   for(%i = 0; %i < %count; %i++)
   {        
      %downBrick = %brick.getDownBrick(%i);

      if(!isObject(%downBrick.physicalZone))
         continue;

      if(%downBrick.getDataBlock().brickSizeX != %data.brickSizeX ||
         %downBrick.getDataBlock().brickSizeY != %data.brickSizeY ||
         %downBrick.getDataBlock().brickSizeZ != %data.brickSizeZ )
         continue;

      %pos = %brick.getPosition();
      %downBrickPos = %downBrick.getPosition();

      %xDelta = mAbs(getWord(%pos, 0) - getWord(%downBrickPos, 0));
      %yDelta = mAbs(getWord(%pos, 1) - getWord(%downBrickPos, 1));

      if(  %xDelta > 0.05  ||
           %yDelta > 0.05  )
         continue;

      %downBrick.setRendering(1);
   }

}

function brick8xWaterData::onRemove(%data, %brick)
{
   if(isObject(%brick.physicalZone))
      %brick.physicalZone.delete();
}

function brick8xWaterData::onColorChange(%data, %brick)
{
   Parent::onColorChange(%data, %brick);
   %color = getColorIDTable(%brick.colorID);
   %rgb = getWords(%color, 0, 2);
   %a = getWord(%color, 3) * 0.75;

   if(isObject(%brick.physicalZone))
      %brick.physicalZone.setWaterColor(%rgb SPC %a);
}

function brick8xWaterData::onFakeDeath(%data, %brick)
{
   Parent::onFakeDeath(%data, %brick);
   if(isObject(%brick.physicalZone))
      %brick.physicalZone.deactivate();
}

function brick8xWaterData::onClearFakeDeath(%data, %brick)
{
   Parent::onClearFakeDeath(%data, %brick);
   if(isObject(%brick.physicalZone))
      %brick.physicalZone.activate();

   %brick.setColliding(0);
   %brick.setRayCasting(0);
}

function brick8xWaterData::disappear(%data, %brick, %time)
{
   Parent::disappear(%data,%brick,%time);
   if(%time == 0)
   {
      //disappear(0) = instantly re-appear
      if(isObject(%brick.physicalZone))
         %brick.physicalZone.activate();
      %brick.setColliding(0);
      %brick.setRayCasting(0);
   }
   else
   {
      if(isObject(%brick.physicalZone))
         %brick.physicalZone.deactivate();
   }
}

function brick8xWaterData::reappear(%data, %brick)
{
   Parent::reappear(%data,%brick);

   if(isObject(%brick.physicalZone))
      %brick.physicalZone.activate();

   %brick.setColliding(0);
   %brick.setRayCasting(0);
}



// River Water //
/////////////////
function brick8xWaterRiverData::onPlant(%data, %brick)
{
   brick8xWaterData::onPlant(%data, %brick);
}

function brick8xWaterRiverData::onLoadPlant(%data, %obj)
{
   brick8xWaterRiverData::onPlant(%data, %obj); 
}

function brick8xWaterRiverData::onTrustCheckFinished(%data, %brick) 
{
   Parent::onTrustCheckFinished(%data, %brick);
   %brick.createWaterZone();  
   %brick.physicalZone.setAppliedForce(vectorScale(%brick.getForwardVector(), 1000));
}

function brick8xWaterRiverData::onDeath(%data, %brick)
{
   brick8xWaterData::onDeath(%data, %brick);
}

function brick8xWaterRiverData::onRemove(%data, %brick)
{
   brick8xWaterData::onRemove(%data, %brick);
}

function brick8xWaterRiverData::onColorChange(%data, %brick)
{
   brick8xWaterData::onColorChange(%data, %brick);
}

function brick8xWaterRiverData::onFakeDeath(%data, %brick)
{
   brick8xWaterData::onFakeDeath(%data, %brick);
}

function brick8xWaterRiverData::onClearFakeDeath(%data, %brick)
{
   brick8xWaterData::onClearFakeDeath(%data, %brick);
}

function brick8xWaterRiverData::disappear(%data, %brick, %time)
{
   brick8xWaterData::disappear(%data, %brick, %time);
}

function brick8xWaterRiverData::reappear(%data, %brick)
{
   brick8xWaterData::reappear(%data, %brick);
}
/////////////////


// Rapids //
////////////
function brick8xWaterRapidsData::onPlant(%data, %brick)
{
   brick8xWaterData::onPlant(%data, %brick);
}

function brick8xWaterRapidsData::onLoadPlant(%data, %obj)
{
   brick8xWaterRapidsData::onPlant(%data, %obj); 
}

function brick8xWaterRapidsData::onTrustCheckFinished(%data, %brick)
{
   Parent::onTrustCheckFinished(%data, %brick);
   %brick.createWaterZone();  
   %brick.physicalZone.setAppliedForce(vectorScale(%brick.getForwardVector(), 3000));
}

function brick8xWaterRapidsData::onDeath(%data, %brick)
{
   brick8xWaterData::onDeath(%data, %brick);
}

function brick8xWaterRapidsData::onRemove(%data, %brick)
{
   brick8xWaterData::onRemove(%data, %brick);
}

function brick8xWaterRapidsData::onColorChange(%data, %brick)
{
   brick8xWaterData::onColorChange(%data, %brick);
}

function brick8xWaterRapidsData::onFakeDeath(%data, %brick)
{
   brick8xWaterData::onFakeDeath(%data, %brick);
}

function brick8xWaterRapidsData::onClearFakeDeath(%data, %brick)
{
   brick8xWaterData::onClearFakeDeath(%data, %brick);
}

function brick8xWaterRapidsData::disappear(%data, %brick, %time)
{
   brick8xWaterData::disappear(%data, %brick, %time);
}

function brick8xWaterRapidsData::reappear(%data, %brick)
{
   brick8xWaterData::reappear(%data, %brick);
}
////////////


// 32x Water //
///////////////
function brick32xWaterData::onPlant(%data, %brick)
{
   brick8xWaterData::onPlant(%data, %brick);
}

function brick32xWaterData::onLoadPlant(%data, %obj)
{
   brick32xWaterData::onPlant(%data, %obj); 
}

function brick32xWaterData::onTrustCheckFinished(%data, %brick)
{
   Parent::onTrustCheckFinished(%data, %brick);
   %brick.createWaterZone();  
}

function brick32xWaterData::onDeath(%data, %brick)
{
   brick8xWaterData::onDeath(%data, %brick);
}

function brick32xWaterData::onRemove(%data, %brick)
{
   brick8xWaterData::onRemove(%data, %brick);
}

function brick32xWaterData::onColorChange(%data, %brick)
{
   brick8xWaterData::onColorChange(%data, %brick);
}

function brick32xWaterData::onFakeDeath(%data, %brick)
{
   brick8xWaterData::onFakeDeath(%data, %brick);
}

function brick32xWaterData::onClearFakeDeath(%data, %brick)
{
   brick8xWaterData::onClearFakeDeath(%data, %brick);
}

function brick32xWaterData::disappear(%data, %brick, %time)
{
   brick8xWaterData::disappear(%data, %brick, %time);
}

function brick32xWaterData::reappear(%data, %brick)
{
   brick8xWaterData::reappear(%data, %brick);
}
///////////////