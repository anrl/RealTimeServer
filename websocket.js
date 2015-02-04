var ReceiveBuffer = new Array();
var IMG_NUM = 8;

var groupKey;
var id;
var groupmate;
var peer = new Peer();
var conn;
var connected = false;
var image = new Array();
			
function listenToData(){
	conn.on('data', function(data){
		if (data.constructor === ArrayBuffer) {
			var dataBlob = new Blob([data]);
			displayImage(dataBlob);
		}
 	});	
 	conn.on('close', function(){
 		for(var i=0; i<image.length; i++){
 			image[i].src = null;
		}
		console.log("disconnect");
	});
}
			
function createConnection() {
	console.log("create");
	conn = peer.connect(groupmate);
	connected = true;
	listenToData();
}
			
function listenToConnection() {
	console.log("listen");
	peer.on('connection', function(con) {
		connected = true;
		conn = con;	
		listenToData();	
	});
}
			
function displayImage(data) {
	var dataBlob = new Blob([data.slice(8, data.size)], {type: 'image/jpeg'});
	var reader = new FileReader();
	var temp = new ArrayBuffer(8);
	reader.onload = function() {
		temp = reader.result;
		var msgtype = temp.substring(0,1);
		var imgID =  temp.substring(1,6);
		var sliceID =  parseInt(temp.substring(6,8));
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
		var ws = new WebSocket("ws://localhost:7681/websocket", "callback_video_transfer");
//		var ws = new WebSocket("ws://192.168.54.65:7681/websocket", "callback_video_transfer");
	
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
				//msgtype = 0, key received and create a peer
				if (msgtype == "0"){
/*					var content = e.data.substring(1,e.data.size);
					var res = content.split("#");
					id = res[0];
					groupKey = res[1];*/
					groupKey = e.data.substring(1,e.data.size);
					peer = new Peer({key: groupKey, debug: 2});
					peer.on('open', function(myid) {
						id = myid;
						ws.send("0"+id);
					});
					//id.innerHTML = res[0];
					//groupKey.innerHTML = res[1];
				}
				//msgtype = 1, create connection with a peer
				else if (msgtype == "1"){
					var content = e.data.substring(1,e.data.size);
					groupmate = content;
					if (id > groupmate)
						createConnection();
					else listenToConnection();				
				}
				else mytext.innerHTML = "text: " + e.data;
			}
			else if(typeof(e.data) == "object"){
				displayImage(e.data);
				frames++;
							
				if (connected && conn.open)
					conn.send(e.data);
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
