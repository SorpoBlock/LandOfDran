datablock fxDTSBrickData (brickTeledoorData)
{
	brickFile = "Add-Ons/Brick_Teledoor/teledoor.blb";
	category = "Special";     
	subCategory = "Interactive";
	uiName = "Teledoor";                  
	iconName = "Add-Ons/Brick_Teledoor/Teledoor";
   orientationFix = 2;
   isTeledoor = 1;
};


function brickTeledoorData::onPlant(%data, %obj)
{
   %client = %obj.client;

   //help the client plant a matching entrance/exit pair
   if(%client.currTeledoorName $= "")
   {
      %name = "Teledoor_" @ getSubStr(sha1(%obj.getTransform()), 0, 5);
      %client.currTeledoorName = %name;
   }
   %obj.setNTObjectName(%client.currTeledoorName);
   %obj.enableTouch = true;
}

function brickTeledoorData::onLoadPlant(%data, %obj)
{
   %obj.enableTouch = true;
}

package TeledoorPackage
{
   function ServerCmdCancelBrick(%client)
   {
      Parent::ServerCmdCancelBrick(%client);
      %client.currTeledoorName = "";
   }
};
activatePackage(TeledoorPackage);


function brickTeledoorData::onTrustCheckFinished(%data, %brick) 
{
   %brick.trustCheckFinished = 1;
}

function brickTeledoorData::onPlayerTouch(%data, %obj, %player)
{
   //do normal game stuff
   Parent::onPlayerTouch(%data, %obj, %player);

   //if trust check has not finished, don't go through
   if(%obj.trustCheckFinished != 1)
      return;

   if(getSimTime() - %player.lastTeledoorTime < 30)
      return;
   %player.lastTeledoorTime = getSimTime();
   
   //get nt object array stuff
   %group = %obj.getGroup();
   %name  = %obj.getName(); 
   %count = %group.NTObjectCount[%name];
   
   //no other bricks of this name, early out
   if(%count <= 1)
      return;

   //find ourselves in ntobject array, go forward from there
   %ourIdx = -1;
   for(%i = 0; %i < %count; %i++)
   {
      %targetObj = %group.NTObject[%name, %i];
      if(%targetObj == %obj)
      {
         %ourIdx = %i;
         break;
      }
   }
   if(%ourIdx == -1)
   {
      error("ERROR: brickTeledoorData::onPlayerTouch(" @ %data @ ", " @ %obj @ ", " @ %player @ ") - could not find ourselves in named target list");
      return;
   }

   //get the next door   
   %targetIdx = %ourIdx + 1;
   %targetIdx = %targetIdx % %count;
   %targetBrick = %group.NTObject[%name, %targetIdx];

   if(%targetBrick.getDatablock().isTeledoor != true)
   {
      //echo("not a teledoor");
      //user has named other bricks the same thing as their teledoor
      //go forward until we find a teledoor 
      for(%i = 0; %i < %count; %i++)
      {
         %testIdx = (%targetIdx + %i) % %count;
         %targetBrick = %group.NTObject[%name, %testIdx];
         if(%targetBrick.getDatablock().getId() == brickTeledoorData.getId() && %targetBrick != %obj)
            break;
      }

      //did we find anything?
      if(%targetBrick.getDatablock().getId() != brickTeledoorData.getId())
         return;
   }

   //teleport player   
   %zOffset = (%targetBrick.getDataBlock().brickSizeZ * 0.2 / 2) - 0.1;
   %pos = vectorSub(%targetBrick.getPosition(), "0 0 " @ %zOffset);

   %vel = %player.getVelocity();
   %velX = getWord(%vel, 0);
   %velY = getWord(%vel, 1);
   %velZ = getWord(%vel, 2);

   %teleportOffset = 0.5 + (%player.getDataBlock().boundingBox / 4) / 2;

   switch(%targetBrick.getAngleId())
   {
      case 0:
         %pos = vectorAdd(%pos, %teleportOffset @ " 0 0");
         %rot = "0 0 1 1.57";

      case 1:
         %pos = vectorAdd(%pos, "0 " @ -%teleportOffset @ " 0");
         %rot = "0 0 1 3.1415";
      case 2: 
         %pos = vectorAdd(%pos, -%teleportOffset @ " 0 0");
         %rot = "0 0 1 -1.57";

      case 3:
         %pos = vectorAdd(%pos, "0 " @ %teleportOffset @ " 0");
         %rot = "0 0 1 0";
   }

   %angleDelta = %targetBrick.getAngleId() - %obj.getAngleId();
   if(%angleDelta < 0)
      %angleDelta += 4;

   %outX = 0;
   %outY = 0;
   switch(%angleDelta)
   {
      case 0:
         %outX = -%velX;
         %outY = -%velY;
         %rotDelta = 3.1415;

      case 1:
         %outX = -%velY;
         %outY =  %velX;
         %rotDelta = -1.57;

      case 2:
         %outX =  %velX;
         %outY =  %velY;
         %rotDelta = 0;

      case 3:
         %outX =  %velY;
         %outY = -%velX;
         %rotDelta = 1.57;
   }

   //if you're moving down you can get stuck in the ground :/
   %velZ = 0;

   %player.setVelocity(%outX SPC %outY SPC %velZ);

   %rotVec = getWord(%player.getTransform(), 5);
   %rotAng = getWord(%player.getTransform(), 6);
   %rotAng += (%rotDelta * %rotVec);

   %player.setTransform(%pos SPC "0 0" SPC %rotVec SPC %rotAng);

   //fire teleportation event on exit brick
   %obj.onTeledoorEnter(%player);
   %targetBrick.onTeledoorExit(%player);   
}


