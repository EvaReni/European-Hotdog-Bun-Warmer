#include "pitches.h"
#include "songDefs.h"


// Tempo (beats per minute)
#define TEMPO       120

// Duration of a whole note in ms (4 beats)
#define WHOLE_NOTE  (240000 / TEMPO)  // 2000ms at 120 BPM
#define HALF_NOTE   (WHOLE_NOTE / 2)  // 1000ms
#define QUARTER_NOTE (WHOLE_NOTE / 4) // 500ms
#define EIGHTH_NOTE  (WHOLE_NOTE / 8) // 250ms
#define SIXTEENTH_NOTE (WHOLE_NOTE / 16) // 125ms
#define NOTE_REST 0


#define DONE  0
#define FAIL  1
#define RESET 2

const int buzzerPin = 8;

// Song: {note, duration}
// Example song with different durations
int doneSong[][2] = {
  {NOTE_C5, EIGHTH_NOTE},
  {NOTE_E5, EIGHTH_NOTE},
  {NOTE_G5, EIGHTH_NOTE},
  {NOTE_C6, QUARTER_NOTE},
  {NOTE_G5, EIGHTH_NOTE},
  {NOTE_C6, HALF_NOTE}
};

int _failSong[][2] = {
  {NOTE_E5, SIXTEENTH_NOTE},
  {NOTE_D5, SIXTEENTH_NOTE},
  {NOTE_C5, EIGHTH_NOTE},
  {NOTE_REST, SIXTEENTH_NOTE},
  {NOTE_C4, SIXTEENTH_NOTE},
  {NOTE_G3, QUARTER_NOTE}
};

int failSong[][2] = {
  {NOTE_A4, EIGHTH_NOTE},
  {NOTE_E4, EIGHTH_NOTE},
  {NOTE_F4, EIGHTH_NOTE},
  {NOTE_E4, EIGHTH_NOTE},
  {NOTE_D4, QUARTER_NOTE},
  {NOTE_C4, EIGHTH_NOTE},
  {NOTE_A3, HALF_NOTE}
};


int resetSong[][2] = {
  {NOTE_E5, SIXTEENTH_NOTE},
  {NOTE_D5, SIXTEENTH_NOTE},
  {NOTE_E5, SIXTEENTH_NOTE},
  {NOTE_F5, SIXTEENTH_NOTE},
  {NOTE_G5, SIXTEENTH_NOTE},
  {NOTE_A5, SIXTEENTH_NOTE},
  {NOTE_F5, SIXTEENTH_NOTE},
  {NOTE_A5, EIGHTH_NOTE},
  {NOTE_REST, EIGHTH_NOTE},
  {NOTE_A5, QUARTER_NOTE}
};

int (*song)[2] = nullptr;
int songLength = 0;


int currentNote = 0;
unsigned long noteStartTime = 0;
bool notePlaying = false;


extern bool played;

void initPlaySong() {
  pinMode(buzzerPin, OUTPUT);

  song = doneSong;
  songLength = sizeof(doneSong) / sizeof(doneSong[0]);
}

void setSong(int songId) {

  if(notePlaying) return;
 
  switch (songId) {
    case DONE:
      song = doneSong;
      songLength = sizeof(doneSong) / sizeof(doneSong[0]);
      break;

    case FAIL:
      song = failSong;
      songLength = sizeof(failSong) / sizeof(failSong[0]);
      break;

    case RESET:
      song = resetSong;
      songLength = sizeof(resetSong) / sizeof(resetSong[0]);
      break;

    default:
      song = nullptr;
      songLength = 0;
      break;
  }

  currentNote = 0;
  notePlaying = false;
  played = false;
  noTone(buzzerPin);
}

bool playSong() {
  unsigned long currentTime = millis();

Serial.print("Note: "); Serial.print(song[currentNote][0]);
Serial.print(" Dur: "); Serial.print(song[currentNote][1]);
Serial.print(" Time: "); Serial.print(currentTime);
Serial.print(" Start: "); Serial.println(noteStartTime);

  if (played) return played;

  if (!notePlaying) {
    int note = song[currentNote][0];
    int duration = song[currentNote][1];

    if (note != 0) {
      tone(buzzerPin, note);
    } else {
      noTone(buzzerPin);
    }

    noteStartTime = currentTime;
    notePlaying = true;
  }

  if (notePlaying && (currentTime - noteStartTime >= song[currentNote][1])) {
    noTone(buzzerPin);

    currentNote++;
    if (currentNote >= songLength) {
      currentNote = 0; // Loop song
      played = true;
    }
    notePlaying = false;
  }

  return(played);
}

void _playSong() {
  if (song == nullptr || songLength == 0) return;

  for (int i = 0; i < songLength; i++) {
    int note = song[i][0];
    int duration = song[i][1];

    if (note != NOTE_REST) {
      tone(buzzerPin, note, duration);
    } else {
      noTone(buzzerPin);
    }

    delay(duration);
    noTone(buzzerPin);  // Ensure silence between notes
    delay(20);          // Small pause between notes
  }

  played = true;
  currentNote = 0;
}


