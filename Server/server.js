const fs = require('fs');
const dgram = require('dgram');

const server = dgram.createSocket('udp4');

const musicFile = fs.openSync('music_s8_22050.raw', 'r');

server.on('error', (error) => {
  console.log(`server error: ${error.stack}`);
  server.close();
});

server.on('message', (message, rinfo) => {
  process.stdout.write('.');
  //console.log(`Message from ${rinfo.address}:${rinfo.port}: ${message}`);
  const command = message.toString('utf8');
  const [start, size] = command.split(':').map(x => parseInt(x));
  const buffer = Buffer.alloc(size);
  fs.read(musicFile, buffer, 0, buffer.length, start, (err, bytesRead, buffer) => {
    //console.log(buffer);
    server.send(buffer, 0, buffer.length, rinfo.port, rinfo.address);
  });
});

server.on('listening', () => {
  var address = server.address();
  console.log(`server listening ${address.address}:${address.port}`);
});

server.bind(41234);
