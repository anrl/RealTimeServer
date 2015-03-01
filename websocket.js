var IMG_NUM = 8;
var FRAME_NUM = 10;
var image = new Array(IMG_NUM);
var ReceiveBuffer = new Array(FRAME_NUM);
for (var i=0;i<FRAME_NUM;i++){
	ReceiveBuffer[i] = new Array(IMG_NUM);
}
var defaultPic = "http://media.zodee.net/products/20647-3855-50-white.jpg";
var currentImage = -FRAME_NUM;

var groupKey;
var id;
var peer = new Peer({host: '132.206.55.124', port: 9000});
peer.on('open', function(myid) {
	id = myid;
});
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
// 		for(var i=0; i<image.length; i++){
// 			image[i].src = defaultPic;
//		}
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
		var imgID =  parseInt(temp.substring(1,6));
		if (currentImage>=0 && imgID > currentImage) currentImage = imgID;
		var sliceID =  parseInt(temp.substring(6,8));
		window.URL.revokeObjectURL(ReceiveBuffer[imgID%FRAME_NUM][sliceID]);
		ReceiveBuffer[imgID%FRAME_NUM][sliceID] = window.URL.createObjectURL(dataBlob);
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
		var ws = new WebSocket("ws://132.206.55.124:9002/websocket", "callback_video_transfer");
		
		ws.onopen = function(){
			ws.send("0"+id);
			listenToConnection();
		};
		
		var frames = 0;
		for(var i=0;i<IMG_NUM;i++){
			image[i] = document.createElement("img");
			image[i].width = 80;
			image[i].height = 480;
			image[i].src = defaultPic;
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
			if (currentImage<0) {
				currentImage++;
			}
			else{
				frames++;
				for(var i=0;i<IMG_NUM;i++){
					image[i].src = ReceiveBuffer[currentImage%FRAME_NUM+1][i];
				}	
			}
				
		}, 100);
    }
}
