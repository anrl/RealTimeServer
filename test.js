function myFunction(){
	console.log("hello world");
}
			var sliceReceived = new Array();
			
//			startStream.onclick = WebSockets;
			var groupKey;
            var id;
            var groupmate;
            var peer = new Peer();
            var conn;
            var connected = false;
            var image = new Array();
            
			var buff = new ArrayBuffer(8);
			var reader = new FileReader();
			reader.onload = function() {
				buff = reader.result;
			};
			
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

				reader.readAsText(data.slice(0,8));
				var msgtype = buff.substring(0,1);
				var imgID =  buff.substring(1,6);
				var sliceID =  parseInt(buff.substring(6,8));
				image[sliceID].src = window.URL.createObjectURL(dataBlob);
//				console.log(buff.substring(0,1));
			}
