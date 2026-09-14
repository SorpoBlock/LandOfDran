datablock fxDTSBrickData (brickTreasureChestOpenData)
{
	brickFile = "Add-Ons/Brick_Treasure_Chest/chestOpen.blb";
	category = "";       //no ui names because we don't want people to plant this directly, it's only used for effect
	subCategory = "";
	uiName = "Treasure Chest";                  
	iconName = "";
   orientationFix = 3;
};

//a bit of trickery here, we have to declare this brick last because it is the one we want to create from bricks saved as the "Treasure Chest" uiname
datablock fxDTSBrickData (brickTreasureChestData)
{
	brickFile = "Add-Ons/Brick_Treasure_Chest/chest.blb";
	category = "Special";
	subCategory = "Interactive";
	uiName = "Treasure Chest";
	iconName = "Add-Ons/Brick_Treasure_Chest/Treasure Chest";
   orientationFix = 3;
};


function brickTreasureChestData::onPlant(%data, %obj)
{
   Parent::onPlant(%data, %obj);

   registerTreasureChest(%obj);   

   //add default events
   %enabled     = 1;
   %delay       = 0;
   %inputEvent  = "OnActivate";
   %target      = "Client";
   %outputEvent = "PlaySound";
   %par1        = rewardSound.getId();
   %obj.addEvent(%enabled, %delay, %inputEvent, %target, %outputEvent, %par1);
}


function brickTreasureChestData::onLoadPlant(%data, %obj)
{
   Parent::onLoadPlant(%data, %obj);

   //register this chest 
   registerTreasureChest(%obj);  
}


function brickTreasureChestData::onRemove(%data, %obj)
{
   if(%obj.isPlanted())
      unRegisterTreasureChest(%obj);
   Parent::onRemove(%data, %obj);
}


function brickTreasureChestData::onDeath(%data, %obj)
{
   if(%obj.isPlanted())
      unRegisterTreasureChest(%obj);
   Parent::onDeath(%data, %obj);
}


function registerTreasureChest(%obj)
{
   //get hash of this chest
   %hash = sha1(%obj.getPosition() @ %obj.angleId);
   
   //echo("registering treasurechest "@ %hash);

   //see if chest is already registered - this should never happen
   if(%obj.isRegisteredTreasureChest)
      return;
   for(%i = 0; %i < $TreasureChest::NumChests; %i++)
   {
      if($TreasureChest::ChestHash[%i] $= %hash)
      {
         error("ERROR: registerTreasureChest - chest " @ %obj.getid() @ " already registered");
         %obj.isRegisteredTreasureChest = true;
         return;
      }
   }

   %obj.isRegisteredTreasureChest = true;

   //convert null to 0
   $TreasureChest::NumChests = mFloor($TreasureChest::NumChests); 

   $TreasureChest::ChestHash[$TreasureChest::NumChests] = %hash;
   $TreasureChest::ChestId[%hash] = %obj.getid();
   $TreasureChest::NumChests++;

   //echo(" numchests " @ $TreasureChest::NumChests);
}


function unRegisterTreasureChest(%obj)
{
   //get hash of this chest
   %hash = sha1(%obj.getPosition() @ %obj.angleId);

   if(!%obj.isRegisteredTreasureChest)
      return;

   //echo("unregistering treasurechest "@ %hash);
   
   %obj.isRegisteredTreasureChest = false;
   for(%i = 0; %i < $TreasureChest::NumChests; %i++)
   {
      if($TreasureChest::ChestHash[%i] !$= %hash)
         continue;
      
      $TreasureChest::ChestHash[%i] = $TreasureChest::ChestHash[$TreasureChest::NumChests-1];
      deleteVariables("$TreasureChest::ChestHash" @ $TreasureChest::NumChests-1);
      $TreasureChest::NumChests--;
      break;
   }

   //echo(" numchests " @ $TreasureChest::NumChests);

   deleteVariables("$TreasureChest::ChestId" @ %hash);

   //go through clients and have them clear this treasure chest hash out of their personal lists
   %count = clientGroup.getCount();
   for(%i = 0; %i < %count; %i++)
   {
      %cl = clientGroup.getObject(%i);
      %cl.removeTreasureChestHash(%hash);
   }
}

function dumpTreasureChests()
{
   echo($TreasureChest::NumChests @ " treasure chests");
   for(%i = 0; %i < $TreasureChest::NumChests; %i++)
   {
      %hash = $TreasureChest::ChestHash[%i];
      %id = $TreasureChest::ChestId[%hash];
      echo(%i @ " " @ %id @ " " @ %hash);
   }
   echo("");
}

