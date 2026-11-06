static const int poll_interval = 20;
static const int poll_timeout  = 500;
static const int level_low     = 15;
static const int level_crit    = 5;
static const int default_scale = 1;
static const int default_grab  = 1;

static const int popup_width   = 440;
static const int popup_height  = 176;
static const int border_width  = 3;

static const int y_title       = 20;
static const int y_percent     = 62;
static const int y_action      = 132;
static const int scale_title   = 4;
static const int scale_percent = 8;
static const int scale_action  = 3;

static const uint32_t color_bg          = 0xff000000;
static const uint32_t color_text        = 0xffffffff;
static const uint32_t color_low_accent  = 0xff7e56c3;
static const uint32_t color_crit_accent = 0xff8000ff;

static const char *bat_path     = "/sys/class/power_supply/BAT0";
static const char *msg_low      = "BATTERY LOW";
static const char *msg_critical = "BATTERY CRITICAL";
static const char *msg_action   = "PLUG IN CHARGER";
