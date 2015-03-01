var IMG_NUM = 8;
var FRAME_NUM = 10;
var image = new Array(IMG_NUM);
var ReceiveBuffer = new Array(FRAME_NUM);
var BackupBuffer = new Array(FRAME_NUM);
var SequenceNum = new Array(FRAME_NUM);
var FrameImageID = new Array(FRAME_NUM);
for (var i=0;i<FRAME_NUM;i++){
	ReceiveBuffer[i] = new Array(IMG_NUM);
	BackupBuffer[i] = new Array(IMG_NUM);
	SequenceNum[i] = new Array(IMG_NUM);
	FrameImageID[i] = -1;
}
var defaultPic = "http://media.zodee.net/products/20647-3855-50-white.jpg";
var currentImage = -FRAME_NUM;
var lastFrame = -1;

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
		
		window.URL.revokeObjectURL(BackupBuffer[imgID%FRAME_NUM][sliceID]);
		BackupBuffer[imgID%FRAME_NUM][sliceID] = ReceiveBuffer[imgID%FRAME_NUM][sliceID];
//		var url = ReceiveBuffer[imgID%FRAME_NUM][sliceID];
		ReceiveBuffer[imgID%FRAME_NUM][sliceID] = window.URL.createObjectURL(dataBlob);
		for(var i=0;i<FRAME_NUM;i++){
			if (ReceiveBuffer[i][sliceID] == BackupBuffer[imgID%FRAME_NUM][sliceID])
				ReceiveBuffer[i][sliceID] = ReceiveBuffer[imgID%FRAME_NUM][sliceID];
		}
//		console.log("create: "+ReceiveBuffer[imgID%FRAME_NUM][sliceID]);
//		console.log("release: "+url);
//		window.URL.revokeObjectURL(url);
		SequenceNum[imgID%FRAME_NUM][sliceID] = imgID;
		if (FrameImageID[imgID%FRAME_NUM] < imgID) FrameImageID[imgID%FRAME_NUM] = imgID;
//		console.log("sliceid: "+sliceID);
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
				else if (msgtype == "2"){
					var content = e.data.substring(1,e.data.size);
					var imgID =  parseInt(e.data.substring(1,6));
					var sliceID =  e.data.substring(6,e.data.size).split("#");
					console.log(sliceID);
					for(var i=0;i<sliceID.length;i++){
						var id = parseInt(sliceID[i]);
						ReceiveBuffer[imgID%FRAME_NUM][id] = ReceiveBuffer[(imgID+FRAME_NUM-1)%FRAME_NUM][id];
						SequenceNum[imgID%FRAME_NUM][id] = imgID;
						if (FrameImageID[imgID%FRAME_NUM] < imgID) FrameImageID[imgID%FRAME_NUM] = imgID;
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
			var pos = (currentImage+5)%FRAME_NUM;
			if (currentImage<0) {
				currentImage++;
			}
			else{
//				console.log(lastFrame+" "+FrameImageID[pos]);
				if (lastFrame < FrameImageID[pos]){
					lastFrame = FrameImageID[pos];
					for(var i=0;i<IMG_NUM;i++){
						if (SequenceNum[pos][i] < FrameImageID[pos]) ReceiveBuffer[pos][i] = ReceiveBuffer[(IMG_NUM+pos-1)%IMG_NUM][i];
						image[i].src = ReceiveBuffer[pos][i];
					}
				}
			}
				
		}, 100);
    }
}
