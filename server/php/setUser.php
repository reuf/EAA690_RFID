<?php
   	include("database.php");
   	$link=Connection();
	$id=$_POST["id"];
	$tag=$_POST["tag"];
	$main=$_POST["main"];
	$kitchen=$_POST["kitchen"];
	$tool=$_POST["tool"];
	$simulator=$_POST["simulator"];
	$storage=$_POST["storage"];
        $entryArray = explode(',', $entry);

        $result = mysql_query("SELECT id FROM users WHERE id = '".$id."'", $link);
        if ($result !== FALSE) {
    	  $query = "UPDATE users SET tag = '".$tag."' WHERE id = '".$id."';
   	  mysql_query($query, $link);
        } else { // Add User?
    	  //$query = "INSERT INTO users SET tag = '".$tag."' WHERE id = '".$id."';
   	  //mysql_query($query, $link);
        }
        $result = mysql_query("SELECT id FROM access WHERE tag = '".$tag."'", $link);
        if ($result !== FALSE) {
    	  $query = "UPDATE access SET main = '".$main."', kitchen = '".$kitchen."', tool_room = '".$tool."', simulator = '".$simulator."', storage = '".$storage."' WHERE tag = '".$tag."';
   	  mysql_query($query, $link);
        } else {
    	  $query = "INSERT INTO access (tag, main, kitchen, tool_room, simulator, storage) VALUES ('".$tag."', '".$main."', '".$kitchen."', '".$tool."', '".$simulator."', '".$storage."');
   	  mysql_query($query, $link);
        }
	mysql_close($link);
   	header("Location: current.php");
?>