package TreasureChestPackage
{
   function fxDTSBrick::OnActivate(%obj, %player, %client, %pos, %vec)
   {  
      %data = %obj.getDataBlock();

      if(%data.getid() == brickTreasureChestData.getId())
      {
         %hash = sha1(%obj.getPosition() @ %obj.angleId);

         if(!%client.foundTreasureChest_[%hash])
         {         
            %obj.openTreasureChest(%player);
            Parent::OnActivate(%obj, %player, %client, %pos, %vec);
         }
         else
         {
            commandToClient(%client, 'BottomPrint', "\c3You already opened this treasure chest (" @ %client.numFoundTreasureChests @ " / " @ $TreasureChest::NumChests @ " found)", 2);
         }
      }
      else if(%data.getid() == brickTreasureChestOpenData.getId())
      {
         //don't do on activate events
      }
      else
      {
         Parent::OnActivate(%obj, %player, %client, %pos, %vec);
      }
   }  

   function ServerLoadSaveFile_End()
   {
      Parent::ServerLoadSaveFile_End();
      rebuildAllClientTreasureChestLists();
   }
};
activatePackage(TreasureChestPackage);


function fxDTSBrick::openTreasureChest(%obj, %player)
{
   %data = %obj.getDataBlock();
   %data.openTreasureChest(%obj, %player);
}


function fxDTSBrickData::openTreasureChest(%data ,%obj, %player)
{
   //do nothing by default
}


function brickTreasureChestData::openTreasureChest(%data ,%obj, %player)
{
   %client = %player.client;
   if(!isObject(%client))
      return;
   
   //open
   %obj.setDataBlock(brickTreasureChestOpenData);

   //schedule close on global quota
   %oldQuotaObject = getCurrentQuotaObject();
   if(isObject(%oldQuotaObject))
      clearCurrentQuotaObject();

   %obj.schedule(2000, setDataBlock, brickTreasureChestData);
   %obj.schedule(2000, playSound, BrickChangeSound);

   if(isObject(%oldQuotaObject))
      setCurrentQuotaObject(%oldQuotaObject);
   
   //mark this treasure chest as opened for this client
   %hash = sha1(%obj.getPosition() @ %obj.angleId);
   %client.foundTreasureChest_[%hash] = 1;  //so we can do direct checks
   %client.numFoundTreasureChests = mFloor(%client.numFoundTreasureChests);
   %client.foundTreasureChestList[%client.numFoundTreasureChests] = %hash; //so we can iterate through 
   %client.numFoundTreasureChests++;

   //%client.dumpTreasureChests();

   //tell client how many they've found
   %player.emote(winStarProjectile, 1);
   if(%client.numFoundTreasureChests >= $TreasureChest::NumChests)
   {
      if($TreasureChest::NumChests == 1)
      {
         commandToClient(%client, 'BottomPrint', "\c3You have found the treasure chest!", 2);
      }
      else
      {
         commandToClient(%client, 'BottomPrint', "\c3You have found all " @ $TreasureChest::NumChests @ " treasure chests!", 2);

         //tell everyone, limit this to prevent spam by finding/destroying/rebuilding last chest
         %elapsedtime = getSimTime() - %client.treasureVictoryTime;
         if(%elapsedTime > 10000)
         {
            %client.treasureVictoryTime = getSimTime();
            messageAll('', "\c3" @ %client.getPlayerName() @ "\c0 found all \c6" @ $TreasureChest::NumChests @ "\c0 treasure chests!");
            commandToClient(%client, 'BottomPrint', "\c3You have found all " @ $TreasureChest::NumChests @ " treasure chests!", 2);
         }
      }
   }
   else
   {
      commandToClient(%client, 'BottomPrint', "\c3You have found " @ %client.numFoundTreasureChests @ " / " @ $TreasureChest::NumChests @ " treasure chests!", 2);
   }
}


function GameConnection::removeTreasureChestHash(%client, %hash)
{
   //if we haven't found this chest, there's nothing to remove
   if(!%client.foundTreasureChest_[%hash])
   {
      //echo("client hasn't found that treasure chest yet");
      return;
   }

   %client.foundTreasureChest_[%hash] = "";
   for(%i = 0; %i < %client.numFoundTreasureChests; %i++)
   {
      if(%client.foundTreasureChestList[%i] !$= %hash)
         continue;

      %client.foundTreasureChestList[%i] = %client.foundTreasureChestList[%client.numFoundTreasureChests-1];
      %client.foundTreasureChestList[%client.numFoundTreasureChests] = "";
      %client.numFoundTreasureChests--;
      break;
   }  

   //%client.dumpTreasureChests();
}


