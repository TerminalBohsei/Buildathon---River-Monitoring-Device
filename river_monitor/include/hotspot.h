// hotspot.h --> the esp32's own wifi network and the download web page
#pragma once

// starts the wifi hotspot and web server
void hotspotStart();

// handles web page requests, call often while the hotspot is on
void hotspotHandle();

// turns the hotspot and web server off
void hotspotStop();

// runs the hotspot until no one has been connected for a while
void hotspotRunUntilIdle();
