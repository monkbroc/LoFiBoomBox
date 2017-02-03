/* Lo-Fi Network Boom Box
 *
 * Copyright 2016 Julien Vanier. Released under the MIT license
 * 
 */
#include "speaker.h"
#include <stdlib.h>

SYSTEM_THREAD(ENABLED);

/* Configure audio output */
const uint16_t bufferSize = 128;
Speaker speaker(bufferSize);

const uint16_t audioFrequency = 22050; // Hz

const auto trackLength = 45033332;
uint32_t playbackPos = 0;

/* Network stuff */
UDP Udp;
const int localPort = 4444;
const int remotePort = 41234;
IPAddress remoteIpAddress(192,168,0,7);

const auto networkBufferSize = 2*1024;
int8_t networkBuffer[networkBufferSize] = { 0 };
uint32_t networkPos = playbackPos + networkBufferSize / 2;

void playSilence(uint16_t *buffer) {
  for (uint16_t i = 0; i < bufferSize; i++)
  {
    buffer[i] = 128 << 8;
  }
}

void playSamples(uint16_t *buffer) {
  for (uint16_t i = 0; i < bufferSize; i++)
  {
    int8_t sample = networkBuffer[playbackPos++ % networkBufferSize];
    buffer[i] = ((uint16_t)((int16_t)sample + 128)) << 8;
  }

  if (playbackPos > trackLength) {
    playbackPos = 0;
    networkPos = networkBufferSize / 2;
  }
}
void updateAudio() {
  if (speaker.ready()) {
    if (!Particle.connected()) {
      playSilence(speaker.getBuffer());
    } else {
      playSamples(speaker.getBuffer());
    }
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
  // Random start
  unsigned int seed;
  EEPROM.get(0, seed);
  randomSeed(seed);
  seed = rand();
  EEPROM.put(0, seed);

  playbackPos = random(trackLength) & ~(networkBufferSize-1);
  networkPos = playbackPos + networkBufferSize / 2;

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
  // Reconnect UDP
  static bool lastConnected = true;
  if (Particle.connected() && !lastConnected) {
    Udp.begin(localPort);
  }
  lastConnected = Particle.connected();

  updateAudio();

  if (playbackPos >= networkPos) {
    downloadSamples();
  }
}

