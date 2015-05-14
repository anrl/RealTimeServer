var counter = 0;
var PIECE_NUM = 8;
var FRAME_NUM = 40;
var image = new Array(PIECE_NUM);	//document element of images
var ReceiveBuffer = new Array(FRAME_NUM); //store URLs of each piece displayed
//var BackupBuffer = new Array(FRAME_NUM); 
var SequenceNum = new Array(FRAME_NUM); //store the imageID of each piece in each frame
var FrameImageID = new Array(FRAME_NUM); //store the largest imageID of frame
var RESPONSIBILITY_MAP = new Array(PIECE_NUM); 
//store the responsibility relationship between peers and pieces, when imageID = RESPONSIBILITY_imageID
var RESPONSIBILITY_imageID;
var myfd;
var Slow_Value = {};

//Initialization of arrays
for (var i=0;i<FRAME_NUM;i++){
	ReceiveBuffer[i] = new Array(PIECE_NUM);
//	BackupBuffer[i] = new Array(PIECE_NUM);
	SequenceNum[i] = new Array(PIECE_NUM);
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
//			console.log("receive from group");
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
	conn[i] = peer.connect(groupmate, {reliable: true});
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

function garbageCollection(sliceID, imgID){
	var pos = imgID % FRAME_NUM;
	var flag = false;
	for(var i=0;i<FRAME_NUM;i++){
		if ((i!=pos) && (ReceiveBuffer[pos][sliceID]==ReceiveBuffer[i][sliceID])){
			flag = true;
			break;
		}
	}
	if (!flag) {
		window.URL.revokeObjectURL(ReceiveBuffer[pos][sliceID]);
//		console.log("imgID: "+imgID+" currentImage: "+currentImage+" revoked: "+ReceiveBuffer[pos][sliceID]);
	}
	if (typeof SequenceNum[pos][sliceID] === 'undefined' || SequenceNum[pos][sliceID] < imgID)
		SequenceNum[pos][sliceID] = imgID;
	if (FrameImageID[pos] < imgID) FrameImageID[pos] = imgID;
}

function displayImage(data) {
	var dataBlob = new Blob([data.slice(8, data.size)], {type: 'image/jpeg'});
	var reader = new FileReader();
	var temp = new ArrayBuffer(8);
	reader.onload = function() {
		temp = reader.result;
		var imgID =  parseInt(temp.substring(1,6));
		var pos = imgID%FRAME_NUM;
		if (currentImage>=0 && imgID > currentImage) currentImage = imgID;
		var sliceID =  parseInt(temp.substring(6,8));
//		console.log(imgID+" "+sliceID);
		garbageCollection(sliceID, imgID);
		if (SequenceNum[pos][sliceID] == imgID)
			ReceiveBuffer[pos][sliceID] = window.URL.createObjectURL(dataBlob);
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
		for(var i=0;i<PIECE_NUM;i++){
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
				//msgtype = 2, some pieces of the frame is exact the same as the previous frame
				else if (msgtype == "2"){
					var imgID =  parseInt(e.data.substring(1,6));
					var sliceID =  e.data.substring(6,e.data.size).split("#");
					for(var i=0;i<sliceID.length;i++){
						var id = parseInt(sliceID[i]);
//						console.log("duplicate "+imgID+" "+sliceID)
						garbageCollection(id, imgID);
						if (currentImage>0)
							ReceiveBuffer[imgID%FRAME_NUM][id] = ReceiveBuffer[currentImage%FRAME_NUM][id];
						else
							for(var j=1;j<FRAME_NUM;j++)
								if (typeof ReceiveBuffer[(imgID+FRAME_NUM-j)%FRAME_NUM][id] !== 'undefined'){
									ReceiveBuffer[imgID%FRAME_NUM][id] = ReceiveBuffer[(imgID+FRAME_NUM-j)%FRAME_NUM][id];
									break;
								}
//						console.log("after: "+ReceiveBuffer[imgID%FRAME_NUM][id]);
					}
					if (currentImage>=0 && imgID > currentImage) currentImage = imgID;
				}
				//msgtype = 3, send the relationship map between peers and pieces
				else if (msgtype == "3"){
					var content = e.data.substring(1,e.data.size).split("#");
					RESPONSIBILITY_imageID = parseInt(content[0]);
					fd = parseInt(content[1]);
					for(var i=2;i<content.length;i++) {
						var peer_piece = content[i].split(":");
						var piece = parseInt(peer_piece[1]);
						RESPONSIBILITY_MAP[piece] = parseInt(peer_piece[0]);
					}
					console.log("image id "+RESPONSIBILITY_imageID);
					console.log("fd is "+fd);
//					var temp = Slow_Value;					
					for(var i=0;i<PIECE_NUM;i++) {
//						temp[RESPONSIBILITY_MAP[i]] = 0;
						console.log(i+" : "+RESPONSIBILITY_MAP[i]);
						Slow_Value[RESPONSIBILITY_MAP[i]] = 0;
//						if (typeof temp[RESPONSIBILITY_MAP[i]] !== 'undefined')
//							Slow_Value[RESPONSIBILITY_MAP[i]] = temp[RESPONSIBILITY_MAP[i]];
					}
//					for(var key in SLow_Value)
					
				}
				else mytext.innerHTML = "text: " + e.data;
			}
			else if(typeof(e.data) == "object"){
//				console.log("receive from server");
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
			var pos = (currentImage+10)%FRAME_NUM;
			if (currentImage<0) {
				currentImage++;
			}
			else{
//				console.log(lastFrame+" "+FrameImageID[pos]);
//				if (currentImage!=FrameImageID[currentImage%FRAME_NUM])
//					console.log(FrameImageID[currentImage%FRAME_NUM]+" "+currentImage);
				if (lastFrame <= FrameImageID[pos]){
					lastFrame = FrameImageID[pos];
					for(var i=0;i<PIECE_NUM;i++){
						if (SequenceNum[pos][i] < FrameImageID[pos]) {
							var diff = (i+PIECE_NUM-(FrameImageID[pos]-RESPONSIBILITY_imageID)%PIECE_NUM)%PIECE_NUM;
							var fd = RESPONSIBILITY_MAP[diff];
							Slow_Value[fd]++;
//							console.log(RESPONSIBILITY_MAP[(i+FrameImageID[pos]-RESPONSIBILITY_imageID)%PIECE_NUM]);
							console.log("current image id: "+lastFrame+", piece No."+i+", this image id: "+SequenceNum[pos][i]);
							if (SequenceNum[pos][i] < SequenceNum[(pos+FRAME_NUM-1)%FRAME_NUM][i])
								ReceiveBuffer[pos][i] = ReceiveBuffer[(pos+FRAME_NUM-1)%FRAME_NUM][i];
//							garbageCollection(i, pos);
							SequenceNum[pos][i] = FrameImageID[pos];
						}
						else if(SequenceNum[pos][i] > FrameImageID[pos])
							console.log(SequenceNum[pos][i]+" > "+FrameImageID[pos]);
						image[i].src = ReceiveBuffer[pos][i];
					}
				}
				else
					console.log(FrameImageID[pos]+" "+lastFrame);
			}
			counter++;
			console.log(counter);
			for(var fd in Slow_Value)
				console.log("fd: "+fd+" value: "+Slow_Value[fd]);
				
		}, 100);
    }
}
