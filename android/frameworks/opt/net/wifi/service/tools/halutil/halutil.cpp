#include <stdint.h>
#include <stdlib.h>

#include "wifi_hal.h"

#define LOG_TAG  "WifiHAL"

#include <utils/Log.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <ctype.h>
#include <stdarg.h>

pthread_mutex_t printMutex;
void printMsg(const char *fmt, ...)
{
    pthread_mutex_lock(&printMutex);
    va_list l;
    va_start(l, fmt);

    vprintf(fmt, l);
    va_end(l);
    pthread_mutex_unlock(&printMutex);
}

template<typename T, unsigned N>
unsigned countof(T (&rgt)[N]) {
    return N;
}

template<typename T>
T min(const T& t1, const T& t2) {
    return (t1 < t2) ? t1 : t2;
}

#define EVENT_BUF_SIZE 2048
#define MAX_CH_BUF_SIZE  64
#define MAX_FEATURE_SET  8
#define HOTLIST_LOST_WINDOW  5

static wifi_handle halHandle;
static wifi_interface_handle *ifaceHandles;
static wifi_interface_handle wlan0Handle;
static wifi_interface_handle p2p0Handle;
static int numIfaceHandles;
static int cmdId = 0;
static int ioctl_sock = 0;
static int max_event_wait = 5;
static int stest_max_ap = 10;
static int stest_base_period = 5000;
static int stest_threshold = 80;
static int swctest_rssi_sample_size =  3;
static int swctest_rssi_lost_ap =  3;
static int swctest_rssi_min_breaching =  2;
static int swctest_rssi_ch_threshold =  1;
static int htest_low_threshold =  90;
static int htest_high_threshold =  10;
static int rtt_samples = 30;
static wifi_band band = WIFI_BAND_UNSPECIFIED;

mac_addr hotlist_bssids[16];
unsigned char mac_oui[3];
int channel_list[16];
int num_hotlist_bssids = 0;
int num_channels = 0;

void parseMacAddress(const char *str, mac_addr addr);

