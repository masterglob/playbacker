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
  <button class="tablinks" onclick="openTab(event, 'Project')">Project</button>
  <button class="tablinks" onclick="openTab(event, 'Config')">Config</button>
</div>

<div id="Control" class="tabcontent">
<table id="fr0" width="100%">
	<tr>
		<td id="frYellow">
			PLAYBACK CONTROL
			<table id="subFrame" style="table-layout: fixed; width: 100%;">
			<tr style=" height: 240px;">
				<td><button class="button buttonGreen" onclick="sendCmd('pPlay')"/>PLAY</td>
				<td><button class="button buttonRed" onclick="sendCmd('pStop')"/>STOP</td>
				<td><button class="button buttonGray" width="40%" onclick="sendCmd('pBackward')">&lt;&lt;</button></td>
				<td><button class="button buttonGray" width="40%" onclick="sendCmd('pFastForward')">&gt;&gt;</button></td>
			</tr>
			<tr>
			<td colspan="2" id="lPlayStatus"></td>
			<td colspan="2" id="timecode">&nbsp;&nbsp;:&nbsp;&nbsp;</td>
			</tr>
			</table>
		</td>
	</tr>
	
	<?php /*
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
	*/
	?>
	
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
$nbCol=3;
echo '	<td width ="30%" colspan="'.$nbCol.'"> Current Track</td>';
echo '	<td width ="70%" id= "lTrack" class="subFrame"></td>';
?>
				</tr>
			</table>
			<table style="table-layout: fixed; width: 100%;">
				<tr>
<?php
    $nTrack=20;
    for ($i = 0; $i < $nTrack / $nbCol; $i++) 
    {
        echo "<tr>";
        for ($j = 1; $j <= $nbCol; $j++)
        {
            $trackId = $j + $i * $nbCol;
            echo '<td height="200"><button id="lTrack'.$trackId.'" class="button trackInactive" ';
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

<?php 
/*******************************************************/
/*******************************************************/
    /* "PROJECTS" tab */
?>
<div id="Project" class="tabcontent">

<table id="fr0" width="100%">
	<tr>
		<td  rowspan="2" id="frYellow" width="140">
			PROJECTS
			<table id="subFrame" width="100%">

<?php 
for ($i = 1; $i < 6 ; $i++) 
{
    echo '<tr><td height=150><button id="project'. $i .'" class="projectSel" onclick="sendCmd(\'project/' . $i. '\')"></button></td></tr>';
}
?>
			</table>
		</td>
	</tr>
</table>
</div>


<?php 
/*******************************************************/
/*******************************************************/
    /* "CONFIG" tab  */
?>
<div id="Config" class="tabcontent">
<!-- 	<div id="lConsole"></div> -->
<table id="fr0" width="100%">
	<tr>
	<td width="30%">Version </td>
	<td id="lVersion"/>
	</tr>
	<tr>
	<td width="30%">Build </td>
	<td id="lBuild"/>
	</tr>
	<tr>
	<td width="30%">Message </td>
	<td id="lMessage"/>
	</tr>
	
</table>

<table  id="frYellow" width="100%">
	<tr>
	<td width="30%">Menu L1 </td>
	<td id="lMenuL1"/>
	</tr>
	
	<tr>
	<td width="30%">Menu L2 </td>
	<td id="lMenuL2"/>
	</tr>
	
	<tr>
		<table id="subFrame" style="table-layout: fixed; width: 100%;">
			<tr style=" height: 240px;">
				<td width="20%"></td>
				<td width="20%"><button class="button buttonGray" onclick="sendCmd('pUp')">UP</button></td>
				<td width="20%"></td>
				<td width="20%"><button class="button buttonGreen" onclick="sendCmd('pOk')">OK</button></td>
				<td width="20%"><button class="button buttonRed" onclick="sendCmd('pClose')">CLOSE</button></td>
			</tr><tr>
				<td width="20%"><button class="button buttonGray" onclick="sendCmd('pLeft')"/>LEFT</td>
				<td width="20%"><button class="button buttonGray" onclick="sendCmd('pDown')">DOWN</button></td>
				<td width="20%"><button class="button buttonGray" onclick="sendCmd('pRight')"/>RIGHT</td>
				<td width="40%"></td>
			</tr><tr>
				<td width="20%"></td>
				<td width="60%"></td>
			</tr>
		</table>

</table>
	
</div>


</body>

<script>
// Get the element with id="defaultOpen" and click on it
document.getElementById("defaultOpen").click();

// const width  = window.innerWidth || document.documentElement.clientWidth || 
// document.body.clientWidth;
// const height = window.innerHeight|| document.documentElement.clientHeight|| 
// document.body.clientHeight;

// document.getElementById("lConsole").innerHTML = "W=" + width + ",H=" +height;

</script>
</html> 