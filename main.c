/* 
 *  Squeezelite - lightweight headless squeezebox emulator
 *
 *  (c) Adrian Smith 2012-2015, triode1@btinternet.com
 *  
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "squeezelite.h"

#include <signal.h>

#define TITLE "Squeezelite Copyright 2012-2015 Adrian Smith.\n"
#define MODINFO "this modified version (" VERSION "), Copyright 2015 - 2024 Marco Curti."
#define MODINFO2 "see https://github.com/marcoc1712/squeezelite-R2\n"
#define CODECS_BASE "flac,pcm,mp3,ogg"
#if FAAD
#define CODECS_FAAD   ",aac"
#else
#define CODECS_FAAD   ""
#endif
#if FFMPEG
#define CODECS_FF   ",wma,alac"
#else
#define CODECS_FF   ""
#endif
#if DSD
#define CODECS_DSD  ",dsd"
#else
#define CODECS_DSD  ""
#endif
#define CODECS_MP3  " (mad,mpg for specific mp3 codec)"

#define CODECS CODECS_BASE CODECS_FF CODECS_DSD CODECS_MP3

static void usage(const char *argv0) {
	printf(TITLE "\n" MODINFO "\n" MODINFO2
		   "\n\nSee -t for license terms\n\n"
		   "Usage: %s [options]\n"
		   "  -s <server>[:<port>]\tConnect to specified server, otherwise uses autodiscovery to find server\n"
		   "  -o <output device>\tSpecify output device, default \"default\", - = output to stdout\n"
		   "  -l \t\t\tList output devices\n"
		   "  -x \t\t\tDisable downsampling requests to LMS\n"
#if ALSA
		   "  -a <b>:<p>:<f>:<m>\tSpecify ALSA params to open output device, b = buffer time in ms or size in bytes, p = period count or size in bytes, f sample format (16|24|24_3|32), m = use mmap (0|1)\n"
#endif
#if PORTAUDIO
#if OSX
		   "  -a <l>:<r>\t\tSpecify Portaudio params to open output device, l = target latency in ms, r = allow OSX to resample (0|1)\n"
#else
		   "  -a <l>\t\tSpecify Portaudio params to open output device, l = target latency in ms\n"
#endif
#endif
		   "  -a <f>\t\tSpecify sample format (16|24|32) of output file when using -o - to output samples to stdout (interleaved little endian only)\n"
		   "  -b <stream>:<output>\tSpecify internal Stream and Output buffer sizes in Kbytes\n"
		   "  -c <codec1>,<codec2>\tRestrict codecs to those specified, otherwise load all available codecs; known codecs: " CODECS "\n"
		   "  -C <timeout>\t\tClose output device when idle after timeout seconds, default is to keep it open while player is 'on'\n"
#if !IR
		   "  -d <log>=<level>\tSet logging level, logs: all|slimproto|stream|decode|output, level: info|debug|sdebug\n"
#else
		   "  -d <log>=<level>\tSet logging level, logs: all|slimproto|stream|decode|output|ir, level: info|debug|sdebug\n"
#endif
		   "  -e <codec1>,<codec2>\tExplicitly exclude native support of one or more codecs; known codecs: " CODECS "\n"
		   "  -f <logfile>\t\tWrite debug to logfile\n"
#if IR
		   "  -i [<filename>]\tEnable lirc remote control support (lirc config file ~/.lircrc used if filename not specified)\n"
#endif
		   "  -m <mac addr>\t\tSet mac address, format: ab:cd:ef:12:34:56\n"
		   "  -M <modelname>\tSet the squeezelite player model name sent to the server (default: " MODEL_NAME_STRING ")\n"
		   "  -n <name>\t\tSet the player name\n"
		   "  -N <filename>\t\tStore player name in filename to allow server defined name changes to be shared between servers (not supported with -n)\n"
#if ALSA
		   "  -p <priority>\t\tSet real time priority of output thread (1-99)\n"
#endif
#if LINUX || FREEBSD
		   "  -P <filename>\t\tStore the process id (PID) in filename\n"
#endif
		   "  -r <rates>[:<delay>]\tSample rates supported, allows output to be off when squeezelite is started; rates = <maxrate>|<minrate>-<maxrate>|<rate1>,<rate2>,<rate3>; delay = optional delay switching rates in ms\n"
#if RESAMPLE
		   "  -R -u [params]\tResample, params = <recipe>:<flags>:<attenuation>:<precision>:<passband_end>:<stopband_start>:<phase_response>,\n" 
		   "  \t\t\t recipe = (v|h|m|l|q)(L|I|M)(s) [E|X], E = exception - resample only if native rate not supported, X = async - resample to max rate for device, otherwise to max sync rate\n"
		   "  \t\t\t flags = num in hex,\n"
		   "  \t\t\t attenuation = attenuation in dB to apply (default is -1db if not explicitly set),\n"
		   "  \t\t\t precision = number of bits precision (NB. HQ = 20. VHQ = 28),\n"
		   "  \t\t\t passband_end = number in percent (0dB pt. bandwidth to preserve. nyquist = 100%%),\n"
		   "  \t\t\t stopband_start = number in percent (Aliasing/imaging control. > passband_end),\n"
		   "  \t\t\t phase_response = 0-100 (0 = minimum / 50 = linear / 100 = maximum)\n"
#endif
#if DSD
#if ALSA
		   "  -D [delay][:format]\tOutput device supports DSD, delay = optional delay switching between PCM and DSD in ms\n"
		   "  \t\t\t format = dop (default if not specified), u8, u16le, u16be, u32le or u32be.\n"
#else
		   "  -D [delay]\t\tOutput device supports DSD over PCM (DoP), delay = optional delay switching between PCM and DoP in ms\n"
#endif
#endif
#if VISEXPORT
		   "  -v \t\t\tVisualiser support\n"
#endif
# if ALSA
		   "  -L \t\t\tList volume controls for output device\n"
		   "  -U <control>\t\tUnmute ALSA control and set to full volume (not supported with -V)\n"
		   "  -V <control>\t\tUse ALSA control for volume adjustment, otherwise use software volume adjustment\n"
#endif
#if LINUX || FREEBSD
		   "  -z \t\t\tDaemonize\n"
#endif
		   "  -t \t\t\tLicense terms\n"
		   "  -? \t\t\tDisplay this help text\n"
		   "\n"
		   "Build options:"
#if LINUX
		   " LINUX"
#endif
#if WIN
		   " WIN"
#endif
#if OSX
		   " OSX"
#endif
#if FREEBSD
		   " FREEBSD"
#endif
#if ALSA
		   " ALSA"
#endif
#if PORTAUDIO
		   " PORTAUDIO"
#endif
#if EVENTFD
		   " EVENTFD"
#endif
#if SELFPIPE
		   " SELFPIPE"
#endif
#if WINEVENT
		   " WINEVENT"
#endif
#if RESAMPLE_MP
		   " RESAMPLE_MP"
#else
#if RESAMPLE
		   " RESAMPLE"
#endif
#endif
#if FAAD
		   " FAAD"
#endif
#if FFMPEG
		   " FFMPEG"
#endif
#if VISEXPORT
		   " VISEXPORT"
#endif
#if IR
		   " IR"
#endif
#if DSD
		   " DSD"
#endif
#if LINKALL
		   " LINKALL"
#endif
		   "\n\n",
		   argv0);
}

static void license(void) {
	printf(TITLE "\n" MODINFO "\n" MODINFO2 "\n"
		   "-x option patch, to disable LMS downsampling, (c) 2015 Daphile\n"
		   "dsd native playback on alsa linux, (c) 2015 Daphile\n"
		   "libflac12 cand ogf suport 2024 Ralph Irving 2015-2024, ralph_irving@hotmail.com\n\n"
		   "This program is free software: you can redistribute it and/or modify\n"
		   "it under the terms of the GNU General Public License as published by\n"
		   "the Free Software Foundation, either version 3 of the License, or\n"
		   "(at your option) any later version.\n\n"
		   "This program is distributed in the hope that it will be useful,\n"
		   "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		   "GNU General Public License for more details.\n\n"
		   "You should have received a copy of the GNU General Public License\n"
		   "along with this program.  If not, see <http://www.gnu.org/licenses/>.\n\n"
#if DSD		   
		   "Contains dsd2pcm library Copyright 2009, 2011 Sebastian Gesemann which\n"
		   "is subject to its own license.\n\n"
#endif
		   );
}

static void sighandler(int signum) {
	slimproto_stop();

	// remove ourselves in case above does not work, second SIGINT will cause non gracefull shutdown
	signal(signum, SIG_DFL);
}

int main(int argc, char **argv) {
	char *server = NULL;
	char *output_device = "default";
	char *include_codecs = NULL;
	char *exclude_codecs = "";
	char *name = NULL;
	char *namefile = NULL;
	char *modelname = NULL;
	char *logfile = NULL;
	u8_t mac[6];
	unsigned stream_buf_size = STREAMBUF_SIZE;
	unsigned output_buf_size = 0; // set later
	unsigned rates[MAX_SUPPORTED_SAMPLERATES] = { 0 };
	unsigned rate_delay = 0;
	char *resample = NULL;
	char *output_params = NULL;
	unsigned idle = 0;
	bool lms_downsample = true;
#if LINUX || FREEBSD
	bool daemonize = false;
	char *pidfile = NULL;
	FILE *pidfp = NULL;
#endif
#if ALSA
	unsigned rt_priority = OUTPUT_RT_PRIORITY;
	char *output_mixer = NULL;
	bool output_mixer_unmute = false;
#endif
#if DSD
	unsigned dsd_delay = 0;
	dsd_format dsd_outfmt = PCM;
#endif
#if VISEXPORT
	bool visexport = false;
#endif
#if IR
	char *lircrc = NULL;
#endif
	
	log_level log_output = lWARN;
	log_level log_stream = lWARN;
	log_level log_decode = lWARN;
	log_level log_slimproto = lWARN;
#if IR
	log_level log_ir     = lWARN;
#endif

	char *optarg = NULL;
	int optind = 1;
	int i;

#define MAXCMDLINE 512
	char cmdline[MAXCMDLINE] = "";

	get_mac(mac);

	for (i = 0; i < argc && (strlen(argv[i]) + strlen(cmdline) + 2 < MAXCMDLINE); i++) {
		strcat(cmdline, argv[i]);
		strcat(cmdline, " ");
	}

	while (optind < argc && strlen(argv[optind]) >= 2 && argv[optind][0] == '-') {
		char *opt = argv[optind] + 1;
		if (strstr("oabcCdefmMnNpPrs"
#if ALSA
				   "UV"
#endif
				   , opt) && optind < argc - 1) {
			optarg = argv[optind + 1];
			optind += 2;
		} else if (strstr("ltxz?"
#if ALSA
						  "L"
#endif
#if RESAMPLE
						  "uR"
#endif
#if DSD
						  "D"
#endif
#if VISEXPORT
						  "v"
#endif
#if IR
						  "i"
#endif

						  , opt)) {
			optarg = NULL;
			optind += 1;
		} else {
			fprintf(stderr, "\nOption error: -%s\n\n", opt);
			usage(argv[0]);
			exit(1);
		}

		switch (opt[0]) {
		case 'o':
			output_device = optarg;
			break;
		case 'a':
			output_params = optarg;
			break;
		case 'b': 
			{
				char *s = next_param(optarg, ':');
				char *o = next_param(NULL, ':');
				if (s) stream_buf_size = atoi(s) * 1024;
				if (o) output_buf_size = atoi(o) * 1024;
			}
			break;
		case 'c':
			include_codecs = optarg;
			break;
		case 'C':
			if (atoi(optarg) > 0) {
				idle = atoi(optarg) * 1000;
			}
			break;
		case 'e':
			exclude_codecs = optarg;
			break;
		case 'd':
			{
				char *l = strtok(optarg, "=");
				char *v = strtok(NULL, "=");
				log_level new = lWARN;
				if (l && v) {
					if (!strcmp(v, "info"))   new = lINFO;
					if (!strcmp(v, "debug"))  new = lDEBUG;
					if (!strcmp(v, "sdebug")) new = lSDEBUG;
					if (!strcmp(l, "all") || !strcmp(l, "slimproto")) log_slimproto = new;
					if (!strcmp(l, "all") || !strcmp(l, "stream"))    log_stream = new;
					if (!strcmp(l, "all") || !strcmp(l, "decode"))    log_decode = new;
					if (!strcmp(l, "all") || !strcmp(l, "output"))    log_output = new;
#if IR
					if (!strcmp(l, "all") || !strcmp(l, "ir"))        log_ir     = new;
#endif
				} else {
					fprintf(stderr, "\nDebug settings error: -d %s\n\n", optarg);
					usage(argv[0]);
					exit(1);
				}
			}
			break;
		case 'f':
			logfile = optarg;
			break;
		case 'm':
			{
				int byte = 0;
				char *tmp;
				if (!strncmp(optarg, "00:04:20", 8)) {
					LOG_ERROR("ignoring mac address from hardware player range 00:04:20:**:**:**");
				} else {
					char *t = strtok(optarg, ":");
					while (t && byte < 6) {
						mac[byte++] = (u8_t)strtoul(t, &tmp, 16);
						t = strtok(NULL, ":");
					}
				}
			}
			break;
		case 'M':
			modelname = optarg;
			break;
		case 'r':
			{ 
				char *rstr = next_param(optarg, ':');
				char *dstr = next_param(NULL, ':');
				if (rstr && strstr(rstr, ",")) {
					// parse sample rates and sort them
					char *r = next_param(rstr, ',');
					unsigned tmp[MAX_SUPPORTED_SAMPLERATES] = { 0 };
					int i, j;
					int last = 999999;
					for (i = 0; r && i < MAX_SUPPORTED_SAMPLERATES; ++i) { 
						tmp[i] = atoi(r);
						r = next_param(NULL, ',');
					}
					for (i = 0; i < MAX_SUPPORTED_SAMPLERATES; ++i) {
						int largest = 0;
						for (j = 0; j < MAX_SUPPORTED_SAMPLERATES; ++j) {
							if (tmp[j] > largest && tmp[j] < last) {
								largest = tmp[j];
							}
						}
						rates[i] = last = largest;
					}
				} else if (rstr) {
					// optstr is <min>-<max> or <max>, extract rates from test rates within this range
					unsigned ref[] TEST_RATES;
					char *str1 = next_param(rstr, '-');
					char *str2 = next_param(NULL, '-');
					unsigned max = str2 ? atoi(str2) : (str1 ? atoi(str1) : ref[0]);
					unsigned min = str1 && str2 ? atoi(str1) : 0;
					unsigned tmp;
					int i, j;
					if (max < min) { tmp = max; max = min; min = tmp; }
					rates[0] = max;
					for (i = 0, j = 1; i < MAX_SUPPORTED_SAMPLERATES; ++i) {
						if (ref[i] < rates[j-1] && ref[i] >= min) {
							rates[j++] = ref[i];
						}
					}
				}
				if (dstr) {
					rate_delay = atoi(dstr);
				}
			}
			break;
		case 's':
			server = optarg;
			break;
		case 'n':
			name = optarg;
			break;
		case 'N':
			namefile = optarg;
			break;
		case 'x':
			lms_downsample = false;
			break;
#if ALSA
		case 'p':
			rt_priority = atoi(optarg);
			if (rt_priority > 99 || rt_priority < 1) {
				fprintf(stderr, "\nError: invalid priority: %s\n\n", optarg);
				usage(argv[0]);
				exit(1);
			}
			break;
#endif
#if LINUX || FREEBSD
		case 'P':
			pidfile = optarg;
			break;
#endif
		case 'l':
			list_devices();
			exit(0);
			break;
#if ALSA
		case 'L':
			list_mixers(output_device);
			exit(0);
			break;
#endif
#if RESAMPLE
		case 'u':
		case 'R':
			if (optind < argc && argv[optind] && argv[optind][0] != '-') {
				resample = argv[optind++];
			} else {
				resample = "";
			}
			break;
#endif
#if DSD
		case 'D':
			dsd_outfmt = DOP;
			if (optind < argc && argv[optind] && argv[optind][0] != '-') {
				char *dstr = next_param(argv[optind++], ':');
				char *fstr = next_param(NULL, ':');
				dsd_delay = dstr ? atoi(dstr) : 0;
				if (fstr) {
					if (!strcmp(fstr, "dop")) dsd_outfmt = DOP; 
					if (!strcmp(fstr, "u8")) dsd_outfmt = DSD_U8; 
					if (!strcmp(fstr, "u16le")) dsd_outfmt = DSD_U16_LE; 
					if (!strcmp(fstr, "u32le")) dsd_outfmt = DSD_U32_LE; 
					if (!strcmp(fstr, "u16be")) dsd_outfmt = DSD_U16_BE; 
					if (!strcmp(fstr, "u32be")) dsd_outfmt = DSD_U32_BE;
					if (!strcmp(fstr, "dop24")) dsd_outfmt = DOP_S24_LE;
					if (!strcmp(fstr, "dop24_3")) dsd_outfmt = DOP_S24_3LE;
				}
			}
			break;
#endif
#if VISEXPORT
		case 'v':
			visexport = true;
			break;
#endif
#if ALSA
		case 'U':
			output_mixer_unmute = true;
		case 'V':
			if (output_mixer) {
				fprintf(stderr, "-U and -V option should not be used at same time\n");
				exit(1);
			}
			output_mixer = optarg;
			break;
#endif
#if IR
		case 'i':
			if (optind < argc && argv[optind] && argv[optind][0] != '-') {
				lircrc = argv[optind++];
			} else {
				lircrc = "~/.lircrc"; // liblirc_client will expand ~/
			}
			break;
#endif
#if LINUX || FREEBSD
		case 'z':
			daemonize = true;
			break;
#endif
		case 't':
			license();
			exit(0);
		case '?':
			usage(argv[0]);
			exit(0);
		default:
			fprintf(stderr, "Arg error: %s\n", argv[optind]);
			break;
		}
	}

	// warn if command line includes something which isn't parsed
	if (optind < argc) {
		fprintf(stderr, "\nError: command line argument error\n\n");
		usage(argv[0]);
		exit(1);
	}

	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);
#if defined(SIGQUIT)
	signal(SIGQUIT, sighandler);
#endif
#if defined(SIGHUP)
	signal(SIGHUP, sighandler);
#endif

	// set the output buffer size if not specified on the command line, take account of resampling
	if (!output_buf_size) {
		output_buf_size = OUTPUTBUF_SIZE;
		if (resample) {
			unsigned scale = 8;
			if (rates[0]) {
				scale = rates[0] / 44100;
				if (scale > 8) scale = 8;
				if (scale < 1) scale = 1;
			}
			output_buf_size *= scale;
		}
	}

	if (logfile) {
		if (!freopen(logfile, "a", stderr)) {
			fprintf(stderr, "error opening logfile %s: %s\n", logfile, strerror(errno));
		} else {
			if (log_output >= lINFO || log_stream >= lINFO || log_decode >= lINFO || log_slimproto >= lINFO) {
				fprintf(stderr, "\n%s\n", cmdline);
			}
		}
	}

#if LINUX || FREEBSD
	if (pidfile) {
		if (!(pidfp = fopen(pidfile, "w")) ) {
			fprintf(stderr, "Error opening pidfile %s: %s\n", pidfile, strerror(errno));
			exit(1);
		}
		pidfile = realpath(pidfile, NULL); // daemonize will change cwd
	}

	if (daemonize) {
		if (daemon(0, logfile ? 1 : 0)) {
			fprintf(stderr, "error daemonizing: %s\n", strerror(errno));
		}
	}

	if (pidfp) {
		fprintf(pidfp, "%d\n", getpid());
		fclose(pidfp);
	}
#endif

#if WIN
	winsock_init();
#endif

	stream_init(log_stream, stream_buf_size);

	if (!strcmp(output_device, "-")) {
		output_init_stdout(log_output, output_buf_size, output_params, rates, rate_delay);
	} else {
#if ALSA
		output_init_alsa(log_output, output_device, output_buf_size, output_params, rates, rate_delay, rt_priority, idle, output_mixer,
						 output_mixer_unmute);
#endif
#if PORTAUDIO
		output_init_pa(log_output, output_device, output_buf_size, output_params, rates, rate_delay, idle);
#endif
	}

#if DSD
	dsd_init(dsd_outfmt, dsd_delay);
#endif

#if VISEXPORT
	if (visexport) {
		output_vis_init(log_output, mac);
	}
#endif

	decode_init(log_decode, include_codecs, exclude_codecs);

#if RESAMPLE
	if (resample) {
		process_init(resample);
	}
#endif

#if IR
	if (lircrc) {
		ir_init(log_ir, lircrc);
	}
#endif

	if (name && namefile) {
		fprintf(stderr, "-n and -N option should not be used at same time\n");
		exit(1);
	}

	slimproto(log_slimproto, server, mac, name, namefile, modelname, lms_downsample);

	decode_close();
	stream_close();

	if (!strcmp(output_device, "-")) {
		output_close_stdout();
	} else {
#if ALSA
		output_close_alsa();
#endif
#if PORTAUDIO
		output_close_pa();
#endif
	}

#if IR
	ir_close();
#endif

#if WIN
	winsock_close();
#endif

#if LINUX || FREEBSD
	if (pidfile) {
		unlink(pidfile);
		free(pidfile);
	}
#endif

	exit(0);
}
