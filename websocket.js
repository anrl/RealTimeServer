			var sliceReceived = new Array();
			var startStream = document.getElementById('startStream');
			startStream.onclick = WebSockets;			

			var address;
			
			function str2ab(str) {
				var buf = new ArrayBuffer(str.length);
				var bufView = new Uint8Array(buf);
				for (var i=0, strLen=str.length; i<strLen; i++) {
					bufView[i] = str.charCodeAt(i);
				}
				return buf;
			}	

            function WebSockets() {
            	startStream.disabled = true;
                if ("WebSocket" in window) {
                	window.WebSocket = window.WebSocket || window.MozWebSocket;
                    var ws = new WebSocket("ws://localhost:7681/websocket", "callback_video_transfer");
//                    var ws = new WebSocket("ws://192.168.54.65:7681/websocket", "callback_video_transfer");

                    var frames = 0;
                    var image = new Array();
                    image[0] = document.getElementById('image');
                    image[1] = document.getElementById('image2');
                    var mytext = document.getElementById('header'); 
//                    var addr = document.getElementById('addr');                  
  
                    var data = new ArrayBuffer(10000);
                    var reader = new FileReader();
					reader.onload = function() {
						data = reader.result;
					};		
                    ws.onmessage = function (e) {											
						if (typeof(e.data) == "string"){
							window.address = e.data;
							addr.innerHTML = e.data;
						}
						else if(typeof(e.data) == "object"){
							reader.readAsText(e.data);
							var msgtype = data.substring(0,1);
							var imgID = data.substring(1,6);
							var sliceID = data.substring(6,8);
							mytext.innerHTML = msgtype+" "+imgID+" "+sliceID;
							var imgObject = e.data.slice(8, e.data.size);
							image[0].src = window.URL.createObjectURL(imgObject);
							frames++;
						}
                    };

                    var fpsOut = document.getElementById('fps');
                    setInterval(function(){
                      fpsOut.innerHTML = frames + " fps";
                      frames = 0;
                    }, 1000);
                }
            }
