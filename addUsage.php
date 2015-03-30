<?php
   	include(“database.php");
   	
   	$link=Connection();

	$entry=$_POST[“entry”];
        $entryArray = explode(',', $entry);

        if (sizeof($entryArray) == 4) {
    	  $query = "INSERT INTO `door_usage` (`time`, `tag`, `door`, `access`) 
	    	  VALUES ('".$entryArray[0].”’,’”.$entryArray[1].”’,’”.$entryArray[2].”’,’”.$entryArray[3].”’)”; 
   	
   	  mysql_query($query,$link);
	  mysql_close($link);
        }
   	header("Location: doorUsage.php");
?>
