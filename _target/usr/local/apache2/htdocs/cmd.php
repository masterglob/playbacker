<?php
$log = fopen("/tmp/cmd.log","w");
fwrite($log,  "Command received" );
$value = $_GET["cmd"];
fwrite($log,  "$value= " . $value);
if ( ! empty ($value) )
{ 
    fwrite($log,  "opening... /home/www/cmd");
    $file = fopen("/home/www/cmd","a");
    fwrite($file,  $value );
    fwrite($log,  "write..." . $value);
    fclose($file);
}
?>
