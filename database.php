<?php

	function Connection(){
		$server="mysql";
		$user="eaa690";
		$pass="eaa690";
		$db="eaa690";
	   	
		$connection = mysql_connect($server, $user, $pass);

		if (!$connection) {
	    	  die('MySQL ERROR: ' . mysql_error());
		}
		
		mysql_select_db($db) or die( 'MySQL ERROR: '. mysql_error() );

		return $connection;
	}
?>
