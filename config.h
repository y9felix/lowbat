enum {
	POLL_INTERVAL = 20,
	POLL_TIMEOUT  = 500,
	LEVEL_LOW     = 15,
	LEVEL_CRIT    = 5,
	DEFAULT_SCALE = 1,
	DEFAULT_GRAB  = 1,
	POPUP_WIDTH   = 440,
	POPUP_HEIGHT  = 176,
	BORDER_WIDTH  = 3,
	Y_TITLE       = 20,
	Y_PERCENT     = 62,
	Y_ACTION      = 132,
	SCALE_TITLE   = 4,
	SCALE_PERCENT = 8,
	SCALE_ACTION  = 3
};

#define COLOR_BG          0xff000000
#define COLOR_TEXT        0xffffffff
#define COLOR_LOW_ACCENT  0xff7e56c3
#define COLOR_CRIT_ACCENT 0xff8000ff

#define BAT_PATH          "/sys/class/power_supply/BAT0"
#define MSG_LOW           "BATTERY LOW"
#define MSG_CRITICAL      "BATTERY CRITICAL"
#define MSG_ACTION        "PLUG IN CHARGER"
