<?php
header('Content-Description: File Transfer');
header("Content-Type: application/csv") ;
header("Content-Disposition: attachment; filename=database.csv");
//header("Pragma: no-cache");
header("Expires: 0");

echo "554948485052706651505767,Brian,Michael,01 JAN 2015,hangar,Hello\n";
echo "554948485052706651505768,Ben,Michael,01 JAN 2015,hangar,Hello\n";
echo "554948485052706651505769,Kate,Michael,01 JAN 2015,hangar,Hello\n";
echo "554948485052706651505770,Karen,Michael,01 JAN 2015,hangar,Hello\n";
echo "554948485052706651505771,Bill,Michael,01 JAN 2015,hangar,Hello\n";
echo "554948485052706651505772,Randy,Epstein,01 JAN 2015,hangar,Hello\n";
?>
