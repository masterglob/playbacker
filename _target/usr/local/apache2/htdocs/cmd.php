<?php
$log = fopen("/tmp/cmd.log","w");
fwrite($log,  "Command received\n" );
$value = $_GET["cmd"];
fwrite($log,  "$value= " . $value. "\n");
if ( ! empty ($value) )
{ 
    fwrite($log,  "opening... /home/www/cmd\n");
    $file = fopen("/home/www/cmd","a");
    fwrite($file,  $value );
    fwrite($log,  "write..." . $value. "\n");
    fclose($file);
}
?>
