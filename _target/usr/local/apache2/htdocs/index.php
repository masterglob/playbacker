 <!DOCTYPE html>
<html>
<head>
    <title>Playbacker Page</title>
    <link rel="stylesheet" href="style.css" type="text/css" />
</head>
<body style="background-color:rgb(30,20,30); color:rgb(230,220,240);">

<script type="text/javascript" src="index.js"></script>

<div id="bip" class="display">DIV</div>
<BR>


<table id="fr0">
	<tr>
		<td  rowspan="2" id="frYellow">
			PLAYBACK CONTROL
			<table id="subFrame" width="140">
			<tr>
				<td colspan="1" id="subFrame" width="50%"> PLAY</td>
				<td colspan="1" id="subFrame" width="50%"> STOP</td>
			</tr>
			<tr>
				<td><button class="button buttonGreen" onclick=""/></td>
				<td><button class="button buttonGray" onclick=""/></td>
			</tr>
			</table>
		</td>
		<td colspan="1" id="frYellow">
			<table id="subFrame" style="table-layout: fixed; width: 100%;">
			<tr>
				<td><button class="button buttonGraySmall" width="33%">&lt;&lt;</button></td>
				<td><button class="button buttonGraySmall" width="33%">&gt;&gt;</button></td>
				<td id="timecode" width="33%">&nbsp;&nbsp;:&nbsp;&nbsp;</td>
			</tr>
			</table>
		</td>
	</tr>
	<tr>
		<td colspan="2" id="frYellow">
		Message:
			<table id="subFrame" style="table-layout: fixed; width: 100%; height:50px;text-align: center;">
			<tr>
				<td id="lMessage"></td>
			</tr>
			</table>
		</td>
	</tr>
	<tr>
		<td colspan="2" id="frYellow">
			<table>
				<tr>
					<td> Playlist</td>
					<td class="subFrame"><div id="projectName"></div></td>
				</tr>
			</table>
		</td>
	</tr>
	<tr>
		<td colspan="2" id="frYellow">
			<table style="table-layout: fixed; width: 100%;">
				<tr>
<?php
$nbCol=5;
echo '	<td width ="30%" colspan="'.$nbCol.'"> Current Track</td>';
echo '	<td width ="70%" id= "lTrack" class="subFrame"></td>';
?>
				</tr>
			</table>
			<table style="table-layout: fixed; width: 100%;">
				<tr>
<?php			
    for ($i = 0; $i < 3; $i++) 
    {
        echo "<tr>";
        for ($j = 1; $j <= $nbCol; $j++)
        {
            $trackId = $j + $i * $nbCol;
            echo '<td><button id="lTrack'.$trackId.'" class="button buttonGraySmall">'.$trackId.'</button></td>';
        };
        echo "</tr>";
    }
?>
			</table>
		</td>
	</tr>
</table>

</body>
</html> 