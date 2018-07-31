//File system module
//var fs = require('fs');
//Provides an asynchronous network API for creating stream-based TCP or IPC servers 
var net = require('net');
//------------------------------------
// const AWS = require('aws-sdk');
// //*/ get reference to S3 client 
// var s3 = new AWS.S3();
//-----------------------------------
const AWS = require('aws-sdk');
var s3 = new AWS.S3();

var settings = {
    ip: "127.0.0.1",
    port: 5550
};

var moment = require('moment');


var _buffer = [];
var _bufferTimer = null;
var leakyDelay = 1000;

var imageCounter = 0;


var savePhoto = function() {
	//open file
	console.log("saving image");
	var data = Buffer.concat(_buffer);
	var filename = "images/" + imageCounter++ + ".jpg";

	console.log("writing " + data.length + " bytes to file");
	//Write File.jpg
	//fs.writeFileSync(filename, data);
	//----------------------
	var params = {
		"Body": data,            //Image
		"Bucket": "photonimagebucket",   //name of S3 Bucket
		"Key": filePath                  //Location in bucket
	 };
	 s3.upload(params, function(err, data){
		if(err) {
			callback(err, null);
		} else {
			let response = {
		 "statusCode": 200,
		 "headers": {
			 "my_header": "my_value"
		 },
		 "body": JSON.stringify(data),
		 "isBase64Encoded": false
	 };
			callback(null, response);
	 }
	 });
	//----------------------
	_buffer = [];
	 }
var leakyBufferFn = function(part) {
	if (_bufferTimer) {
		clearTimeout(_bufferTimer);
	}
	_buffer.push(part);
	_bufferTimer = setTimeout(savePhoto, leakyDelay);
}


var server = net.createServer(function (socket) {
    console.log("Connection received");

    socket.on('readable', function() {
        var data = socket.read();
		leakyBufferFn(data);
    });
});

server.listen(settings.port, function () {
	console.log('Listening for TCP packets on port ' + settings.port + ' ...');
});