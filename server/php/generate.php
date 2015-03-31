<?php
    include("database.php");
    $link=Connection();
    $result=mysql_query("SELECT 'tag', 'main', 'kitchen', 'tool_room', 'simulator', 'storage' FROM 'access'", $link);
    $dbfile = fopen("database.csv", "w") or die("Unable to open file!");
    if($result!==FALSE){
        fwrite($dbfile, "RFID,Main Door,Kitchen Door,Tool Room,Simulator Room,Storage Room");
        while($row = mysql_fetch_array($result)) {
            fwrite($dbfile, "%s,%s,%s,%s,%s,%s", $row["tag"], $row["main"], $row["kitchen"], $row["tool_room"], $row["simulator"], $row["storage"]);
        }
        mysql_free_result($result);
        mysql_close();
        fclose($myfile);
    }
?>
<html>
   <head>
      <title>Door Database</title>
   </head>
<body>
   <h1>database.csv generated</h1>
</body>
</html>
