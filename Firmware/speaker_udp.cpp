/* Lo-Fi Network Boom Box
 *
 * Copyright 2016 Julien Vanier. Released under the MIT license
 * 
 */
#include "speaker.h"

SYSTEM_THREAD(ENABLED);

/* Configure audio output */
const uint16_t bufferSize = 128;
Speaker speaker(bufferSize);

const uint16_t audioFrequency = 22050; // Hz

uint32_t playbackPos = 2032640; // start on second track

/* Network stuff */
UDP Udp;
const int localPort = 4444;
const int remotePort = 41234;
IPAddress remoteIpAddress(192,168,0,118);

const auto networkBufferSize = 2*1024;
int8_t networkBuffer[networkBufferSize] = { 0 };
uint32_t networkPos = playbackPos + networkBufferSize / 2;

void playSamples(uint16_t *buffer) {
  for (uint16_t i = 0; i < bufferSize; i++)
  {
    int8_t sample = networkBuffer[playbackPos++ % networkBufferSize];
    buffer[i] = ((uint16_t)((int16_t)sample + 128)) << 8;
  }
}
void updateAudio() {
  if (speaker.ready()) {
    playSamples(speaker.getBuffer());
  }
}

void downloadSamples() {
  String command = String::format("%d:%d", networkPos, networkBufferSize/2);
  //Serial.println(command);
  Udp.sendPacket(command, command.length(), remoteIpAddress, remotePort);

  uint8_t *buffer = (uint8_t*)&networkBuffer[networkPos % networkBufferSize];
  int ret;
  auto start = millis();
  const auto TIMEOUT = 4;
  while ((ret = Udp.receivePacket(buffer, networkBufferSize / 2)) <= 0 &&
      millis() - start < TIMEOUT) {
    Particle.process();
    updateAudio();
  }
  //Serial.printlnf("Received %d", ret);

  networkPos += networkBufferSize / 2;
}

/* Particle function to restart play */
int play(String arg)
{
  playbackPos = 0;
  networkPos = networkBufferSize / 2;
  return 0;
}

/* Set up the speaker */
void setup() {
  Serial.begin(9600);
  waitUntil(Particle.connected);
  Udp.begin(localPort);
  Particle.function("play", play);
  //Serial.println("Connected");

  // Start the audio output
  speaker.begin(audioFrequency);
  downloadSamples();
}

/* Download more audio data when needed */
void loop() {
  updateAudio();

  if (playbackPos >= networkPos) {
    downloadSamples();
  }
}

