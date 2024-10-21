![nubmoan banner](nubmoanBanner.png)

# nubmoan

Source code for nubmoan, the program that makes your ThinkPad TrackPoint (the red nub) moan when you press it.\
Original code can be found here: https://github.com/wttdotm/nubmoan

This nubmoan fork uses [SDL2](https://github.com/libsdl-org/SDL) for audio playback, [cargs](https://github.com/likle/cargs) for argument parsing and reads events from /dev/input/eventX.\
I have zero experience with SDl2 so this is probably questionable solution for the questionable task.\
Tested on Ubuntu 22.04.

# Build
```sh
cd ./nubmoan
pip install conan
conan profile detect --force
conan install . --output-folder=build --build=missing
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

# System configuration
```sh
sudo usermod -aG input $USER
su - $USER
```

# Find out your event device
## Easy way:
```sh
sudo apt-get install evtest
evtest
```
ctrl+c to terminate evtest.\
Use event device corresponding to the TrackPoint (TPPS/2 Elan TrackPoint in my case).
## Hard way:
Run ```cat /dev/input/eventX``` for each event device that is available.\
Move TrackPoint to check if this desired event device(it should print garbage in terminal on success).\
Press ctrl+c to terminate.

# Prepare sound files
Save moans to the basenameX.wav files, where ***basename*** is a common name and ***X*** is a number starting from 1.
## Generate wav files for testing
You can easily generate wav files for testing purposes using [Piper](https://github.com/rhasspy/piper) text to speech system:
```sh
pip install piper-tts
echo 'One!' | piper   --model en_US-lessac-medium   --output_file file1.wav
echo 'Two!' | piper   --model en_US-lessac-medium   --output_file file2.wav
echo 'Three!' | piper   --model en_US-lessac-medium   --output_file file3.wav
echo 'Four!' | piper   --model en_US-lessac-medium   --output_file file4.wav
echo 'Five!' | piper   --model en_US-lessac-medium   --output_file file5.wav
echo 'Six!' | piper   --model en_US-lessac-medium   --output_file file6.wav
```

# Usage
```sh
Usage: nubmoan -d /dev/input/event14 -f ./file -n 6 -l 10

  -d, --dev=EVDEV_PATH            TrackPoint event device e.g. "/dev/input/event14"
  -f, --file=SOUND_FILE           Sound file basename e.g. "./file" for ./fileX.wav (X is a number 1..NUM_FILES)
  -n, --num_files=NUM_FILES       Number of available sound files e.g. "6"
  -l, --lim=EVENT_COUNT_LIMIT     Number of events to track before playing a sound e.g. "10"
  -h, --help                      Shows the command help

```

# Run
```sh
cd ./build
./nubmoan -d /dev/input/event14 -f ./file -n 6 -l 10
```

# Cleanup
After you done "playing" ~~wash your hands~~ remove your user from the input group:

```sh
sudo deluser $USER input
su - $USER
```