int linux_set_iface_flags(int sock, const char *ifname, int dev_up)
{
    struct ifreq ifr;
    int ret;

    printMsg("setting interface %s flags (%s)\n", ifname, dev_up ? "UP" : "DOWN");

    if (sock < 0) {
      printMsg("Bad socket: %d\n", sock);
      return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strlcpy(ifr.ifr_name, ifname, IFNAMSIZ);

    printMsg("reading old value\n");

    if (ioctl(sock, SIOCGIFFLAGS, &ifr) != 0) {
      ret = errno ? -errno : -999;
      printMsg("Could not read interface %s flags: %d\n", ifname, errno);
      return ret;
    }else {
      printMsg("writing new value\n");
    }

    if (dev_up) {
      if (ifr.ifr_flags & IFF_UP) {
        printMsg("interface %s is already up\n", ifname);
        return 0;
      }
      ifr.ifr_flags |= IFF_UP;
    }else {
      if (!(ifr.ifr_flags & IFF_UP)) {
        printMsg("interface %s is already down\n", ifname);
        return 0;
      }
      ifr.ifr_flags &= ~IFF_UP;
    }

    if (ioctl(sock, SIOCSIFFLAGS, &ifr) != 0) {
      printMsg("Could not set interface %s flags \n", ifname);
      return ret;
    }else {
      printMsg("set interface %s flags (%s)\n", ifname, dev_up ? "UP" : "DOWN");
    }
    printMsg("Done\n");
    return 0;
}


static int init() {

    ioctl_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (ioctl_sock < 0) {
      printMsg("Bad socket: %d\n", ioctl_sock);
      return errno;
    } else {
      printMsg("Good socket: %d\n", ioctl_sock);
    }

    int ret = linux_set_iface_flags(ioctl_sock, "wlan0", 1);
    if (ret < 0) {
        return ret;
    }

    wifi_error res = wifi_initialize(&halHandle);
    if (res < 0) {
        return res;
    }

    res = wifi_get_ifaces(halHandle, &numIfaceHandles, &ifaceHandles);
    if (res < 0) {
        return res;
    }

    char buf[EVENT_BUF_SIZE];
    for (int i = 0; i < numIfaceHandles; i++) {
        if (wifi_get_iface_name(ifaceHandles[i], buf, sizeof(buf)) == WIFI_SUCCESS) {
            if (strcmp(buf, "wlan0") == 0) {
                printMsg("found interface %s\n", buf);
                wlan0Handle = ifaceHandles[i];
            } else if (strcmp(buf, "p2p0") == 0) {
                printMsg("found interface %s\n", buf);
                p2p0Handle = ifaceHandles[i];
            }
        }
    }

    return res;
}

static void cleaned_up_handler(wifi_handle handle) {
    printMsg("HAL cleaned up handler\n");
    halHandle = NULL;
    ifaceHandles = NULL;
}

static void cleanup() {
    printMsg("cleaning up HAL\n");
    wifi_cleanup(halHandle, cleaned_up_handler);
}

static void *eventThreadFunc(void *context) {

    printMsg("starting wifi event loop\n");
    wifi_event_loop(halHandle);
    printMsg("out of wifi event loop\n");

    return NULL;
}


static int getNewCmdId() {
    return cmdId++;
}

/* -------------------------------------------  */
/* helpers                                      */
/* -------------------------------------------  */

void printScanHeader() {
    printMsg("SSID\t\t\t\t\tBSSID\t\t  RSSI\tchannel\ttimestamp\tRTT\tRTT SD\n");
}

void printScanResult(wifi_scan_result result) {

    printMsg("%-32s\t", result.ssid);

    printMsg("%02x:%02x:%02x:%02x:%02x:%02x ", result.bssid[0], result.bssid[1],
            result.bssid[2], result.bssid[3], result.bssid[4], result.bssid[5]);

    printMsg("%d\t", result.rssi);
    printMsg("%d\t", result.channel);
    printMsg("%lld\t", result.ts);
    printMsg("%lld\t", result.rtt);
    printMsg("%lld\n", result.rtt_sd);
}

void printSignificantChangeResult(wifi_significant_change_result *res) {

    wifi_significant_change_result &result = *res;
    printMsg("%02x:%02x:%02x:%02x:%02x:%02x ", result.bssid[0], result.bssid[1],
            result.bssid[2], result.bssid[3], result.bssid[4], result.bssid[5]);

    printMsg("%d\t", result.channel);

    for (int i = 0; i < result.num_rssi; i++) {
        printMsg("%d,", result.rssi[i]);
    }
    printMsg("\n");
}

void printScanCapabilities(wifi_gscan_capabilities capabilities)
{
    printMsg("max_scan_cache_size = %d\n", capabilities.max_scan_cache_size);
    printMsg("max_scan_buckets = %d\n", capabilities.max_scan_buckets);
    printMsg("max_ap_cache_per_scan = %d\n", capabilities.max_ap_cache_per_scan);
    printMsg("max_rssi_sample_size = %d\n", capabilities.max_rssi_sample_size);
    printMsg("max_scan_reporting_threshold = %d\n", capabilities.max_scan_reporting_threshold);
    printMsg("max_hotlist_aps = %d\n", capabilities.max_hotlist_aps);
    printMsg("max_significant_wifi_change_aps = %d\n",
    capabilities.max_significant_wifi_change_aps);
}


/* -------------------------------------------  */
/* commands and events                          */
/* -------------------------------------------  */

typedef enum {
    EVENT_TYPE_SCAN_RESULTS_AVAILABLE = 1000,
    EVENT_TYPE_HOTLIST_AP_FOUND = 1001,
    EVENT_TYPE_SIGNIFICANT_WIFI_CHANGE = 1002,
    EVENT_TYPE_RTT_RESULTS = 1003,
    EVENT_TYPE_SCAN_COMPLETE = 1004,
    EVENT_TYPE_HOTLIST_AP_LOST = 1005
} EventType;

typedef struct {
    int type;
    char buf[256];
} EventInfo;

const int MAX_EVENTS_IN_CACHE = 256;
EventInfo eventCache[256];
int eventsInCache = 0;
pthread_cond_t eventCacheCondition;
pthread_mutex_t eventCacheMutex;

void putEventInCache(int type, const char *msg) {
    pthread_mutex_lock(&eventCacheMutex);
    if (eventsInCache + 1 < MAX_EVENTS_IN_CACHE) {
        eventCache[eventsInCache].type = type;
        strcpy(eventCache[eventsInCache].buf, msg);
        eventsInCache++;
        pthread_cond_signal(&eventCacheCondition);
        //printf("put new event in cache; size = %d\n", eventsInCache);
    } else {
        printf("Too many events in the cache\n");
    }
    pthread_mutex_unlock(&eventCacheMutex);
}

void getEventFromCache(EventInfo& info) {
    pthread_mutex_lock(&eventCacheMutex);
    while (true) {
        if (eventsInCache > 0) {
            //printf("found an event in cache; size = %d\n", eventsInCache);
            info.type = eventCache[0].type;
            strcpy(info.buf, eventCache[0].buf);
            eventsInCache--;
            memmove(&eventCache[0], &eventCache[1], sizeof(EventInfo) * eventsInCache);
            pthread_mutex_unlock(&eventCacheMutex);
            return;
        } else {
            pthread_cond_wait(&eventCacheCondition, &eventCacheMutex);
            //printf("pthread_cond_wait unblocked ...\n");
        }
    }
}

int numScanResultsAvailable = 0;
static void onScanResultsAvailable(wifi_request_id id, unsigned num_results) {
    printMsg("Received scan results available event\n");
    numScanResultsAvailable = num_results;
    putEventInCache(EVENT_TYPE_SCAN_RESULTS_AVAILABLE, "New scan results are available");
}

static void on_scan_event(wifi_scan_event event, unsigned status) {
    if (event == WIFI_SCAN_BUFFER_FULL) {
        printMsg("Received scan complete event - WIFI_SCAN_BUFFER_FULL \n");
    } else if(event == WIFI_SCAN_COMPLETE) {
        printMsg("Received scan complete event  - WIFI_SCAN_COMPLETE\n");
    }
}

static int scanCmdId;
static int hotlistCmdId;
static int significantChangeCmdId;
static int rttCmdId;

static bool startScan( void (*pfnOnResultsAvailable)(wifi_request_id, unsigned),
                       int max_ap_per_scan, int base_period, int report_threshold) {

    /* Get capabilties */
    wifi_gscan_capabilities capabilities;
    int result = wifi_get_gscan_capabilities(wlan0Handle, &capabilities);
    if (result < 0) {
        printMsg("failed to get scan capabilities - %d\n", result);
        printMsg("trying scan anyway ..\n");
    } else {
        printScanCapabilities(capabilities);
    }

    wifi_scan_cmd_params params;
    memset(&params, 0, sizeof(params));

    if(num_channels > 0){
        params.max_ap_per_scan = max_ap_per_scan;
        params.base_period = base_period;                      // 5 second by default
        params.report_threshold = report_threshold;
        params.num_buckets = 1;

        params.buckets[0].bucket = 0;
        params.buckets[0].band = WIFI_BAND_UNSPECIFIED;
        params.buckets[0].period = base_period;
        params.buckets[0].num_channels = num_channels;

        for(int i = 0; i < num_channels; i++){
            params.buckets[0].channels[i].channel = channel_list[i];
        }

    } else {

        /* create a schedule to scan channels 1, 6, 11 every 5 second and
         * scan 36, 40, 44, 149, 153, 157, 161 165 every 10 second */

      params.max_ap_per_scan = max_ap_per_scan;
      params.base_period = base_period;                      // 5 second
      params.report_threshold = report_threshold;
      params.num_buckets = 3;

      params.buckets[0].bucket = 0;
      params.buckets[0].band = WIFI_BAND_UNSPECIFIED;
      params.buckets[0].period = 5000;                // 5 second
      params.buckets[0].report_events = 0;
      params.buckets[0].num_channels = 2;

      params.buckets[0].channels[0].channel = 2412;
      params.buckets[0].channels[1].channel = 2437;

      params.buckets[1].bucket = 1;
      params.buckets[1].band = WIFI_BAND_A;
      params.buckets[1].period = 10000;               // 10 second
      params.buckets[1].report_events = 1;
      params.buckets[1].num_channels = 8;   // driver should ignore list since band is specified


      params.buckets[1].channels[0].channel = 5180;
      params.buckets[1].channels[1].channel = 5200;
      params.buckets[1].channels[2].channel = 5220;
      params.buckets[1].channels[3].channel = 5745;
      params.buckets[1].channels[4].channel = 5765;
      params.buckets[1].channels[5].channel = 5785;
      params.buckets[1].channels[6].channel = 5805;
      params.buckets[1].channels[7].channel = 5825;

      params.buckets[2].bucket = 2;
      params.buckets[2].band = WIFI_BAND_UNSPECIFIED;
      params.buckets[2].period = 15000;                // 15 second
      params.buckets[2].report_events = 2;
      params.buckets[2].num_channels = 1;

      params.buckets[2].channels[0].channel = 2462;

    }

    wifi_scan_result_handler handler;
    memset(&handler, 0, sizeof(handler));
    handler.on_scan_results_available = pfnOnResultsAvailable;
    handler.on_scan_event = on_scan_event;

    scanCmdId = getNewCmdId();
    printMsg("Starting scan --->\n");
    return wifi_start_gscan(scanCmdId, wlan0Handle, params, handler) == WIFI_SUCCESS;
}

static void stopScan() {
    wifi_request_id id = scanCmdId;
    if (id == 0)
        id = -1;

    wifi_stop_gscan(id, wlan0Handle);
    scanCmdId = 0;
}

wifi_scan_result *saved_scan_results;
unsigned max_saved_scan_results;
unsigned num_saved_scan_results;

static void on_single_shot_scan_event(wifi_scan_event event, unsigned status) {
    if (event == WIFI_SCAN_BUFFER_FULL) {
        printMsg("Received scan complete event - WIFI_SCAN_BUFFER_FULL \n");
    } else if(event == WIFI_SCAN_COMPLETE) {
        printMsg("Received scan complete event  - WIFI_SCAN_COMPLETE\n");
        putEventInCache(EVENT_TYPE_SCAN_COMPLETE, "One scan completed");
    }
}

static void on_full_scan_result(wifi_request_id id, wifi_scan_result *r) {
    if (num_saved_scan_results < max_saved_scan_results) {
        wifi_scan_result *result = &(saved_scan_results[num_saved_scan_results]);
        memcpy(result, r, sizeof(wifi_scan_result));
        //printMsg("Retrieved full scan result for %s(%02x:%02x:%02x:%02x:%02x:%02x)\n",
        //    result->ssid, result->bssid[0], result->bssid[1], result->bssid[2], result->bssid[3],
        //    result->bssid[4], result->bssid[5]);
        num_saved_scan_results++;
    }
}

static int scanOnce(wifi_band band, wifi_scan_result *results, int num_results) {

    saved_scan_results = results;
    max_saved_scan_results = num_results;
    num_saved_scan_results = 0;

    wifi_scan_cmd_params params;
    memset(&params, 0, sizeof(params));

    params.max_ap_per_scan = 10;
    params.base_period = 5000;                        // 5 second by default
    params.report_threshold = 90;
    params.num_buckets = 1;

    params.buckets[0].bucket = 0;
    params.buckets[0].band = band;
    params.buckets[0].period = 5000;                  // 5 second
    params.buckets[0].report_events = 2;              // REPORT_EVENTS_AFTER_EACH_SCAN
    params.buckets[0].num_channels = 0;

    wifi_scan_result_handler handler;
    memset(&handler, 0, sizeof(handler));
    handler.on_scan_results_available = NULL;
    handler.on_scan_event = on_single_shot_scan_event;
    handler.on_full_scan_result = on_full_scan_result;

    int scanCmdId = getNewCmdId();
    printMsg("Starting scan --->\n");
    if (wifi_start_gscan(scanCmdId, wlan0Handle, params, handler) == WIFI_SUCCESS) {
        int events = 0;
        while (true) {
            EventInfo info;
            memset(&info, 0, sizeof(info));
            getEventFromCache(info);
            if (info.type == EVENT_TYPE_SCAN_RESULTS_AVAILABLE
                || info.type == EVENT_TYPE_SCAN_COMPLETE) {
                int retrieved_num_results = num_saved_scan_results;
                if (retrieved_num_results == 0) {
                    printMsg("fetched 0 scan results, waiting for more..\n");
                    continue;
                } else {
                    printMsg("fetched %d scan results\n", retrieved_num_results);

                    /*
                    printScanHeader();

                    for (int i = 0; i < retrieved_num_results; i++) {
                        printScanResult(results[i]);
                    }
                    */

                    printMsg("Scan once completed, stopping scan\n");
                    wifi_stop_gscan(scanCmdId, wlan0Handle);
                    saved_scan_results = NULL;
                    max_saved_scan_results = 0;
                    num_saved_scan_results = 0;
                    return retrieved_num_results;
                }
            }
        }
    } else {
        return 0;
    }
}

static void retrieveScanResults() {

    wifi_scan_result results[256];
    memset(results, 0, sizeof(wifi_scan_result) * 256);
    printMsg("Retrieve Scan results available -->\n");
    int num_results = 256;
    int result = wifi_get_cached_gscan_results(wlan0Handle, 1, num_results, results, &num_results);
    if (result < 0) {
        printMsg("failed to fetch scan results : %d\n", result);
        return;
    } else {
        printMsg("fetched %d scan results\n", num_results);
    }

    printScanHeader();
    for (int i = 0; i < num_results; i++) {
        printScanResult(results[i]);
    }
}


static int compareScanResultsByRssi(const void *p1, const void *p2) {
    const wifi_scan_result *result1 = static_cast<const wifi_scan_result *>(p1);
    const wifi_scan_result *result2 = static_cast<const wifi_scan_result *>(p2);

    /* RSSI is -ve, so lower one wins */
    if (result1->rssi < result2->rssi) {
        return 1;
    } else if (result1->rssi == result2->rssi) {
        return 0;
    } else {
        return -1;
    }
}

static void sortScanResultsByRssi(wifi_scan_result *results, int num_results) {
    qsort(results, num_results, sizeof(wifi_scan_result), &compareScanResultsByRssi);
}

static int removeDuplicateScanResults(wifi_scan_result *results, int num) {
    /* remove duplicates by BSSID */
    int num_results = num;
    for (int i = 0; i < num_results; i++) {
        //printMsg("Processing result[%d] - %02x:%02x:%02x:%02x:%02x:%02x\n", i,
        //        results[i].bssid[0], results[i].bssid[1], results[i].bssid[2],
        //        results[i].bssid[3], results[i].bssid[4], results[i].bssid[5]);

        for (int j = i + 1; j < num_results; ) {
            if (memcmp(results[i].bssid, results[j].bssid, sizeof(mac_addr)) == 0) {
                /* 'remove' this scan result from the list */
                // printMsg("removing dupe entry\n");
                int num_to_move = num_results - j - 1;
                memmove(&results[j], &results[j+1], num_to_move * sizeof(wifi_scan_result));
                num_results--;
            } else {
                j++;
            }
        }

        // printMsg("num_results = %d\n", num_results);
    }

    return num_results;
}

static void onRTTResults (wifi_request_id id, unsigned num_results, wifi_rtt_result result[]) {

    printMsg("RTT results!!\n");
    printMsg("Addr\t\t\tts\t\tRSSI\tSpread\trtt\tsd\tspread\tdist\tsd\tspread\n");

    for (unsigned i = 0; i < num_results; i++) {
        printMsg("%02x:%02x:%02x:%02x:%02x:%02x\t%lld\t%d\t%d\t%lld\t%lld\t%lld\t%d\t%d\t%d\n",
                result[i].addr[0], result[i].addr[1], result[i].addr[2], result[i].addr[3],
                result[i].addr[4], result[i].addr[5], result[i].ts, result[i].rssi,
                result[i].rssi_spread, result[i].rtt, result[i].rtt_sd, result[i].rtt_spread,
                result[i].distance, result[i].distance_sd, result[i].distance_spread);
    }

    putEventInCache(EVENT_TYPE_RTT_RESULTS, "RTT results");
}

static void onHotlistAPFound(wifi_request_id id, unsigned num_results, wifi_scan_result *results) {

    printMsg("Found hotlist APs\n");
    for (unsigned i = 0; i < num_results; i++) {
        printScanResult(results[i]);
    }
    putEventInCache(EVENT_TYPE_HOTLIST_AP_FOUND, "Found a hotlist AP");
}

static void onHotlistAPLost(wifi_request_id id, unsigned num_results, wifi_scan_result *results) {

    printMsg("Lost hotlist APs\n");
    for (unsigned i = 0; i < num_results; i++) {
        printScanResult(results[i]);
    }
    putEventInCache(EVENT_TYPE_HOTLIST_AP_LOST, "Lost event Hotlist APs");
}

static void testRTT() {

    wifi_scan_result results[256];
    int num_results = scanOnce(WIFI_BAND_ABG, results, countof(results));
    if (num_results == 0) {
        printMsg("RTT aborted because of no scan results\n");
        return;
    } else {
        printMsg("Retrieved %d scan results\n", num_results);
    }

    num_results = removeDuplicateScanResults(results, num_results);
    /*
    printMsg("Deduped scan results - %d\n", num_results);
    for (int i = 0; i < num_results; i++) {
        printScanResult(results[i]);
    }
    */

    sortScanResultsByRssi(results, num_results);
    printMsg("Sorted scan results -\n");
    for (int i = 0; i < num_results; i++) {
        printScanResult(results[i]);
    }


    static const int max_ap = 5;
    wifi_rtt_config params[max_ap];
    memset(params, 0, sizeof(params));

    printMsg("Configuring RTT for %d APs, num_samples = %d\n",
            min(num_results, max_ap), rtt_samples);

    unsigned num_ap = 0;
    for (int i = 0; i < min(num_results, max_ap); i++, num_ap++) {

        memcpy(params[i].addr, results[i].bssid, sizeof(mac_addr));
        mac_addr &addr = params[i].addr;
        printMsg("Adding %02x:%02x:%02x:%02x:%02x:%02x (%d) for RTT\n", addr[0],
                addr[1], addr[2], addr[3], addr[4], addr[5], results[i].channel);

        params[i].type  = RTT_TYPE_1_SIDED;
        params[i].channel.center_freq = results[i].channel;
        params[i].channel.width = WIFI_CHAN_WIDTH_20;
        params[i].peer  = WIFI_PEER_INVALID;
        params[i].continuous = 1;
        params[i].interval = 1000;
        params[i].num_samples_per_measurement = rtt_samples;
        params[i].num_retries_per_measurement = 10;
    }

    wifi_rtt_event_handler handler;
    handler.on_rtt_results = &onRTTResults;

    int result = wifi_rtt_range_request(rttCmdId, wlan0Handle, num_ap, params, handler);

    if (result == WIFI_SUCCESS) {
        printMsg("Waiting for RTT results\n");

        while (true) {
            EventInfo info;
            memset(&info, 0, sizeof(info));
            getEventFromCache(info);

            if (info.type == EVENT_TYPE_SCAN_RESULTS_AVAILABLE) {
                retrieveScanResults();
            } else if (info.type == EVENT_TYPE_RTT_RESULTS) {
                break;
            }
        }
    } else {
        printMsg("Could not set setRTTAPs : %d\n", result);
    }
}


static wifi_error setHotlistAPsUsingScanResult(wifi_bssid_hotlist_params *params){
    printMsg("testHotlistAPs Scan started, waiting for event ...\n");
    EventInfo info;
    memset(&info, 0, sizeof(info));
    getEventFromCache(info);

    wifi_scan_result results[256];
    memset(results, 0, sizeof(wifi_scan_result) * 256);

    printMsg("Retrieving scan results for Hotlist AP setting\n");
    int num_results = 256;
    int result = wifi_get_cached_gscan_results(wlan0Handle, 1, num_results, results, &num_results);
    if (result < 0) {
        printMsg("failed to fetch scan results : %d\n", result);
        return WIFI_ERROR_UNKNOWN;
    } else {
        printMsg("fetched %d scan results\n", num_results);
    }

    for (int i = 0; i < num_results; i++) {
        printScanResult(results[i]);
    }

    for (int i = 0; i < stest_max_ap; i++) {
        memcpy(params->ap[i].bssid, results[i].bssid, sizeof(mac_addr));
        params->ap[i].low  = -htest_low_threshold;
        params->ap[i].high = -htest_high_threshold;
    }
    params->num_ap = stest_max_ap;
    return WIFI_SUCCESS;
}

static wifi_error setHotlistAPs() {
    wifi_bssid_hotlist_params params;
    memset(&params, 0, sizeof(params));

    params.lost_ap_sample_size = HOTLIST_LOST_WINDOW;
    if (num_hotlist_bssids > 0) {
      for (int i = 0; i < num_hotlist_bssids; i++) {
          memcpy(params.ap[i].bssid, hotlist_bssids[i], sizeof(mac_addr));
          params.ap[i].low  = -htest_low_threshold;
          params.ap[i].high = -htest_high_threshold;
      }
      params.num_ap = num_hotlist_bssids;
    } else {
      setHotlistAPsUsingScanResult(&params);
    }

    printMsg("BSSID\t\t\tHIGH\tLOW\n");
    for (int i = 0; i < params.num_ap; i++) {
        mac_addr &addr = params.ap[i].bssid;
        printMsg("%02x:%02x:%02x:%02x:%02x:%02x\t%d\t%d\n", addr[0],
                addr[1], addr[2], addr[3], addr[4], addr[5],
                params.ap[i].high, params.ap[i].low);
    }

    wifi_hotlist_ap_found_handler handler;
    handler.on_hotlist_ap_found = &onHotlistAPFound;
    handler.on_hotlist_ap_lost = &onHotlistAPLost;
    hotlistCmdId = getNewCmdId();
    printMsg("Setting hotlist APs threshold\n");
    return wifi_set_bssid_hotlist(hotlistCmdId, wlan0Handle, params, handler);
}

static void resetHotlistAPs() {
    printMsg(", stoping Hotlist AP scanning\n");
    wifi_reset_bssid_hotlist(hotlistCmdId, wlan0Handle);
}

static void setPnoMacOui() {
    wifi_set_scanning_mac_oui(wlan0Handle, mac_oui);
}

static void testHotlistAPs(){

    EventInfo info;
    memset(&info, 0, sizeof(info));

    printMsg("starting Hotlist AP scanning\n");
    if (!startScan(&onScanResultsAvailable, stest_max_ap,stest_base_period, stest_threshold)) {
        printMsg("testHotlistAPs failed to start scan!!\n");
        return;
    }

    int result = setHotlistAPs();
    if (result == WIFI_SUCCESS) {
        printMsg("Waiting for Hotlist AP event\n");
        while (true) {
            memset(&info, 0, sizeof(info));
            getEventFromCache(info);

            if (info.type == EVENT_TYPE_SCAN_RESULTS_AVAILABLE) {
                retrieveScanResults();
            } else if (info.type == EVENT_TYPE_HOTLIST_AP_FOUND ||
                   info.type == EVENT_TYPE_HOTLIST_AP_LOST) {
                printMsg("Hotlist APs");
                if (--max_event_wait > 0)
                  printMsg(", waiting for more event ::%d\n", max_event_wait);
                else
                  break;
            }
        }
        resetHotlistAPs();
    } else {
        printMsg("Could not set AP hotlist : %d\n", result);
    }
}

static void onSignificantWifiChange(wifi_request_id id,
        unsigned num_results, wifi_significant_change_result **results)
{
    printMsg("Significant wifi change for %d\n", num_results);
    for (unsigned i = 0; i < num_results; i++) {
        printSignificantChangeResult(results[i]);
    }
    putEventInCache(EVENT_TYPE_SIGNIFICANT_WIFI_CHANGE, "significant wifi change noticed");
}

static int SelectSignificantAPsFromScanResults() {
    wifi_scan_result results[256];
    memset(results, 0, sizeof(wifi_scan_result) * 256);
    printMsg("Retrieving scan results for significant wifi change setting\n");
    int num_results = 256;
    int result = wifi_get_cached_gscan_results(wlan0Handle, 1, num_results, results, &num_results);
    if (result < 0) {
        printMsg("failed to fetch scan results : %d\n", result);
        return WIFI_ERROR_UNKNOWN;
    } else {
        printMsg("fetched %d scan results\n", num_results);
    }

    for (int i = 0; i < num_results; i++) {
        printScanResult(results[i]);
    }

    wifi_significant_change_params params;
    memset(&params, 0, sizeof(params));

    params.rssi_sample_size = swctest_rssi_sample_size;
    params.lost_ap_sample_size = swctest_rssi_lost_ap;
    params.min_breaching = swctest_rssi_min_breaching;

    for (int i = 0; i < stest_max_ap; i++) {
        memcpy(params.ap[i].bssid, results[i].bssid, sizeof(mac_addr));
        params.ap[i].low  = results[i].rssi - swctest_rssi_ch_threshold;
        params.ap[i].high = results[i].rssi + swctest_rssi_ch_threshold;
    }
    params.num_ap = stest_max_ap;

    printMsg("Settting Significant change params rssi_sample_size#%d lost_ap_sample_size#%d"
        " and min_breaching#%d\n", params.rssi_sample_size,
        params.lost_ap_sample_size , params.min_breaching);
    printMsg("BSSID\t\t\tHIGH\tLOW\n");
    for (int i = 0; i < params.num_ap; i++) {
        mac_addr &addr = params.ap[i].bssid;
        printMsg("%02x:%02x:%02x:%02x:%02x:%02x\t%d\t%d\n", addr[0],
                addr[1], addr[2], addr[3], addr[4], addr[5],
                params.ap[i].high, params.ap[i].low);
    }
    wifi_significant_change_handler handler;
    memset(&handler, 0, sizeof(handler));
    handler.on_significant_change = &onSignificantWifiChange;

    int id = getNewCmdId();
    return wifi_set_significant_change_handler(id, wlan0Handle, params, handler);

}

static void untrackSignificantChange() {
    printMsg(", Stop tracking SignificantChange\n");
    wifi_reset_bssid_hotlist(hotlistCmdId, wlan0Handle);
}

static void trackSignificantChange() {
    printMsg("starting trackSignificantChange\n");

    if (!startScan(&onScanResultsAvailable, stest_max_ap,stest_base_period, stest_threshold)) {
        printMsg("trackSignificantChange failed to start scan!!\n");
        return;
    } else {
        printMsg("trackSignificantChange Scan started, waiting for event ...\n");
    }

    EventInfo info;
    memset(&info, 0, sizeof(info));
    getEventFromCache(info);

    int result = SelectSignificantAPsFromScanResults();
    if (result == WIFI_SUCCESS) {
        printMsg("Waiting for significant wifi change event\n");
        while (true) {
            memset(&info, 0, sizeof(info));
            getEventFromCache(info);

            if (info.type == EVENT_TYPE_SCAN_RESULTS_AVAILABLE) {
                retrieveScanResults();
            } else if(info.type == EVENT_TYPE_SIGNIFICANT_WIFI_CHANGE) {
                printMsg("Received significant wifi change");
                if (--max_event_wait > 0)
                    printMsg(", waiting for more event ::%d\n", max_event_wait);
                else
                    break;
            }
        }
        untrackSignificantChange();
    } else {
        printMsg("Failed to set significant change  ::%d\n", result);
    }
}

/* -------------------------------------------  */
/* tests                                        */
/* -------------------------------------------  */

void testScan() {
    printf("starting scan with max_ap_per_scan#%d  base_period#%d  threshold#%d \n",
           stest_max_ap,stest_base_period, stest_threshold);
    if (!startScan(&onScanResultsAvailable, stest_max_ap,stest_base_period, stest_threshold)) {
        printMsg("failed to start scan!!\n");
        return;
    } else {
        EventInfo info;
        memset(&info, 0, sizeof(info));

        while (true) {
            getEventFromCache(info);
            printMsg("retrieved event %d : %s\n", info.type, info.buf);
            retrieveScanResults();
            if(--max_event_wait > 0)
              printMsg("Waiting for more :: %d event \n", max_event_wait);
            else
              break;
        }

        stopScan();
        printMsg("stopped scan\n");
    }
}

void testStopScan() {
    stopScan();
    printMsg("stopped scan\n");
}

byte parseHexChar(char ch) {
    if (isdigit(ch))
        return ch - '0';
    else if ('A' <= ch && ch <= 'F')
        return ch - 'A' + 10;
    else if ('a' <= ch && ch <= 'f')
        return ch - 'a' + 10;
    else {
        printMsg("invalid character in bssid %c\n", ch);
        return 0;
    }
}

byte parseHexByte(char ch1, char ch2) {
    return (parseHexChar(ch1) << 4) | parseHexChar(ch2);
}

void parseMacAddress(const char *str, mac_addr addr) {
    addr[0] = parseHexByte(str[0], str[1]);
    addr[1] = parseHexByte(str[3], str[4]);
    addr[2] = parseHexByte(str[6], str[7]);
    addr[3] = parseHexByte(str[9], str[10]);
    addr[4] = parseHexByte(str[12], str[13]);
    addr[5] = parseHexByte(str[15], str[16]);
    // printMsg("read mac addr: %02x:%02x:%02x:%02x:%02x:%02x\n", addr[0],
    //      addr[1], addr[2], addr[3], addr[4], addr[5]);
}

void parseMacOUI(char *str, unsigned char *addr) {
    addr[0] = parseHexByte(str[0], str[1]);
    addr[1] = parseHexByte(str[3], str[4]);
    addr[2] = parseHexByte(str[6], str[7]);
    printMsg("read mac OUI: %02x:%02x:%02x\n", addr[0],
            addr[1], addr[2]);
}

void readTestOptions(int argc, char *argv[]){

    printf("Total number of argc #%d\n", argc);
    for (int j = 1; j < argc-1; j++) {
        if (strcmp(argv[j], "-max_ap") == 0 && isdigit(argv[j+1][0])) {
            stest_max_ap = atoi(argv[++j]);
            printf(" max_ap #%d\n", stest_max_ap);
        } else if (strcmp(argv[j], "-base_period") == 0 && isdigit(argv[j+1][0])) {
            stest_base_period = atoi(argv[++j]);
            printf(" base_period #%d\n", stest_base_period);
        } else if (strcmp(argv[j], "-threshold") == 0 && isdigit(argv[j+1][0])) {
            stest_threshold = atoi(argv[++j]);
            printf(" threshold #%d\n", stest_threshold);
        } else if (strcmp(argv[j], "-avg_RSSI") == 0 && isdigit(argv[j+1][0])) {
            swctest_rssi_sample_size = atoi(argv[++j]);
            printf(" avg_RSSI #%d\n", swctest_rssi_sample_size);
        } else if (strcmp(argv[j], "-ap_loss") == 0 && isdigit(argv[j+1][0])) {
            swctest_rssi_lost_ap = atoi(argv[++j]);
            printf(" ap_loss #%d\n", swctest_rssi_lost_ap);
        } else if (strcmp(argv[j], "-ap_breach") == 0 && isdigit(argv[j+1][0])) {
            swctest_rssi_min_breaching = atoi(argv[++j]);
            printf(" ap_breach #%d\n", swctest_rssi_min_breaching);
        } else if (strcmp(argv[j], "-ch_threshold") == 0 && isdigit(argv[j+1][0])) {
            swctest_rssi_ch_threshold = atoi(argv[++j]);
            printf(" ch_threshold #%d\n", swctest_rssi_ch_threshold);
        } else if (strcmp(argv[j], "-wt_event") == 0 && isdigit(argv[j+1][0])) {
            max_event_wait = atoi(argv[++j]);
            printf(" wt_event #%d\n", max_event_wait);
        } else if (strcmp(argv[j], "-low_th") == 0 && isdigit(argv[j+1][0])) {
            htest_low_threshold = atoi(argv[++j]);
            printf(" low_threshold #-%d\n", htest_low_threshold);
        } else if (strcmp(argv[j], "-high_th") == 0 && isdigit(argv[j+1][0])) {
            htest_high_threshold = atoi(argv[++j]);
            printf(" high_threshold #-%d\n", htest_high_threshold);
        } else if (strcmp(argv[j], "-hotlist_bssids") == 0 && isxdigit(argv[j+1][0])) {
            j++;
            for (num_hotlist_bssids = 0;
                        j < argc && isxdigit(argv[j][0]);
                        j++, num_hotlist_bssids++) {
                parseMacAddress(argv[j], hotlist_bssids[num_hotlist_bssids]);
            }
            j -= 1;
        } else if (strcmp(argv[j], "-channel_list") == 0 && isxdigit(argv[j+1][0])) {
            j++;
            for (num_channels = 0; j < argc && isxdigit(argv[j][0]); j++, num_channels++) {
                channel_list[num_channels] = atoi(argv[j]);
            }
            j -= 1;
        } else if ((strcmp(argv[j], "-get_ch_list") == 0)) {
            if(strcmp(argv[j + 1], "a") == 0) {
                band = WIFI_BAND_A_WITH_DFS;
            } else if(strcmp(argv[j + 1], "bg") == 0) {
                band = WIFI_BAND_BG;
            } else if(strcmp(argv[j + 1], "abg") == 0) {
                band = WIFI_BAND_ABG_WITH_DFS;
            } else if(strcmp(argv[j + 1], "a_nodfs") == 0) {
                band = WIFI_BAND_A;
            } else if(strcmp(argv[j + 1], "dfs") == 0) {
                band = WIFI_BAND_A_DFS;
            } else if(strcmp(argv[j + 1], "abg_nodfs") == 0) {
                band = WIFI_BAND_ABG;
            }
            j++;
        } else if ((strcmp(argv[j], "-rtt_samples") == 0)) {
            rtt_samples = atoi(argv[++j]);
            printf(" rtt_retries #-%d\n", rtt_samples);
        } else if (strcmp(argv[j], "-scan_mac_oui") == 0 && isxdigit(argv[j+1][0])) {
            parseMacOUI(argv[++j], mac_oui);
     }
    }
}

wifi_iface_stat link_stat;
wifi_radio_stat trx_stat;
wifi_peer_info peer_info;
wifi_rate_stat rate_stat[32];
void onLinkStatsResults(wifi_request_id id, wifi_iface_stat *iface_stat,
         int num_radios, wifi_radio_stat *radio_stat)
{
    int num_peer = iface_stat->num_peers;
    memcpy(&trx_stat, radio_stat, sizeof(wifi_radio_stat));
    memcpy(&link_stat, iface_stat, sizeof(wifi_iface_stat));
    memcpy(&peer_info, iface_stat->peer_info, num_peer*sizeof(wifi_peer_info));
    int num_rate = peer_info.num_rate;
    memcpy(&rate_stat, iface_stat->peer_info->rate_stats, num_rate*sizeof(wifi_rate_stat));
}

void printFeatureListBitMask(void)
{
    printMsg("WIFI_FEATURE_INFRA              0x0001      - Basic infrastructure mode\n");
    printMsg("WIFI_FEATURE_INFRA_5G           0x0002      - Support for 5 GHz Band\n");
    printMsg("WIFI_FEATURE_HOTSPOT            0x0004      - Support for GAS/ANQP\n");
    printMsg("WIFI_FEATURE_P2P                0x0008      - Wifi-Direct\n");
    printMsg("WIFI_FEATURE_SOFT_AP            0x0010      - Soft AP\n");
    printMsg("WIFI_FEATURE_GSCAN              0x0020      - Google-Scan APIs\n");
    printMsg("WIFI_FEATURE_NAN                0x0040      - Neighbor Awareness Networking\n");
    printMsg("WIFI_FEATURE_D2D_RTT            0x0080      - Device-to-device RTT\n");
    printMsg("WIFI_FEATURE_D2AP_RTT           0x0100      - Device-to-AP RTT\n");
    printMsg("WIFI_FEATURE_BATCH_SCAN         0x0200      - Batched Scan (legacy)\n");
    printMsg("WIFI_FEATURE_PNO                0x0400      - Preferred network offload\n");
    printMsg("WIFI_FEATURE_ADDITIONAL_STA     0x0800      - Support for two STAs\n");
    printMsg("WIFI_FEATURE_TDLS               0x1000      - Tunnel directed link setup\n");
    printMsg("WIFI_FEATURE_TDLS_OFFCHANNEL    0x2000      - Support for TDLS off channel\n");
    printMsg("WIFI_FEATURE_EPR                0x4000      - Enhanced power reporting\n");
    printMsg("WIFI_FEATURE_AP_STA             0x8000      - Support for AP STA Concurrency\n");
}

char *rates[] = {
    "1Mbps",
    "2Mbps",
	"5.5Mbps",
	"6Mbps",
	"9Mbps",
	"11Mbps",
	"12Mbps",
	"18Mbps",
	"24Mbps",
	"36Mbps",
	"48Mbps",
	"54Mbps",
	"VHT MCS0 ss1",
	"VHT MCS1 ss1",
	"VHT MCS2 ss1",
	"VHT MCS3 ss1",
	"VHT MCS4 ss1",
	"VHT MCS5 ss1",
	"VHT MCS6 ss1",
	"VHT MCS7 ss1",
    "VHT MCS8 ss1",
	"VHT MCS9 ss1",
	"VHT MCS0 ss2",
	"VHT MCS1 ss2",
	"VHT MCS2 ss2",
	"VHT MCS3 ss2",
	"VHT MCS4 ss2",
	"VHT MCS5 ss2",
	"VHT MCS6 ss2",
	"VHT MCS7 ss2",
	"VHT MCS8 ss2",
	"VHT MCS9 ss2"
	};

void printLinkStats(wifi_iface_stat link_stat, wifi_radio_stat trx_stat)
{
    printMsg("Printing link layer statistics:\n");
    printMsg("-------------------------------\n");
    printMsg("beacon_rx = %d\n", link_stat.beacon_rx);
    printMsg("RSSI = %d\n", link_stat.rssi_mgmt);
    printMsg("AC_BE:\n");
    printMsg("txmpdu = %d\n", link_stat.ac[WIFI_AC_BE].tx_mpdu);
    printMsg("rxmpdu = %d\n", link_stat.ac[WIFI_AC_BE].rx_mpdu);
    printMsg("mpdu_lost = %d\n", link_stat.ac[WIFI_AC_BE].mpdu_lost);
    printMsg("retries = %d\n", link_stat.ac[WIFI_AC_BE].retries);
    printMsg("AC_BK:\n");
    printMsg("txmpdu = %d\n", link_stat.ac[WIFI_AC_BK].tx_mpdu);
    printMsg("rxmpdu = %d\n", link_stat.ac[WIFI_AC_BK].rx_mpdu);
    printMsg("mpdu_lost = %d\n", link_stat.ac[WIFI_AC_BK].mpdu_lost);
    printMsg("AC_VI:\n");
    printMsg("txmpdu = %d\n", link_stat.ac[WIFI_AC_VI].tx_mpdu);
    printMsg("rxmpdu = %d\n", link_stat.ac[WIFI_AC_VI].rx_mpdu);
    printMsg("mpdu_lost = %d\n", link_stat.ac[WIFI_AC_VI].mpdu_lost);
    printMsg("AC_VO:\n");
    printMsg("txmpdu = %d\n", link_stat.ac[WIFI_AC_VO].tx_mpdu);
    printMsg("rxmpdu = %d\n", link_stat.ac[WIFI_AC_VO].rx_mpdu);
    printMsg("mpdu_lost = %d\n", link_stat.ac[WIFI_AC_VO].mpdu_lost);
    printMsg("\n");
    printMsg("Printing radio statistics:\n");
    printMsg("--------------------------\n");
    printMsg("on time = %d\n", trx_stat.on_time);
    printMsg("tx time = %d\n", trx_stat.tx_time);
    printMsg("rx time = %d\n", trx_stat.rx_time);
    printMsg("\n");
    printMsg("Printing rate statistics:\n");
    printMsg("-------------------------\n");
    printMsg("%27s %12s %14s %15s\n", "TX",  "RX", "LOST", "RETRIES");
    for (int i=0; i < 32; i++) {
        printMsg("%-15s  %10d   %10d    %10d    %10d\n",
	    rates[i], rate_stat[i].tx_mpdu, rate_stat[i].rx_mpdu,
	    rate_stat[i].mpdu_lost, rate_stat[i].retries);
    }
}

void getLinkStats(void)
{
    wifi_stats_result_handler handler;
    memset(&handler, 0, sizeof(handler));
    handler.on_link_stats_results = &onLinkStatsResults;

    int result = wifi_get_link_stats(0, wlan0Handle, handler);
    if (result < 0) {
        printMsg("failed to get link statistics - %d\n", result);
    } else {
        printLinkStats(link_stat, trx_stat);
    }
}

void getChannelList(void)
{
    wifi_channel channel[MAX_CH_BUF_SIZE];
    int num_channels = 0, i;

    int result = wifi_get_valid_channels(wlan0Handle, band, MAX_CH_BUF_SIZE,
                     channel, &num_channels);
    printMsg("Number of channels - %d\nChannel List:\n",num_channels);
    for (i = 0; i < num_channels; i++) {
        printMsg("%d MHz\n", channel[i]);
    }
}

void getFeatureSet(void)
{
    feature_set set;
    int result = wifi_get_supported_feature_set(wlan0Handle, &set);

    if (result < 0) {
        printMsg("Error %d\n",result);
        return;
    }
    printFeatureListBitMask();
    printMsg("Supported feature set bit mask - %x\n", set);
    return;
}

void getFeatureSetMatrix(void)
{
    feature_set set[MAX_FEATURE_SET];
    int size;

    int result = wifi_get_concurrency_matrix(wlan0Handle, MAX_FEATURE_SET, set, &size);

    if (result < 0) {
        printMsg("Error %d\n",result);
        return;
    }
    printFeatureListBitMask();
    for (int i = 0; i < size; i++)
        printMsg("Concurrent feature set - %x\n", set[i]);
    return;
}



int main(int argc, char *argv[]) {

    pthread_mutex_init(&printMutex, NULL);

    if (init() != 0) {
        printMsg("could not initiate HAL");
        return -1;
    } else {
        printMsg("successfully initialized HAL; wlan0 = %p\n", wlan0Handle);
    }

    pthread_cond_init(&eventCacheCondition, NULL);
    pthread_mutex_init(&eventCacheMutex, NULL);

    pthread_t tidEvent;
    pthread_create(&tidEvent, NULL, &eventThreadFunc, NULL);

    sleep(2);     // let the thread start

    if (argc < 2 || argv[1][0] != '-') {
        printf("Usage:  halutil [OPTION]\n");
        printf(" -s               start AP scan test\n");
        printf(" -swc             start Significant Wifi change test\n");
        printf(" -h               start Hotlist APs scan test\n");
        printf(" -ss              stop scan test\n");
        printf(" -max_ap          Max AP for scan \n");
        printf(" -base_period     Base period for scan \n");
        printf(" -threshold       Threshold scan test\n");
        printf(" -avg_RSSI        samples for averaging RSSI\n");
        printf(" -ap_loss         samples to confirm AP loss\n");
        printf(" -ap_breach       APs breaching threshold\n");
        printf(" -ch_threshold    Change in threshold\n");
        printf(" -wt_event        Waiting event for test\n");
        printf(" -low_th          Low threshold for hotlist APs\n");
        printf(" -hight_th        High threshold for hotlist APs\n");
        printf(" -hotlist_bssids  BSSIDs for hotlist test\n");
        printf(" -stats       print link layer statistics\n");
        printf(" -get_ch_list <a/bg/abg/a_nodfs/abg_nodfs/dfs>  Get channel list\n");
        printf(" -get_feature_set  Get Feature set\n");
        printf(" -get_feature_matrix  Get concurrent feature matrix\n");
        printf(" -rtt             Run RTT on nearby APs\n");
        printf(" -rtt_samples     Run RTT on nearby APs\n");
        printf(" -scan_mac_oui XY:AB:CD\n");
        printf(" -nodfs <0|1>     Turn OFF/ON non-DFS locales\n");
        goto cleanup;
    }
    memset(mac_oui, 0, 3);

    if (strcmp(argv[1], "-s") == 0) {
        readTestOptions(argc, argv);
        setPnoMacOui();
        testScan();
    }else if(strcmp(argv[1], "-swc") == 0){
        readTestOptions(argc, argv);
        setPnoMacOui();
        trackSignificantChange();
    }else if (strcmp(argv[1], "-ss") == 0) {
        // Stop scan so clear the OUI too
        setPnoMacOui();
        testStopScan();
    }else if ((strcmp(argv[1], "-h") == 0)  ||
              (strcmp(argv[1], "-hotlist_bssids") == 0)) {
        readTestOptions(argc, argv);
        setPnoMacOui();
        testHotlistAPs();
    }else if (strcmp(argv[1], "-stats") == 0) {
        getLinkStats();
    } else if ((strcmp(argv[1], "-rtt") == 0)) {
        readTestOptions(argc, argv);
        testRTT();
    } else if ((strcmp(argv[1], "-get_ch_list") == 0)) {
        readTestOptions(argc, argv);
        getChannelList();
    } else if ((strcmp(argv[1], "-get_feature_set") == 0)) {
        getFeatureSet();
    } else if ((strcmp(argv[1], "-get_feature_matrix") == 0)) {
        getFeatureSetMatrix();
    } else if ((strcmp(argv[1], "-scan_mac_oui") == 0)) {
        readTestOptions(argc, argv);
        setPnoMacOui();
        testScan();
    } else if (strcmp(argv[1], "-nodfs") == 0) {
        u32 nodfs = 0;
        if (argc > 2)
            nodfs = (u32)atoi(argv[2]);
        wifi_set_nodfs_flag(wlan0Handle, nodfs);
    }
cleanup:
    cleanup();
    return 0;
}
