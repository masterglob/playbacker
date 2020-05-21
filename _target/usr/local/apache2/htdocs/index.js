function openTab(evt, tabName) {
  // Declare all variables
  var i, tabcontent, tablinks;

  // Get all elements with class="tabcontent" and hide them
  tabcontent = document.getElementsByClassName("tabcontent");
  for (i = 0; i < tabcontent.length; i++) {
    tabcontent[i].style.display = "none";
  }

  // Get all elements with class="tablinks" and remove the class "active"
  tablinks = document.getElementsByClassName("tablinks");
  for (i = 0; i < tablinks.length; i++) {
    tablinks[i].className = tablinks[i].className.replace(" active", "");
  }

  // Show the current tab, and add an "active" class to the button that opened the tab
  document.getElementById(tabName).style.display = "block";
  evt.currentTarget.className += " active";
} 

function reqRefrReady(xhr)
{
    // if (xhr.readyState==4) 
	if (xhr.responseXML != null) 
    {
        var docXML= xhr.responseXML;
        var items = docXML.getElementsByTagName("data")
		if (items.length == 1){
			var data = items.item(0);
			if (data.hasAttributes()) {
				var attrs = data.attributes;
				for(var i = attrs.length - 1; i >= 0; i--) {
					var attr = attrs[i];
					var inhtml = document.getElementById(attr.name);
					if (inhtml != null) { inhtml.innerHTML =  attr.value;}
			       }
			}
			var trackId = Number (data.getAttribute ("lTrackIdx"));
			for(let i = 1; i < 14; i++)
			{	
				var name =  "lTrack"  + (i );
				var elt = document.getElementById(name);
				if (elt ==null) continue;
				if (i == trackId)
				{
					elt.className =  "button trackActive";
				}
				else
				{
					elt.className =  "button trackInactive";
				}
			}
		}
    }
}

function reqRefr()
{
    var xhr=null;
 
    if (window.XMLHttpRequest) { 
        xhr = new XMLHttpRequest();
    }
    else if (window.ActiveXObject) 
    {
        xhr = new ActiveXObject("Microsoft.XMLHTTP");
    }
    //on d�finit l'appel de la fonction au retour serveur
    xhr.onreadystatechange = function() { reqRefrReady(xhr); };

    xhr.open("GET", "/refresh.php", true);
    xhr.send(null);
}

var counter = 4;
function finish() {
  clearInterval(intervalId);
  // document.getElementById("bip").innerHTML = "TERMINE!";	
}
function refresh() {
    //counter--;
    if(counter == 0) finish();
    else {	
		reqRefr();
        //document.getElementById("bip").innerHTML = counter + " secondes restantes";
        //document.getElementById("lPlaylist").innerHTML = counter + " secondes restantes";
    }	
}

var intervalId = setInterval(refresh, 500);

function sendCmd(cmd){
	var url = "/cmd.php?cmd=" + cmd;
    var xhr=null;
 
    if (window.XMLHttpRequest) { 
        xhr = new XMLHttpRequest();
    }
    else if (window.ActiveXObject) 
    {
        xhr = new ActiveXObject("Microsoft.XMLHTTP");
    }
    xhr.open("GET", url, true);
    xhr.send();
}