#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <SDL.h>
#include <cargs.h>

#define EVENT_COUNT_LIMIT 10 // Number of events to track before playing a sound

static struct cag_option options[] = {
    {.identifier = 'd',
     .access_letters = "d",
     .access_name = "dev",
     .value_name = "EVDEV_PATH",
     .description = "TrackPoint event device e.g. \"/dev/input/event14\""},

    {.identifier = 'f',
     .access_letters = "f",
     .access_name = "file",
     .value_name = "SOUND_FILE",
     .description = "Sound file basename e.g. \"./file\" for ./fileX.wav (X is a number 1..NUM_FILES)"},

    {.identifier = 'n',
     .access_letters = "n",
     .access_name = "num_files",
     .value_name = "NUM_FILES",
     .description = "Number of available sound files e.g. \"6\""},

    {.identifier = 'l',
     .access_letters = "l",
     .access_name = "lim",
     .value_name = "EVENT_COUNT_LIMIT",
     .description = "Number of events to track before playing a sound e.g. \"10\""},

    {.identifier = 'h',
     .access_letters = "h",
     .access_name = "help",
     .description = "Shows the command help"}};

int get_level(int mov, int mov_min, int mov_max, int max_level) {
    int tmp = (int)round((float)(max_level * (mov - mov_min)) / (float)(mov_max - mov_min));
    if (tmp <= 0)
        tmp = 1;
    if (tmp > max_level)
        tmp = max_level;
    return (tmp);
}

// https://discourse.libsdl.org/t/duration-of-a-loaded-wav-file/43548/2
int get_wav_duration_ms(const SDL_AudioSpec *spec, int wav_length) {
    uint32_t sampleSize = SDL_AUDIO_BITSIZE(spec->format) / 8;
    uint32_t sampleCount = wav_length / sampleSize;
    // could do a sanity check and make sure (audioLen % sampleSize) is 0
    uint32_t sampleLen = 0;
    if (spec->channels) {
        sampleLen = sampleCount / spec->channels;
    } else {
        // spec.channels *should* be 1 or higher, but just in case
        sampleLen = sampleCount;
    }
    // seconds = (double)sampleLen / (double)wavSpec.freq;
    return (int)round(((double)sampleLen / (double)spec->freq) * 1000);
}

void print_level(int level, int max) {
    printf("[");
    for (size_t i = 1; i <= max; i++) {
        if (i <= level)
        {
            printf("*");
        }
        else
            printf(" ");
    }
    printf("]\n");
}

void play_wav(const char *filename) {
    SDL_AudioSpec wavSpec;
    Uint32 wavLength;
    Uint8 *wavBuffer;
    SDL_LoadWAV(filename, &wavSpec, &wavBuffer, &wavLength);
    SDL_AudioDeviceID deviceId = SDL_OpenAudioDevice(NULL, 0, &wavSpec, NULL, 0);
    int success = SDL_QueueAudio(deviceId, wavBuffer, wavLength);
    SDL_PauseAudioDevice(deviceId, 0);
    SDL_Delay(get_wav_duration_ms(&wavSpec, wavLength));
    SDL_CloseAudioDevice(deviceId);
    SDL_FreeWAV(wavBuffer);
}

void flush_input(int fd) {
    fd_set set;
    struct timeval timeout;
    int ret;
    struct input_event gc;
    while (1) {
        FD_ZERO(&set);
        FD_SET(fd, &set);
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
        ret = select(fd + 1, &set, NULL, NULL, &timeout);
        if (ret == -1) {
            perror("evdev select");
            break;
        }
        else if (ret == 0) {
            break;
        }
        if (FD_ISSET(fd, &set)) {
            ret = read(fd, &gc, sizeof(struct input_event));
        }
    }
}

int main(int argc, char *argv[]) {
    bool simple_flag = false, multiple_flag = false, long_flag = false;
    const char *evdev = NULL;
    const char *sound_file_path = NULL;
    int num_files = 0;
    int event_count_limit = EVENT_COUNT_LIMIT;
    int param_index;

    cag_option_context context;
    cag_option_init(&context, options, CAG_ARRAY_SIZE(options), argc, argv);
    while (cag_option_fetch(&context)) {
        switch (cag_option_get_identifier(&context)) {
        case 'd':
            evdev = cag_option_get_value(&context);
            break;
        case 'f':
            sound_file_path = cag_option_get_value(&context);
            break;
        case 'n':
            num_files = strtol(cag_option_get_value(&context), NULL, 10);
            break;
        case 'l':
            event_count_limit = strtol(cag_option_get_value(&context), NULL, 10);
            break;
        case 'h':
            printf("Usage: nubmoan -d /dev/input/event14 -f ./file -n 6 -l 10\n\n");
            cag_option_print(options, CAG_ARRAY_SIZE(options), stdout);
            return EXIT_SUCCESS;
        case '?':
            cag_option_print_error(&context, stdout);
            return 0;
            break;
        }
    }

    int fd;
    if ((fd = open(evdev, O_RDONLY)) < 0) {
        perror("evdev open");
        exit(1);
    }

    struct input_event ev;
    int cumulativeMovement = 0; // Track the cumulative movement
    int cumulativeMovementMax = 2 * EVENT_COUNT_LIMIT;
    int eventCounter = 0; // Counter for mouse events after cooldown
    int level = 1;

    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_Init(SDL_INIT_AUDIO);

    while (1) {
        int ret;
        ret = read(fd, &ev, sizeof(struct input_event));
        if ((ev.value != 0) && (ev.type == 2)) {
            cumulativeMovement += abs(ev.value);
            eventCounter++;
        }
        if (eventCounter >= EVENT_COUNT_LIMIT) {
            level = get_level(cumulativeMovement, EVENT_COUNT_LIMIT, cumulativeMovementMax, num_files);
            print_level(level, num_files);
            if (cumulativeMovement > cumulativeMovementMax) {
                cumulativeMovementMax = cumulativeMovement;
            }
            char filename[128];
            snprintf(filename, sizeof(filename), "%s%d.wav", sound_file_path, level);
            play_wav(filename);
            cumulativeMovement = 0;
            eventCounter = 0;
            flush_input(fd);
        }
    }
    SDL_Quit();
    return 0;
}
