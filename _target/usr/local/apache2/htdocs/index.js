
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
					elt.className =  "button buttonGraySmall";
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