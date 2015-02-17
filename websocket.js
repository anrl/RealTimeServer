var ReceiveBuffer = new Array();
var image = new Array();
var IMG_NUM = 8;

var groupKey;
var id;
var peer = new Peer();
var conn = [null, null, null, null, null, null, null, null];
var connected = false;

function findEmptyPeer(){
	for(i=0;i<conn.length;i++){
		if (!conn[i]) return i;
	}
	return -1;
}

function listenToData(i){
	conn[i].on('data', function(data){
		if (data.constructor === ArrayBuffer) {
			var dataBlob = new Blob([data]);
			displayImage(dataBlob);
		}
 	});	
 	conn[i].on('close', function(){
 		for(var i=0; i<image.length; i++){
 			image[i].src = null;
		}
		conn[i] = null;
		console.log("disconnect");
	});
}
			
function createConnection(groupmate) {
	console.log("create");
	var i = findEmptyPeer();
	conn[i] = peer.connect(groupmate);
	connected = true;
	listenToData(i);
}
			
function listenToConnection() {
	console.log("listen");
	peer.on('connection', function(con) {
		console.log("connected");
		connected = true;
		var i = findEmptyPeer();
		conn[i] = con;	
		listenToData(i);	
	});
}
			
function displayImage(data) {
	var dataBlob = new Blob([data.slice(8, data.size)], {type: 'image/jpeg'});
	var reader = new FileReader();
	var temp = new ArrayBuffer(8);
	reader.onload = function() {
		temp = reader.result;
//		var msgtype = temp.substring(0,1);
		var imgID =  temp.substring(1,6);
		var sliceID =  parseInt(temp.substring(6,8));
		console.log(sliceID);
		window.URL.revokeObjectURL(ReceiveBuffer[sliceID]);
		ReceiveBuffer[sliceID] = window.URL.createObjectURL(dataBlob);
	};
	reader.readAsText(data.slice(0,8));
}

function WebSockets(button) {
	button.disabled = true;
	if ("WebSocket" in window) {
		var counter=0;
		var address;
		window.WebSocket = window.WebSocket || window.MozWebSocket;
//		var ws = new WebSocket("ws://localhost:7681/websocket", "callback_video_transfer");
		var ws = new WebSocket("ws://192.168.54.86:7681/websocket", "callback_video_transfer");
		
		ws.onopen = function(){
			peer = new Peer({host: '192.168.54.86', port: 9000});
			peer.on('open', function(myid) {
				id = myid;
				ws.send("0"+id);
			});
			listenToConnection();
		};
		
		var frames = 0;
		var imageID = -1;
		for(var i=0;i<IMG_NUM;i++){
			image[i] = document.createElement("img");
			image[i].width = 60;
			document.body.appendChild(image[i]);
		}
		var mytext = document.getElementById('mytext'); 

		ws.onmessage = function (e) {										
			if (typeof(e.data) == "string"){
				var msgtype = e.data.substring(0,1);
				//msgtype = 1, create connection with a peer
				if (msgtype == "1"){
					var content = e.data.substring(1,e.data.size);
					var groupmates = content.split("#");
					console.log("content: "+groupmates);
			 		for(var i=0; i<groupmates.length; i++){
			 			createConnection(groupmates[i]);
					}				
				}
				else mytext.innerHTML = "text: " + e.data;
			}
			else if(typeof(e.data) == "object"){
				displayImage(e.data);
				frames++;
							
				for(var i=0;i<conn.length;i++){
					if (connected && conn[i]!=null && conn[i].open){
						conn[i].send(e.data);
					}
				}
			}
		};
		var fpsOut = document.getElementById('fps');
		setInterval(function(){
		  fpsOut.innerHTML = frames + " fps";
		  frames = 0;
		}, 1000);
		
		setInterval(function(){
			for(var i=0;i<IMG_NUM;i++){
				image[i].src = ReceiveBuffer[i];
			}		
		}, 100);
    }
}