//events
registerInputEvent("fxDTSBrick", "onTeledoorEnter",  "Self fxDTSBrick" TAB 
                                                     "Player Player" TAB 
                                                     "Client GameConnection" TAB
                                                     "MiniGame MiniGame");

registerInputEvent("fxDTSBrick", "onTeledoorExit",   "Self fxDTSBrick" TAB 
                                                     "Player Player" TAB 
                                                     "Client GameConnection" TAB
                                                     "MiniGame MiniGame");



function fxDTSBrick::onTeledoorEnter(%obj, %player)
{
   %data = %obj.getDataBlock();
   %data.onTeledoorEnter(%obj,%player);
}

function fxDTSBrickData::onTeledoorEnter(%data, %obj, %player)
{ 
   //prevent function call spam
   if(%obj.numEvents <= 0) 
      return;

   //immune from ontouch events when holding destructo wand
   %image = %player.getMountedImage(0);
   if(%image)
      if(%image.getId() == AdminWandImage.getId())
         return;

   %client = %player.client;

   $InputTarget_["Self"]   = %obj;
   $InputTarget_["Player"] = %player;
   $InputTarget_["Client"] = %client;

   if($Server::LAN)
   {
      $InputTarget_["MiniGame"] = getMiniGameFromObject(%client);
   }
   else
   {
      if(getMiniGameFromObject(%obj) == getMiniGameFromObject(%client))
         $InputTarget_["MiniGame"] = getMiniGameFromObject(%obj);
      else
         $InputTarget_["MiniGame"] = 0;
   }

   %obj.processInputEvent("onTeledoorEnter", %client);
}



function fxDTSBrick::onTeledoorExit(%obj, %player)
{
   %data = %obj.getDataBlock();
   %data.onTeledoorExit(%obj,%player);
}

function fxDTSBrickData::onTeledoorExit(%data, %obj, %player)
{ 
   //prevent function call spam
   if(%obj.numEvents <= 0) 
      return;

   //immune from ontouch events when holding destructo wand
   %image = %player.getMountedImage(0);
   if(%image)
      if(%image.getId() == AdminWandImage.getId())
         return;

   %client = %player.client;

   $InputTarget_["Self"]   = %obj;
   $InputTarget_["Player"] = %player;
   $InputTarget_["Client"] = %client;

   if($Server::LAN)
   {
      $InputTarget_["MiniGame"] = getMiniGameFromObject(%client);
   }
   else
   {
      if(getMiniGameFromObject(%obj) == getMiniGameFromObject(%client))
         $InputTarget_["MiniGame"] = getMiniGameFromObject(%obj);
      else
         $InputTarget_["MiniGame"] = 0;
   }

   %obj.processInputEvent("onTeledoorExit", %client);
}