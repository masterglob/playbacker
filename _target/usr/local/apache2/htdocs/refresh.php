<?php
header('Content-Type: text/xml'); 
echo "<?xml version=\"1.0\"?>\n";

function dataValue ($name)
{
	$fname="/home/www/res/".$name;
	if (file_exists ($fname))
	{
	    $f =  fopen("/home/www/res/".$name, "r") ;
		
		$s =  fread($f, 50);
	}
	else
	{
		$s =  "undefined";
	}
	echo $name . "='" . $s . "' ";
}

echo "<data ";

dataValue("projectName");
dataValue("lTrack");
for ($i = 0; $i < 64; $i++) {
    dataValue("lTrack".($i+1));
}
dataValue("lTrackIdx");
dataValue("timecode");
dataValue("lMessage");

echo "/>"
?>