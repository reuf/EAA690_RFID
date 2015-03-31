<?php

	include("database.php"); 	
	
	$link=Connection();

	$result=mysql_query("SELECT `time`, `tag`, `door`, `access` FROM `door_usage` ORDER BY `time` DESC",$link);
?>

<html>
   <head>
      <title>Door Usage Data</title>
   </head>
<body>
   <h1>Access</h1>

   <table border="1" cellspacing="1" cellpadding="1">
		<tr>
			<td>Timestamp</td>
			<td>Tag</td>
			<td>Door</td>
			<td>Access</td>
		</tr>

      <?php 
		  if($result!==FALSE){
		     while($row = mysql_fetch_array($result)) {
		        printf("<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>", 
		           $row["time"], $row["tag"], $row["door"], $row["access"]);
		     }
		     mysql_free_result($result);
		     mysql_close();
		  }
      ?>

   </table>
</body>
</html>
