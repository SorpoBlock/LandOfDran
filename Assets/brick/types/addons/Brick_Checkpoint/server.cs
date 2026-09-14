datablock fxDTSBrickData (brickCheckpointData){
	brickFile = "Add-Ons/Brick_Checkpoint/Checkpoint.blb";

	specialBrickType = "Checkpoint";

	orientationFix = 1;

	canCover = false;

	category = "Special";
	subCategory = "Interactive";
	uiName = "Checkpoint";
	iconName = "Add-Ons/Brick_Checkpoint/Checkpoint";

   indestructable = true;
};

function brickCheckpointData::onPlant(%data, %obj)
{
   %obj.enableTouch = true;

   %enabled     = 1;
   %delay       = 0;
   %inputEvent  = "OnPlayerTouch";
   %target      = "Self";
   %outputEvent = "PlaySound";
   %par1        = Beep_Popup_Sound.getId();
   %obj.addEvent(%enabled, %delay, %inputEvent, %target, %outputEvent, %par1);
}

function brickCheckpointData::onLoadPlant(%data, %obj)
{
   Parent::onLoadPlant(%data, %obj);
   %obj.enableTouch = true;
}

function brickCheckpointData::onPlayerTouch(%data, %obj, %player)
{
   %client = %player.client;
   if(!isObject(%client))
      return;

   if(%client.checkPointBrick != %obj)
   {
      %client.checkPointBrick = %obj;
      %client.checkPointBrickPos = %obj.getPosition();
      commandToClient(%client, 'BottomPrint', "\c4Checkpoint reached! \c7- Say /clearCheckpoint to go back to the beginning", 3);

      Parent::onPlayerTouch(%data, %obj, %player);
   }
}

function serverCmdClearCheckpoint(%client)
{
   if(isObject(%client.checkPointBrick))
   {
      %client.checkPointBrick = "";
      %client.checkPointBrickPos = "";

      messageClient(%client, '', '\c3Checkpoint reset');
      %client.instantRespawn();
   }
}

//persistence hooks
LoadRequiredAddOn("Support_Player_Persistence");

if(isFunction(RegisterPersistenceVar))
{
   RegisterPersistenceVar("checkPointBrickPos", false, "");    
}


package CheckpointPackage
{
   function GameConnection::getSpawnPoint(%client)
   {
      if(isObject(%client.checkPointBrick))
         return (%client.checkPointBrick.getSpawnPoint());
         
      return Parent::getSpawnPoint(%client);   
   }

   function GameConnection::applyPersistence(%client, %gotPlayer, %gotCamera)
   {
      Parent::applyPersistence(%client, %gotPlayer, %gotCamera);
      
      //attempt to recover checkpoint brick from checkpoint position
      if(%client.checkPointBrickPos $= "")
         return;

      %pos = %client.checkPointBrickPos;
      %box = "0.1 0.1 0.1";
      %mask = $TypeMasks::FxBrickAlwaysObjectType;
      InitContainerBoxSearch(%pos, %box, %mask);

      while (%checkBrick = containerSearchNext())
      {	
         if(%checkBrick.getDataBlock().getName() !$= "brickCheckpointData")
            continue;

         %client.checkpointBrick = %checkBrick;
         break;
      }

   }  
};
activatePackage(CheckpointPackage);


