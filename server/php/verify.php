<?php
  $rfid = $_GET["rfid"];
  // Do some database query to get account information.  For the interim, just dummy up some data...
 ?>
<html>
  <body>
    <form action="csv.php" method="get">
      <table border="0" cellpadding="0" cellspacing="0">
        <tr>
          <td>RFID: <input type="text" name="rfid"></td>
          <td><input type="submit"></td>
        </tr>
      </table>
    </form>
    <hr/>
    <table border="1">
      <tr>
        <td>RFID</td>
        <td>First Name</td>
        <td>Last Name</td>
        <td>Expire Date</td>
        <td>Access Type</td>
        <td>Message</td>
      </tr>
    </table>
    <pre>
<?php echo $rfid; ?>|Brian|Michael|01 JAN 2015|hangar|Hello
    </pre>
  </body>
</html>
