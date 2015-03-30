<?php
   	include(“database.php");
   	
   	$link=Connection();

	$entry=$_POST[“entry”];
        $entryArray = explode(',', $entry);

        if (sizeof($entryArray) == 4) {
          //INSERT INTO  `eaa690`.`door_usage` (`id` ,`time` ,`tag` ,`door` ,`access`) VALUES ('1',  '2015-03-30 03:23:11',  '23FR4576DF34',  '1',  '1');
          //INSERT INTO  `eaa690`.`door_usage` (`time` ,`tag` ,`door` ,`access`) VALUES ('2015-03-30 03:23:11',  '23FR4576DF34',  '1',  '1');

    	  $query = "INSERT INTO `door_usage` (`time`, `tag`, `door`, `access`) 
	    	  VALUES ('".$entryArray[0].”’,’”.$entryArray[1].”’,’”.$entryArray[2].”’,’”.$entryArray[3].”’)”; 
   	
   	  mysql_query($query,$link);
	  mysql_close($link);
        }
   	header("Location: doorUsage.php");
?>