function GameConnection::rebuildTreasureChestList(%client)
{
   //rebuild the client's found list based on the hashes they have found vs registered chest hashes
   //probably want to call this on persistence load, or brick load
   //there is a possibility that a brick will be rebuilt in the exact position as one you had previously found and your count will be off, but reconnecting will fix this

   //clear current list
   for(%i = 0; %i < %client.numFoundTreasureChests; %i++)
   {
      %client.foundTreasureChestList[%i] = "";
   }
   %client.numFoundTreasureChests = 0;

   //go through all registered chests and see if we have found their hash
   for(%i = 0; %i < $TreasureChest::NumChests; %i++)
   {
      %hash = $TreasureChest::ChestHash[%i];

      if(!%client.foundTreasureChest_[%hash])
         continue;

      //we have found this hash, add it to the list
      %client.foundTreasureChestList[%client.numFoundTreasureChests] = %hash;
      %client.numFoundTreasureChests++;
   }
}


function rebuildAllClientTreasureChestLists()
{
   %count = clientGroup.getCount();
   for(%i = 0; %i < %count; %i++)
   {
      %client = clientGroup.getObject(%i);
      %client.rebuildTreasureChestList();
   }
}


//debug function
function GameConnection::dumpTreasureChests(%client)
{
   echo(%client.getPlayerName() @ " has " @ %client.numFoundTreasureChests @ " treasure chests");
   for(%i = 0; %i < %client.numFoundTreasureChests; %i++)
   {
      echo("  " @ %i @ ":  " @ %client.foundTreasureChestList[%i]);
   }
}


function serverCmdTreasureStatus(%client)
{
   //tell client how many treasure chests they have found
   if($TreasureChest::NumChests == 1)
   {
      if(%client.numFoundTreasureChests >= 1)
         commandToClient(%client, 'BottomPrint', "\c3You already found the treasure chest", 2);
      else
         commandToClient(%client, 'BottomPrint', "\c3You haven't found the treasure chest yet", 2);
   }
   else
   {
      if(%client.numFoundTreasureChests >= $TreasureChest::NumChests)
         commandToClient(%client, 'BottomPrint', "\c3You have found all " @ $TreasureChest::NumChests @ " treasure chests!", 2);   
      else
         commandToClient(%client, 'BottomPrint', "\c3You have found " @ %client.numFoundTreasureChests @ " of " @ $TreasureChest::NumChests @ " treasure chests!", 2);   
   }
}


//administrative function to teleport through the chests
function serverCmdNextChest(%client, %arg)
{
   if(!%client.isAdmin)
      return;

   if($TreasureChest::NumChests <= 0)
      messageClient(%client, '', "There aren't any treasure chests");

   if(%arg !$= "")
      %client.currTeleportChest = %arg;

   %client.currTeleportChest = mFloor(%client.currTeleportChest);
   if(%client.currTeleportChest >= $TreasureChest::NumChests)
      %client.currTeleportChest = 0;

   %hash = $TreasureChest::ChestHash[%client.currTeleportChest];
   %chest = $TreasureChest::ChestId[%hash];

   if(!isObject(%chest))
   {
      messageClient(%client, '', "ERROR: Chest ID '" @ %chest @ "' for hash '" @ %hash @ "' is not an object");
      %client.currTeleportChest++;
      return;
   }

   commandToClient(%client, 'BottomPrint', "Chest " @ %client.currTeleportChest @ " / " @ $TreasureChest::NumChests, 3 );

   %trans = %chest.getTransform();
   %controlObj = %client.getControlObject();
   %controlObj.setTransform(%trans);

   %client.currTeleportChest++;
}


// persistence hooks //
///////////////////////
LoadRequiredAddOn("Support_Player_Persistence");

if(isFunction(RegisterPersistenceVar))
{
   //just save the hashes we have found, we will rebuild the list on load
   //RegisterPersistenceVar(string variableName, bool matchAll, string datablockClass);
   RegisterPersistenceVar("foundTreasureChest_", true, "");    
}


package TreasureChestPersistencePackage
{
   function GameConnection::applyPersistence(%client, %gotPlayer, %gotCamera)
   {
      Parent::applyPersistence(%client, %gotPlayer, %gotCamera);
      %client.rebuildTreasureChestList();
   }
};
activatePackage(TreasureChestPersistencePackage);

