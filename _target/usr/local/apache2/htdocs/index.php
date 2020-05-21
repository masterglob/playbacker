 <!DOCTYPE html>
<html>
<head>
    <title>Playbacker Page</title>
    <link rel="stylesheet" href="style.css" type="text/css" />
</head>
<body style="background-color:rgb(30,20,30); color:rgb(230,220,240);">

<script type="text/javascript" src="index.js"></script>

<div class="tab">
  <button class="tablinks" onclick="openTab(event, 'Control')" id="defaultOpen">Control</button>
  <button class="tablinks" onclick="openTab(event, 'Keyb')">Keyboard</button>
  <button class="tablinks" onclick="openTab(event, 'Setup')">Setup</button>
</div>

<div id="Control" class="tabcontent">
<table id="fr0">
	<tr>
		<td  rowspan="2" id="frYellow" width="140">
			PLAYBACK CONTROL
			<table id="subFrame">
			<tr>
				<td colspan="1" id="subFrame" width="50%"> PLAY</td>
				<td colspan="1" id="subFrame" width="50%"> STOP</td>
			</tr>
			<tr>
				<td><button class="button buttonGreen" onclick="sendCmd('pPlay')"/></td>
				<td><button class="button buttonGray" onclick="sendCmd('pStop')"/></td>
			</tr>
			</table>
		</td>
		<td colspan="1" id="frYellow">
			<table id="subFrame" style="table-layout: fixed; width: 100%;">
			<tr>
				<td><button class="button buttonGraySmall" width="40%" onclick="sendCmd('pBackward')">&lt;&lt;</button></td>
				<td><button class="button buttonGraySmall" width="40%" onclick="sendCmd('pFastForward')">&gt;&gt;</button></td>
				<td id="timecode" width="20%">&nbsp;&nbsp;:&nbsp;&nbsp;</td>
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
            echo '<td><button id="lTrack'.$trackId.'" class="button trackInactive" ';
            echo 'onclick="sendCmd(\'mtTrackSel/' . $trackId. '\')">';
            echo $trackId;
            echo '</button></td>';
        };
        echo "</tr>";
    }
?>
			</table>
		</td>
	</tr>
</table>
</div>

<div id="Control" class="tabcontent">
</div>
<div id="Keyb" class="tabcontent">
</div>


</body>

<script>
// Get the element with id="defaultOpen" and click on it
document.getElementById("defaultOpen").click();
</script>
</html> 