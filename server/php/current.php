<?php
	include("database.php"); 	
	$link=Connection();
	$result=mysql_query("SELECT u.id, a.tag, u.first_name, u.last_name, a.main, a.kitchen, a.tool_room, a.simulator, a.storage FROM access a, users u WHERE a.tag = u.tag", $link);
?>
<html>
   <head>
      <title>Current Members</title>
   </head>
<body>
   <h1>Current Members</h1>
   <table border="1" cellspacing="1" cellpadding="1">
		<tr>
			<td>ID</td>
			<td>Tag</td>
			<td>First Name</td>
			<td>Last Name</td>
			<td>Main Door</td>
			<td>Kitchen Door</td>
			<td>Tool Room</td>
			<td>Simulator Room</td>
			<td>Storage Room</td>
		</tr>

      <?php 
		  if($result!==FALSE){
		     while($row = mysql_fetch_array($result)) {
		        printf("<tr><td id='id'>%s</td><td id='tag'>%s</td><td id='first'>%s</td><td id='last'>%s</td><td id='main'>%s</td><td id='kitchen'>%s</td><td id='tool'>%s</td><td id='simulator'>%s</td><td id='storage'>%s</td></tr>", 
		           $row["id"], $row["tag"], $row["first_name"], $row["last_name"], $row["main"], $row["kitchen"], $row["tool_room"], $row["simulator"], $row["storage"]);
		     }
		     mysql_free_result($result);
		     mysql_close();
		  }
      ?>

   </table>
</body>
</html>
